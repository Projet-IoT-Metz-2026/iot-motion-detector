using Azure.Messaging.EventHubs;
using Azure.Messaging.EventHubs.Consumer;
using Microsoft.EntityFrameworkCore;
using System.Text;
using System.Text.Json;
using IoTDetectorApi.Data;
using IoTDetectorApi.Models;

namespace IoTDetectorApi.Services;

public class IoTHubListenerService : BackgroundService
{
    private readonly IServiceProvider _serviceProvider;
    private readonly ILogger<IoTHubListenerService> _logger;
    private readonly IConfiguration _configuration;

    public IoTHubListenerService(
        IServiceProvider serviceProvider,
        ILogger<IoTHubListenerService> logger,
        IConfiguration configuration)
    {
        _serviceProvider = serviceProvider;
        _logger = logger;
        _configuration = configuration;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var enabled = _configuration.GetValue<bool>("Azure:IoTHub:Enabled");
        if (!enabled)
        {
            _logger.LogInformation("Azure IoT Hub listener is disabled");
            return;
        }

        var eventHubConnectionString = _configuration.GetValue<string>("Azure:IoTHub:EventHubConnectionString");
        var consumerGroup = _configuration.GetValue<string>("Azure:IoTHub:ConsumerGroup") ?? EventHubConsumerClient.DefaultConsumerGroupName;

        if (string.IsNullOrEmpty(eventHubConnectionString))
        {
            _logger.LogWarning("Azure Event Hub connection string not configured");
            return;
        }

        _logger.LogInformation("Starting Azure IoT Hub listener...");

        try
        {
            // Create Event Hub consumer client directly with Event Hub connection string
            var consumer = new EventHubConsumerClient(
                consumerGroup,
                eventHubConnectionString);

            _logger.LogInformation("Connected to Azure IoT Hub Event Hub. Consumer Group: {ConsumerGroup}", consumerGroup);

            // Create log entry
            await LogEventAsync("Azure IoT Hub", "Connexion établie avec Azure IoT Hub", "info");

            // Read events from all partitions
            await foreach (PartitionEvent partitionEvent in consumer.ReadEventsAsync(stoppingToken))
            {
                if (partitionEvent.Data == null)
                    continue;

                try
                {
                    await ProcessEventAsync(partitionEvent.Data);
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error processing IoT Hub event");
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Fatal error in IoT Hub listener");
            await LogEventAsync("Azure IoT Hub", $"Erreur critique: {ex.Message}", "error");
        }
    }

    private async Task ProcessEventAsync(EventData eventData)
    {
        using var scope = _serviceProvider.CreateScope();
        var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();

        // Get device ID from system properties
        var deviceId = eventData.SystemProperties.ContainsKey("iothub-connection-device-id")
            ? eventData.SystemProperties["iothub-connection-device-id"].ToString()
            : "unknown";

        // Parse message body
        var messageBody = Encoding.UTF8.GetString(eventData.EventBody.ToArray());
        _logger.LogInformation("Received message from {DeviceId}: {Message}", deviceId, messageBody);

        JsonDocument? jsonDoc = null;
        try
        {
            jsonDoc = JsonDocument.Parse(messageBody);
        }
        catch (JsonException ex)
        {
            _logger.LogWarning(ex, "Failed to parse message as JSON: {Message}", messageBody);
            return;
        }

        var root = jsonDoc.RootElement;

        // Update device LastSeen
        var device = await context.Devices.FindAsync(deviceId);
        if (device != null)
        {
            device.LastSeen = DateTime.UtcNow;
            device.Status = "active";
            await context.SaveChangesAsync();
        }
        else
        {
            // Create device if it doesn't exist
            _logger.LogInformation("Creating new device: {DeviceId}", deviceId);
            device = new Device
            {
                Id = deviceId,
                Name = deviceId,
                Type = deviceId.Contains("esp32") ? "ESP32" : deviceId.Contains("photon") ? "Photon2" : "Unknown",
                Status = "active",
                LastSeen = DateTime.UtcNow,
                CreatedAt = DateTime.UtcNow
            };
            context.Devices.Add(device);
            await context.SaveChangesAsync();

            await LogEventAsync(deviceId, $"Nouveau device détecté: {deviceId}", "info");
        }

        // Determine event type and create sensor data entry
        var eventType = root.TryGetProperty("event", out var eventProp)
            ? eventProp.GetString()
            : "unknown";

        var sensorData = new SensorData
        {
            DeviceId = deviceId,
            EventType = eventType ?? "unknown",
            Timestamp = DateTime.UtcNow,
            RawData = messageBody
        };

        // Parse specific fields based on event type
        if (eventType == "motion")
        {
            sensorData.Value = 1;
            if (root.TryGetProperty("count", out var countProp))
            {
                sensorData.Value = countProp.GetInt32();
            }

            // Create event log for motion detection
            await LogEventAsync(deviceId, $"Détection PIR - {deviceId}", "info");
        }

        context.SensorData.Add(sensorData);
        await context.SaveChangesAsync();

        _logger.LogInformation("Saved sensor data: {DeviceId} - {EventType}", deviceId, eventType);
    }

    private async Task LogEventAsync(string? deviceId, string message, string level)
    {
        try
        {
            using var scope = _serviceProvider.CreateScope();
            var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();

            var eventLog = new EventLog
            {
                DeviceId = deviceId,
                Message = message,
                Level = level,
                Timestamp = DateTime.UtcNow
            };

            context.EventLogs.Add(eventLog);
            await context.SaveChangesAsync();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to log event");
        }
    }

    public override async Task StopAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("Stopping Azure IoT Hub listener...");
        await base.StopAsync(stoppingToken);
    }
}
