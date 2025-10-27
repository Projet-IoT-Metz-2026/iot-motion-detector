using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;

namespace IoTDetectorDashboard.Controllers;

public class MonitoringController : Controller
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<MonitoringController> _logger;
    private readonly IConfiguration _configuration;

    public MonitoringController(ApplicationDbContext context, ILogger<MonitoringController> logger, IConfiguration configuration)
    {
        _context = context;
        _logger = logger;
        _configuration = configuration;
    }

    // GET: Monitoring
    public async Task<IActionResult> Index()
    {
        var viewModel = new MonitoringViewModel
        {
            SystemStatus = GetSystemStatus(),
            MessageRateData = await GetMessageRateDataAsync(),
            LatencyData = await GetLatencyDataAsync(),
            ProvisioningLogs = await GetProvisioningLogsAsync()
        };

        return View(viewModel);
    }

    private List<SystemStatusItem> GetSystemStatus()
    {
        var azureEnabled = _configuration.GetValue<bool>("Azure:IoTHub:Enabled");
        var azureConnectionString = _configuration.GetValue<string>("Azure:IoTHub:ConnectionString");
        var particleEnabled = _configuration.GetValue<bool>("Particle:Enabled");

        var deviceCount = _context.Devices.Count();
        var connectedDevices = _context.Devices.Count(d => d.IsConnected);
        var disconnectedDevices = deviceCount - connectedDevices;

        return new List<SystemStatusItem>
        {
            new SystemStatusItem
            {
                Component = "Azure IoT Hub",
                Status = azureEnabled && !string.IsNullOrEmpty(azureConnectionString) ? "ok" : "warning",
                Message = azureEnabled ? "Opérationnel" : "Désactivé"
            },
            new SystemStatusItem
            {
                Component = "Particle Cloud",
                Status = particleEnabled ? "ok" : "warning",
                Message = particleEnabled ? "Opérationnel" : "Désactivé"
            },
            new SystemStatusItem
            {
                Component = "Device Registry",
                Status = deviceCount > 0 ? "ok" : "warning",
                Message = $"{deviceCount} appareil(s) enregistré(s)"
            },
            new SystemStatusItem
            {
                Component = "Device Connectivity",
                Status = disconnectedDevices > 0 ? "warning" : "ok",
                Message = $"{connectedDevices}/{deviceCount} appareil(s) connecté(s)"
            },
            new SystemStatusItem
            {
                Component = "Database",
                Status = "ok",
                Message = "InMemory Database actif"
            }
        };
    }

    private async Task<List<MessageRateData>> GetMessageRateDataAsync()
    {
        // Get data for the last 7 hours
        var now = DateTime.UtcNow;
        var data = new List<MessageRateData>();

        for (int i = 6; i >= 0; i--)
        {
            var hourStart = now.AddHours(-i).Date.AddHours(now.AddHours(-i).Hour);
            var hourEnd = hourStart.AddHours(1);

            var messageCount = await _context.SensorData
                .Where(s => s.Timestamp >= hourStart && s.Timestamp < hourEnd)
                .CountAsync();

            data.Add(new MessageRateData
            {
                Time = hourStart.ToString("HH:mm"),
                Messages = messageCount,
                Errors = 0 // TODO: Track errors in the future
            });
        }

        return data;
    }

    private async Task<List<LatencyData>> GetLatencyDataAsync()
    {
        // Simulate latency data (in real scenario, track message receive time vs sent time)
        var now = DateTime.UtcNow;
        var data = new List<LatencyData>();

        for (int i = 6; i >= 0; i--)
        {
            var hourStart = now.AddHours(-i);

            data.Add(new LatencyData
            {
                Time = hourStart.ToString("HH:mm"),
                Latency = Random.Shared.Next(30, 60) // Simulated latency in ms
            });
        }

        return data;
    }

    private async Task<List<ProvisioningLog>> GetProvisioningLogsAsync()
    {
        var devices = await _context.Devices
            .OrderByDescending(d => d.LastConnected)
            .Take(10)
            .ToListAsync();

        return devices.Select(d => new ProvisioningLog
        {
            Device = d.DeviceId,
            Status = d.IsConnected ? "success" : "warning",
            Timestamp = d.LastConnected?.ToString("yyyy-MM-dd HH:mm:ss") ?? "N/A",
            Duration = "N/A"
        }).ToList();
    }
}

public class MonitoringViewModel
{
    public List<SystemStatusItem> SystemStatus { get; set; } = new();
    public List<MessageRateData> MessageRateData { get; set; } = new();
    public List<LatencyData> LatencyData { get; set; } = new();
    public List<ProvisioningLog> ProvisioningLogs { get; set; } = new();
}

public class SystemStatusItem
{
    public string Component { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty; // ok, warning, error
    public string Message { get; set; } = string.Empty;
}

public class MessageRateData
{
    public string Time { get; set; } = string.Empty;
    public int Messages { get; set; }
    public int Errors { get; set; }
}

public class LatencyData
{
    public string Time { get; set; } = string.Empty;
    public int Latency { get; set; }
}

public class ProvisioningLog
{
    public string Device { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty;
    public string Timestamp { get; set; } = string.Empty;
    public string Duration { get; set; } = string.Empty;
}
