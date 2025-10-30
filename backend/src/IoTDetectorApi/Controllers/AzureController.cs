using Microsoft.AspNetCore.Mvc;
using Microsoft.Azure.Devices;
using Microsoft.EntityFrameworkCore;
using IoTDetectorApi.Data;
using IoTDetectorApi.Models;

namespace IoTDetectorApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class AzureController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly IConfiguration _configuration;
    private readonly ILogger<AzureController> _logger;

    public AzureController(
        ApplicationDbContext context,
        IConfiguration configuration,
        ILogger<AzureController> logger)
    {
        _context = context;
        _configuration = configuration;
        _logger = logger;
    }

    [HttpPost("test-connection")]
    public async Task<ActionResult<object>> TestConnection()
    {
        try
        {
            var connectionString = _configuration.GetValue<string>("Azure:IoTHub:ConnectionString");

            if (string.IsNullOrEmpty(connectionString))
            {
                return BadRequest(new { success = false, message = "Connection string not configured" });
            }

            var registryManager = RegistryManager.CreateFromConnectionString(connectionString);

            // Try to get IoT Hub statistics as a connection test
            var stats = await registryManager.GetRegistryStatisticsAsync();

            await registryManager.CloseAsync();

            return Ok(new
            {
                success = true,
                message = "Connexion réussie à Azure IoT Hub",
                totalDeviceCount = stats.TotalDeviceCount,
                enabledDeviceCount = stats.EnabledDeviceCount,
                disabledDeviceCount = stats.DisabledDeviceCount
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to test Azure IoT Hub connection");
            return Ok(new
            {
                success = false,
                message = $"Échec de connexion: {ex.Message}"
            });
        }
    }

    [HttpPost("sync-devices")]
    public async Task<ActionResult<object>> SyncDevices()
    {
        try
        {
            var connectionString = _configuration.GetValue<string>("Azure:IoTHub:ConnectionString");

            if (string.IsNullOrEmpty(connectionString))
            {
                return BadRequest(new { success = false, message = "Connection string not configured" });
            }

            var registryManager = RegistryManager.CreateFromConnectionString(connectionString);

            // Get all devices from Azure IoT Hub
            var query = registryManager.CreateQuery("SELECT * FROM devices", 100);
            var devicesAdded = 0;
            var devicesUpdated = 0;

            while (query.HasMoreResults)
            {
                var page = await query.GetNextAsTwinAsync();

                foreach (var twin in page)
                {
                    var deviceId = twin.DeviceId;
                    var existingDevice = await _context.Devices.FindAsync(deviceId);

                    if (existingDevice == null)
                    {
                        // Add new device
                        var device = new Models.Device
                        {
                            Id = deviceId,
                            Name = deviceId,
                            Type = deviceId.Contains("esp32") ? "ESP32" :
                                   deviceId.Contains("photon") ? "Photon2" : "Unknown",
                            Status = twin.ConnectionState.ToString() == "Connected" ? "active" : "inactive",
                            LastSeen = twin.LastActivityTime,
                            CreatedAt = DateTime.UtcNow
                        };

                        _context.Devices.Add(device);
                        devicesAdded++;

                        _logger.LogInformation("Added device from Azure: {DeviceId}", deviceId);
                    }
                    else
                    {
                        // Update existing device
                        existingDevice.Status = twin.ConnectionState.ToString() == "Connected" ? "active" : "inactive";
                        existingDevice.LastSeen = twin.LastActivityTime;
                        devicesUpdated++;

                        _logger.LogInformation("Updated device from Azure: {DeviceId}", deviceId);
                    }
                }
            }

            await _context.SaveChangesAsync();
            await registryManager.CloseAsync();

            // Log the sync event
            var eventLog = new EventLog
            {
                Message = $"Synchronisation Azure: {devicesAdded} ajoutés, {devicesUpdated} mis à jour",
                Level = "info",
                Timestamp = DateTime.UtcNow
            };
            _context.EventLogs.Add(eventLog);
            await _context.SaveChangesAsync();

            return Ok(new
            {
                success = true,
                message = "Synchronisation terminée",
                devicesAdded,
                devicesUpdated
            });
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to sync devices from Azure IoT Hub");
            return Ok(new
            {
                success = false,
                message = $"Échec de synchronisation: {ex.Message}"
            });
        }
    }
}
