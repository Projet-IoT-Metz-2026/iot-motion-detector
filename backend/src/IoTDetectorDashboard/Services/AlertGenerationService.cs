using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;
using IoTDetectorDashboard.Hubs;
using Microsoft.AspNetCore.SignalR;
using Microsoft.EntityFrameworkCore;

namespace IoTDetectorDashboard.Services;

public class AlertGenerationService
{
    private readonly IHubContext<SensorHub> _hubContext;
    private readonly ILogger<AlertGenerationService> _logger;

    public AlertGenerationService(
        IHubContext<SensorHub> hubContext,
        ILogger<AlertGenerationService> logger)
    {
        _hubContext = hubContext;
        _logger = logger;
    }

    public async Task CheckAndGenerateAlertsAsync(Device device, SensorData sensorData, ApplicationDbContext context)
    {
        var alerts = new List<Alert>();

        // Rule 1: Check for high motion activity (motion threshold)
        if (sensorData.EventType == "motion_detected" && sensorData.MotionCount.HasValue)
        {
            // Check last 5 minutes for high frequency
            var fiveMinutesAgo = DateTime.UtcNow.AddMinutes(-5);
            var recentDetections = await context.SensorData
                .Where(s => s.DeviceId == device.DeviceId &&
                           s.EventType == "motion_detected" &&
                           s.Timestamp >= fiveMinutesAgo)
                .CountAsync();

            if (recentDetections >= 10)
            {
                var existingAlert = await context.Alerts
                    .Where(a => a.DeviceId == device.DeviceId &&
                               a.AlertType == "motion_threshold" &&
                               !a.Acknowledged &&
                               a.Timestamp >= fiveMinutesAgo)
                    .FirstOrDefaultAsync();

                if (existingAlert == null)
                {
                    alerts.Add(new Alert
                    {
                        DeviceId = device.DeviceId,
                        AlertType = "motion_threshold",
                        Message = $"High motion activity detected ({recentDetections} detections in 5 minutes)",
                        Timestamp = DateTime.UtcNow,
                        Acknowledged = false
                    });
                    _logger.LogWarning("Motion threshold alert for device {DeviceId}: {Count} detections",
                        device.DeviceId, recentDetections);
                }
            }
        }

        // Rule 2: Check for weak signal (RSSI)
        if (sensorData.Rssi.HasValue && sensorData.Rssi < -80)
        {
            var oneHourAgo = DateTime.UtcNow.AddHours(-1);
            var existingAlert = await context.Alerts
                .Where(a => a.DeviceId == device.DeviceId &&
                           a.AlertType == "weak_signal" &&
                           !a.Acknowledged &&
                           a.Timestamp >= oneHourAgo)
                .FirstOrDefaultAsync();

            if (existingAlert == null)
            {
                alerts.Add(new Alert
                {
                    DeviceId = device.DeviceId,
                    AlertType = "weak_signal",
                    Message = $"Signal strength below -80 dBm (current: {sensorData.Rssi} dBm)",
                    Timestamp = DateTime.UtcNow,
                    Acknowledged = false
                });
                _logger.LogWarning("Weak signal alert for device {DeviceId}: {Rssi} dBm",
                    device.DeviceId, sensorData.Rssi);
            }
        }

        // Rule 3: Check for low memory
        if (sensorData.FreeMemory.HasValue && sensorData.FreeMemory < 100000) // Less than 100KB
        {
            var oneHourAgo = DateTime.UtcNow.AddHours(-1);
            var existingAlert = await context.Alerts
                .Where(a => a.DeviceId == device.DeviceId &&
                           a.AlertType == "low_memory" &&
                           !a.Acknowledged &&
                           a.Timestamp >= oneHourAgo)
                .FirstOrDefaultAsync();

            if (existingAlert == null)
            {
                alerts.Add(new Alert
                {
                    DeviceId = device.DeviceId,
                    AlertType = "low_memory",
                    Message = $"Low memory detected (free: {sensorData.FreeMemory / 1024} KB)",
                    Timestamp = DateTime.UtcNow,
                    Acknowledged = false
                });
                _logger.LogWarning("Low memory alert for device {DeviceId}: {Memory} bytes",
                    device.DeviceId, sensorData.FreeMemory);
            }
        }

        // Save and broadcast new alerts
        if (alerts.Any())
        {
            context.Alerts.AddRange(alerts);
            await context.SaveChangesAsync();

            foreach (var alert in alerts)
            {
                // Broadcast via SignalR
                await _hubContext.Clients.All.SendAsync("ReceiveAlert", new
                {
                    id = alert.Id,
                    deviceId = alert.DeviceId,
                    alertType = alert.AlertType,
                    message = alert.Message,
                    timestamp = alert.Timestamp,
                    acknowledged = alert.Acknowledged
                });

                _logger.LogInformation("Alert broadcasted via SignalR: {AlertType} for {DeviceId}",
                    alert.AlertType, alert.DeviceId);
            }
        }
    }

    public async Task CheckDeviceOfflineAsync(ApplicationDbContext context)
    {
        // Check for devices that haven't sent data in over 1 hour
        var oneHourAgo = DateTime.UtcNow.AddHours(-1);

        var offlineDevices = await context.Devices
            .Where(d => d.LastDataReceived.HasValue &&
                       d.LastDataReceived < oneHourAgo &&
                       d.Status != "Disconnected")
            .ToListAsync();

        foreach (var device in offlineDevices)
        {
            device.Status = "Disconnected";

            // Check if alert already exists
            var existingAlert = await context.Alerts
                .Where(a => a.DeviceId == device.DeviceId &&
                           a.AlertType == "device_offline" &&
                           !a.Acknowledged &&
                           a.Timestamp >= oneHourAgo)
                .FirstOrDefaultAsync();

            if (existingAlert == null)
            {
                var alert = new Alert
                {
                    DeviceId = device.DeviceId,
                    AlertType = "device_offline",
                    Message = "Device has been offline for more than 1 hour",
                    Timestamp = DateTime.UtcNow,
                    Acknowledged = false
                };

                context.Alerts.Add(alert);
                await context.SaveChangesAsync();

                _logger.LogWarning("Device offline alert for {DeviceId}", device.DeviceId);

                // Broadcast device status change
                await _hubContext.Clients.All.SendAsync("ReceiveDeviceStatus", new
                {
                    deviceId = device.DeviceId,
                    status = "Disconnected"
                });

                // Broadcast alert
                await _hubContext.Clients.All.SendAsync("ReceiveAlert", new
                {
                    id = alert.Id,
                    deviceId = alert.DeviceId,
                    alertType = alert.AlertType,
                    message = alert.Message,
                    timestamp = alert.Timestamp,
                    acknowledged = alert.Acknowledged
                });
            }
        }

        if (offlineDevices.Any())
        {
            await context.SaveChangesAsync();
        }
    }
}
