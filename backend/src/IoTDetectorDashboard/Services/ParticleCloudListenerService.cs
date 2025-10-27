using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;
using IoTDetectorDashboard.Hubs;
using Microsoft.AspNetCore.SignalR;
using System.Diagnostics;
using System.Text.Json;

namespace IoTDetectorDashboard.Services;

public class ParticleCloudListenerService : BackgroundService
{
    private readonly IServiceProvider _serviceProvider;
    private readonly IHubContext<SensorHub> _hubContext;
    private readonly ILogger<ParticleCloudListenerService> _logger;
    private readonly IConfiguration _configuration;
    private Process? _particleProcess;

    public ParticleCloudListenerService(
        IServiceProvider serviceProvider,
        IHubContext<SensorHub> hubContext,
        ILogger<ParticleCloudListenerService> logger,
        IConfiguration configuration)
    {
        _serviceProvider = serviceProvider;
        _hubContext = hubContext;
        _logger = logger;
        _configuration = configuration;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("Particle Cloud Listener Service starting...");

        // Get Particle credentials
        var particleEnabled = _configuration.GetValue<bool>("Particle:Enabled", true);
        var particleToken = _configuration["Particle:AccessToken"];
        var eventName = _configuration["Particle:EventName"] ?? "pir_motion";

        if (!particleEnabled)
        {
            _logger.LogInformation("Particle Cloud listener is disabled in configuration");
            return;
        }

        if (string.IsNullOrEmpty(particleToken))
        {
            _logger.LogWarning("Particle access token not configured. Attempting to use logged-in account...");
        }

        try
        {
            // Start particle subscribe process
            var startInfo = new ProcessStartInfo
            {
                FileName = "particle",
                Arguments = $"subscribe {eventName} --json",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            _particleProcess = new Process { StartInfo = startInfo };

            _particleProcess.OutputDataReceived += async (sender, e) =>
            {
                if (!string.IsNullOrEmpty(e.Data))
                {
                    try
                    {
                        await ProcessParticleEventAsync(e.Data, stoppingToken);
                    }
                    catch (Exception ex)
                    {
                        _logger.LogError(ex, "Error processing Particle event: {Data}", e.Data);
                    }
                }
            };

            _particleProcess.ErrorDataReceived += (sender, e) =>
            {
                if (!string.IsNullOrEmpty(e.Data))
                {
                    _logger.LogWarning("Particle CLI error: {Error}", e.Data);
                }
            };

            _particleProcess.Start();
            _particleProcess.BeginOutputReadLine();
            _particleProcess.BeginErrorReadLine();

            _logger.LogInformation("Particle Cloud listener started successfully for event '{EventName}'", eventName);

            // Wait until cancelled
            await Task.Delay(Timeout.Infinite, stoppingToken);
        }
        catch (TaskCanceledException)
        {
            _logger.LogInformation("Particle Cloud Listener Service is stopping...");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in Particle Cloud Listener Service");
        }
    }

    private async Task ProcessParticleEventAsync(string jsonData, CancellationToken cancellationToken)
    {
        _logger.LogDebug("Received Particle event: {Data}", jsonData);

        try
        {
            var particleEvent = JsonSerializer.Deserialize<ParticleEvent>(jsonData,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

            if (particleEvent == null || string.IsNullOrEmpty(particleEvent.Coreid))
            {
                _logger.LogWarning("Invalid Particle event format");
                return;
            }

            // Parse the data payload
            ParticleTelemetry? telemetry = null;
            if (!string.IsNullOrEmpty(particleEvent.Data))
            {
                try
                {
                    telemetry = JsonSerializer.Deserialize<ParticleTelemetry>(particleEvent.Data,
                        new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
                }
                catch
                {
                    // If not JSON, treat as simple string event
                    _logger.LogDebug("Particle data is not JSON: {Data}", particleEvent.Data);
                }
            }

            var deviceId = particleEvent.Coreid;
            var eventType = particleEvent.Event ?? "unknown";

            _logger.LogInformation("Processing Particle event from device {DeviceId}: {Event}", deviceId, eventType);

            // Process in a new scope
            using var scope = _serviceProvider.CreateScope();
            var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();
            var alertService = scope.ServiceProvider.GetRequiredService<AlertGenerationService>();

            // Ensure device exists
            var device = await context.Devices.FindAsync(deviceId);
            if (device == null)
            {
                device = new Device
                {
                    DeviceId = deviceId,
                    DeviceType = "Photon2",
                    Status = "Connected",
                    LastConnected = DateTime.UtcNow,
                    LastDataReceived = DateTime.UtcNow
                };
                context.Devices.Add(device);
                _logger.LogInformation("New Photon2 device registered: {DeviceId}", deviceId);
            }
            else
            {
                device.Status = "Connected";
                device.LastDataReceived = DateTime.UtcNow;
            }

            // Create sensor data entry
            var sensorData = new SensorData
            {
                DeviceId = deviceId,
                EventType = eventType,
                MotionCount = telemetry?.MotionCount ?? (eventType.Contains("motion") ? 1 : null),
                Rssi = telemetry?.Rssi,
                FreeMemory = telemetry?.FreeMemory,
                Uptime = telemetry?.Uptime,
                FirmwareVersion = telemetry?.FirmwareVersion,
                Timestamp = particleEvent.Published_at ?? DateTime.UtcNow
            };

            context.SensorData.Add(sensorData);
            await context.SaveChangesAsync(cancellationToken);

            _logger.LogInformation("Sensor data saved from Particle: {DeviceId} - {EventType}", deviceId, eventType);

            // Check for alert conditions
            await alertService.CheckAndGenerateAlertsAsync(device, sensorData, context);

            // Broadcast to SignalR clients
            await _hubContext.Clients.All.SendAsync("ReceiveSensorData", new
            {
                deviceId,
                eventType = sensorData.EventType,
                motionCount = sensorData.MotionCount,
                rssi = sensorData.Rssi,
                freeMemory = sensorData.FreeMemory,
                uptime = sensorData.Uptime,
                firmwareVersion = sensorData.FirmwareVersion,
                timestamp = sensorData.Timestamp,
                source = "Particle Cloud"
            }, cancellationToken);

            _logger.LogDebug("Particle sensor data broadcasted via SignalR");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing Particle event JSON: {Data}", jsonData);
        }
    }

    public override async Task StopAsync(CancellationToken cancellationToken)
    {
        _logger.LogInformation("Particle Cloud Listener Service stopping...");

        if (_particleProcess != null && !_particleProcess.HasExited)
        {
            _particleProcess.Kill(true);
            await _particleProcess.WaitForExitAsync(cancellationToken);
            _particleProcess.Dispose();
        }

        await base.StopAsync(cancellationToken);
    }
}

// Particle event models
public class ParticleEvent
{
    public string? Event { get; set; }
    public string? Data { get; set; }
    public DateTime? Published_at { get; set; }
    public string? Coreid { get; set; }
}

public class ParticleTelemetry
{
    public int? MotionCount { get; set; }
    public int? Rssi { get; set; }
    public long? FreeMemory { get; set; }
    public long? Uptime { get; set; }
    public string? FirmwareVersion { get; set; }
}
