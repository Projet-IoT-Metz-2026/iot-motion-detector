using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;

namespace IoTDetectorDashboard.Services;

public static class DatabaseSeeder
{
    public static void SeedDatabase(ApplicationDbContext context, ILogger logger)
    {
        if (context.Devices.Any())
        {
            logger.LogInformation("Database already contains data. Skipping seeding.");
            return;
        }

        logger.LogInformation("Seeding database with sample data...");

        // Add sample devices
        var devices = new[]
        {
            new Device
            {
                DeviceId = "ESP32-PIR-001",
                DeviceType = "ESP32",
                Status = "Connected",
                LastConnected = DateTime.UtcNow.AddMinutes(-5),
                LastDataReceived = DateTime.UtcNow.AddMinutes(-2)
            },
            new Device
            {
                DeviceId = "Photon2-PIR-001",
                DeviceType = "Photon2",
                Status = "Connected",
                LastConnected = DateTime.UtcNow.AddMinutes(-10),
                LastDataReceived = DateTime.UtcNow.AddMinutes(-3)
            },
            new Device
            {
                DeviceId = "ESP32-PIR-002",
                DeviceType = "ESP32",
                Status = "Disconnected",
                LastConnected = DateTime.UtcNow.AddHours(-2),
                LastDataReceived = DateTime.UtcNow.AddHours(-2)
            }
        };
        context.Devices.AddRange(devices);
        logger.LogInformation("Added {Count} devices", devices.Length);

        // Add sample sensor data
        var sensorData = new List<SensorData>();
        var random = new Random(42); // Fixed seed for reproducible data

        for (int i = 0; i < 50; i++)
        {
            var deviceId = i % 3 == 0 ? "ESP32-PIR-001" : "Photon2-PIR-001";
            var timestamp = DateTime.UtcNow.AddHours(-24 + i * 0.5);
            var isMotion = i % 5 == 0;

            sensorData.Add(new SensorData
            {
                DeviceId = deviceId,
                EventType = isMotion ? "motion_detected" : "heartbeat",
                MotionCount = isMotion ? random.Next(1, 10) : null,
                Rssi = -60 - random.Next(0, 30),
                FreeMemory = 200000 + random.Next(-50000, 50000),
                Uptime = 3600 * (i + 1),
                FirmwareVersion = "v3.0.0",
                Timestamp = timestamp
            });
        }

        context.SensorData.AddRange(sensorData);
        logger.LogInformation("Added {Count} sensor data entries", sensorData.Count);

        // Add sample alerts
        var alerts = new[]
        {
            new Alert
            {
                DeviceId = "ESP32-PIR-001",
                AlertType = "motion_threshold",
                Message = "High motion activity detected (10 detections in 5 minutes)",
                Timestamp = DateTime.UtcNow.AddHours(-1),
                Acknowledged = false
            },
            new Alert
            {
                DeviceId = "ESP32-PIR-002",
                AlertType = "device_offline",
                Message = "Device has been offline for more than 1 hour",
                Timestamp = DateTime.UtcNow.AddHours(-2),
                Acknowledged = false
            },
            new Alert
            {
                DeviceId = "Photon2-PIR-001",
                AlertType = "weak_signal",
                Message = "Signal strength below -80 dBm",
                Timestamp = DateTime.UtcNow.AddHours(-3),
                Acknowledged = true,
                AcknowledgedAt = DateTime.UtcNow.AddHours(-2)
            }
        };
        context.Alerts.AddRange(alerts);
        logger.LogInformation("Added {Count} alerts", alerts.Length);

        context.SaveChanges();
        logger.LogInformation("Database seeding completed successfully!");
    }
}
