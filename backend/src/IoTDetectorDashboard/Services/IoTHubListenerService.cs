using Azure.Messaging.EventHubs.Consumer;
using Azure.Messaging.EventHubs;
using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;
using IoTDetectorDashboard.Hubs;
using Microsoft.AspNetCore.SignalR;
using System.Text;
using System.Text.Json;

namespace IoTDetectorDashboard.Services;

public class IoTHubListenerService : BackgroundService
{
    private readonly IServiceProvider _serviceProvider;
    private readonly IHubContext<SensorHub> _hubContext;
    private readonly ILogger<IoTHubListenerService> _logger;
    private readonly IConfiguration _configuration;
    private EventHubConsumerClient? _consumerClient;

    public IoTHubListenerService(
        IServiceProvider serviceProvider,
        IHubContext<SensorHub> hubContext,
        ILogger<IoTHubListenerService> logger,
        IConfiguration configuration)
    {
        _serviceProvider = serviceProvider;
        _hubContext = hubContext;
        _logger = logger;
        _configuration = configuration;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("IoT Hub Listener Service starting...");

        // Check if IoT Hub is enabled
        var iotHubEnabled = _configuration.GetValue<bool>("Azure:IoTHub:Enabled", false);
        if (!iotHubEnabled)
        {
            _logger.LogInformation("Azure IoT Hub listener is disabled in configuration");
            return;
        }

        // Get IoT Hub connection string
        var connectionString = _configuration["Azure:IoTHub:ConnectionString"];

        if (string.IsNullOrEmpty(connectionString) || connectionString.Contains("YOUR_KEY_HERE"))
        {
            _logger.LogWarning("Azure IoT Hub connection string not configured. Service will not listen for events.");
            _logger.LogInformation("To enable IoT Hub listener, update appsettings.json with your IoT Hub connection string");
            return;
        }

        try
        {
            // Convert IoT Hub connection string to Event Hub compatible format
            var eventHubConnectionString = ConvertToEventHubConnectionString(connectionString);
            var eventHubName = "messages/events"; // Default IoT Hub built-in endpoint

            _consumerClient = new EventHubConsumerClient(
                EventHubConsumerClient.DefaultConsumerGroupName,
                eventHubConnectionString,
                eventHubName);

            _logger.LogInformation("Connected to IoT Hub Event Hub endpoint");

            // Start reading events from all partitions
            await foreach (PartitionEvent partitionEvent in _consumerClient.ReadEventsAsync(stoppingToken))
            {
                try
                {
                    await ProcessEventAsync(partitionEvent);
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error processing event from partition {PartitionId}",
                        partitionEvent.Partition.PartitionId);
                }
            }
        }
        catch (TaskCanceledException)
        {
            _logger.LogInformation("IoT Hub Listener Service is stopping...");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in IoT Hub Listener Service");
            throw;
        }
    }

    private async Task ProcessEventAsync(PartitionEvent partitionEvent)
    {
        if (partitionEvent.Data == null) return;

        var messageBody = Encoding.UTF8.GetString(partitionEvent.Data.EventBody.ToArray());
        var deviceId = partitionEvent.Data.SystemProperties.ContainsKey("iothub-connection-device-id")
            ? partitionEvent.Data.SystemProperties["iothub-connection-device-id"]?.ToString()
            : "unknown";

        _logger.LogInformation("Received message from device {DeviceId}: {Message}", deviceId, messageBody);

        try
        {
            // Parse telemetry message
            var telemetry = JsonSerializer.Deserialize<TelemetryMessage>(messageBody,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

            if (telemetry == null || string.IsNullOrEmpty(deviceId))
            {
                _logger.LogWarning("Invalid telemetry message or device ID");
                return;
            }

            // Process in a new scope to get scoped services
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
                    DeviceType = DetermineDeviceType(deviceId),
                    Status = "Connected",
                    LastConnected = DateTime.UtcNow,
                    LastDataReceived = DateTime.UtcNow
                };
                context.Devices.Add(device);
                _logger.LogInformation("New device registered: {DeviceId}", deviceId);
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
                EventType = telemetry.EventType ?? "unknown",
                MotionCount = telemetry.MotionCount,
                Rssi = telemetry.Rssi,
                FreeMemory = telemetry.FreeMemory,
                Uptime = telemetry.Uptime,
                FirmwareVersion = telemetry.FirmwareVersion,
                Timestamp = DateTime.UtcNow
            };

            context.SensorData.Add(sensorData);
            await context.SaveChangesAsync();

            _logger.LogInformation("Sensor data saved: {DeviceId} - {EventType}", deviceId, sensorData.EventType);

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
                timestamp = sensorData.Timestamp
            });

            _logger.LogDebug("Sensor data broadcasted via SignalR");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing telemetry from device {DeviceId}", deviceId);
        }
    }

    private string ConvertToEventHubConnectionString(string iotHubConnectionString)
    {
        // IoT Hub connection string format: HostName=xxx;SharedAccessKeyName=xxx;SharedAccessKey=xxx
        // Event Hub needs: Endpoint=sb://xxx;SharedAccessKeyName=xxx;SharedAccessKey=xxx;EntityPath=xxx

        var parts = iotHubConnectionString.Split(';');
        var hostName = parts.FirstOrDefault(p => p.StartsWith("HostName="))?.Replace("HostName=", "");
        var keyName = parts.FirstOrDefault(p => p.StartsWith("SharedAccessKeyName="));
        var key = parts.FirstOrDefault(p => p.StartsWith("SharedAccessKey="));

        if (string.IsNullOrEmpty(hostName) || string.IsNullOrEmpty(keyName) || string.IsNullOrEmpty(key))
        {
            throw new InvalidOperationException("Invalid IoT Hub connection string format");
        }

        return $"Endpoint=sb://{hostName};{keyName};{key}";
    }

    private string DetermineDeviceType(string deviceId)
    {
        if (deviceId.Contains("ESP32", StringComparison.OrdinalIgnoreCase))
            return "ESP32";
        else if (deviceId.Contains("Photon", StringComparison.OrdinalIgnoreCase))
            return "Photon2";
        else
            return "Unknown";
    }

    public override async Task StopAsync(CancellationToken cancellationToken)
    {
        _logger.LogInformation("IoT Hub Listener Service stopping...");

        if (_consumerClient != null)
        {
            await _consumerClient.CloseAsync(cancellationToken);
            await _consumerClient.DisposeAsync();
        }

        await base.StopAsync(cancellationToken);
    }
}

// Telemetry message model
public class TelemetryMessage
{
    public string? EventType { get; set; }
    public int? MotionCount { get; set; }
    public int? Rssi { get; set; }
    public long? FreeMemory { get; set; }
    public long? Uptime { get; set; }
    public string? FirmwareVersion { get; set; }
}
