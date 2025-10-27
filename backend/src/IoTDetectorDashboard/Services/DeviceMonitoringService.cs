using IoTDetectorDashboard.Data;
using Microsoft.EntityFrameworkCore;

namespace IoTDetectorDashboard.Services;

public class DeviceMonitoringService : BackgroundService
{
    private readonly IServiceProvider _serviceProvider;
    private readonly ILogger<DeviceMonitoringService> _logger;
    private readonly TimeSpan _checkInterval = TimeSpan.FromMinutes(5);

    public DeviceMonitoringService(
        IServiceProvider serviceProvider,
        ILogger<DeviceMonitoringService> logger)
    {
        _serviceProvider = serviceProvider;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("Device Monitoring Service starting...");
        _logger.LogInformation("Checking for offline devices every {Interval} minutes", _checkInterval.TotalMinutes);

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await CheckOfflineDevicesAsync();
                await Task.Delay(_checkInterval, stoppingToken);
            }
            catch (TaskCanceledException)
            {
                // Normal cancellation, service is stopping
                break;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error in Device Monitoring Service");
                // Wait before retrying
                await Task.Delay(TimeSpan.FromSeconds(30), stoppingToken);
            }
        }

        _logger.LogInformation("Device Monitoring Service stopped");
    }

    private async Task CheckOfflineDevicesAsync()
    {
        using var scope = _serviceProvider.CreateScope();
        var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();
        var alertService = scope.ServiceProvider.GetRequiredService<AlertGenerationService>();

        try
        {
            await alertService.CheckDeviceOfflineAsync(context);
            _logger.LogDebug("Device offline check completed");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking offline devices");
        }
    }
}
