using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;
using IoTDetectorDashboard.Hubs;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Shared;
using Azure.Messaging.EventHubs.Consumer;
using System.Text;
using System.Text.Json;
using System.Net;

namespace IoTDetectorDashboard.Services;

public class IoTHubListenerService : BackgroundService
{
    private readonly IServiceProvider _serviceProvider;
    private readonly IHubContext<SensorHub> _hubContext;
    private readonly ILogger<IoTHubListenerService> _logger;
    private readonly IConfiguration _configuration;
    private RegistryManager? _registryManager;
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
        _logger.LogInformation("IoT Hub Device Registry Service starting...");

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
            _logger.LogWarning("Azure IoT Hub connection string not configured. Service will not sync devices.");
            _logger.LogInformation("To enable IoT Hub sync, update appsettings.json with your IoT Hub connection string");
            return;
        }

        try
        {
            // Create Registry Manager to access IoT Hub device registry
            _registryManager = RegistryManager.CreateFromConnectionString(connectionString);
            _logger.LogInformation("Connected to IoT Hub Registry");

            // Create EventHub Consumer to receive telemetry messages
            var eventHubEndpoint = _configuration["Azure:IoTHub:EventHubCompatibleEndpoint"];
            var eventHubPath = _configuration["Azure:IoTHub:EventHubCompatiblePath"];

            if (!string.IsNullOrEmpty(eventHubEndpoint) && !string.IsNullOrEmpty(eventHubPath))
            {
                // Use Event Hub compatible endpoint from configuration
                var eventHubConnectionString = BuildEventHubConnectionString(connectionString, eventHubEndpoint, eventHubPath);
                _consumerClient = new EventHubConsumerClient(
                    EventHubConsumerClient.DefaultConsumerGroupName,
                    eventHubConnectionString);
                _logger.LogInformation("Connected to IoT Hub Event Hub endpoint using compatible endpoint");
            }
            else
            {
                _logger.LogWarning("Event Hub compatible endpoint not configured. Telemetry reception disabled.");
                _logger.LogInformation("To enable telemetry reception, add EventHubCompatibleEndpoint and EventHubCompatiblePath to appsettings.json");
            }

            // Start two parallel tasks:
            // 1. Sync devices periodically
            // 2. Receive telemetry events continuously (if EventHub consumer is configured)
            var tasks = new List<Task> { SyncDevicesPeriodicallyAsync(stoppingToken) };
            if (_consumerClient != null)
            {
                tasks.Add(ReceiveTelemetryAsync(stoppingToken));
            }

            await Task.WhenAll(tasks);
        }
        catch (TaskCanceledException)
        {
            _logger.LogInformation("IoT Hub Listener Service is stopping...");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error in IoT Hub Listener Service");
        }
    }

    private async Task SyncDevicesFromIoTHubAsync()
    {
        if (_registryManager == null) return;

        try
        {
            // Get all devices from IoT Hub
            var query = _registryManager.CreateQuery("SELECT * FROM devices");
            var iotHubDevices = new List<Twin>();

            while (query.HasMoreResults)
            {
                var twins = await query.GetNextAsTwinAsync();
                iotHubDevices.AddRange(twins);
            }

            _logger.LogInformation("Found {Count} devices in IoT Hub", iotHubDevices.Count);

            // Sync with local database
            using var scope = _serviceProvider.CreateScope();
            var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();

            foreach (var twin in iotHubDevices)
            {
                var deviceId = twin.DeviceId;
                var connectionState = twin.ConnectionState.ToString();

                // Check if device exists in local DB
                var device = await context.Devices.FindAsync(deviceId);
                var localTime = GetLocalTime();
                var lastActivityTime = twin.LastActivityTime ?? DateTime.UtcNow;
                var lastActivityLocal = TimeZoneInfo.ConvertTimeFromUtc(lastActivityTime, TimeZoneInfo.FindSystemTimeZoneById(_configuration["TimeZone"] ?? "UTC"));

                if (device == null)
                {
                    // Create new device
                    device = new Models.Device
                    {
                        DeviceId = deviceId,
                        DeviceType = DetermineDeviceType(deviceId),
                        Status = connectionState,
                        LastConnected = lastActivityLocal,
                        LastDataReceived = lastActivityLocal
                    };
                    context.Devices.Add(device);
                    _logger.LogInformation("New device registered from IoT Hub: {DeviceId}", deviceId);
                }
                else
                {
                    // Update existing device
                    device.Status = connectionState;
                    if (twin.LastActivityTime.HasValue)
                    {
                        device.LastConnected = lastActivityLocal;
                    }
                }
            }

            await context.SaveChangesAsync();
            _logger.LogDebug("Device sync completed");

            // Broadcast devices update via SignalR
            await _hubContext.Clients.All.SendAsync("DevicesUpdated", iotHubDevices.Count);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error syncing devices from IoT Hub");
            throw;
        }
    }

    private async Task SyncDevicesPeriodicallyAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await SyncDevicesFromIoTHubAsync();
                await Task.Delay(TimeSpan.FromSeconds(30), stoppingToken);
            }
            catch (TaskCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error syncing devices from IoT Hub");
                await Task.Delay(TimeSpan.FromMinutes(1), stoppingToken);
            }
        }
    }

    private async Task ReceiveTelemetryAsync(CancellationToken stoppingToken)
    {
        if (_consumerClient == null) return;

        try
        {
            await foreach (var partitionEvent in _consumerClient.ReadEventsAsync(stoppingToken))
            {
                try
                {
                    if (partitionEvent.Data == null) continue;

                    var deviceId = partitionEvent.Data.SystemProperties.ContainsKey("iothub-connection-device-id")
                        ? partitionEvent.Data.SystemProperties["iothub-connection-device-id"].ToString()
                        : "Unknown";

                    var messageBody = Encoding.UTF8.GetString(partitionEvent.Data.Body.ToArray());
                    _logger.LogInformation("Received telemetry from {DeviceId}: {Message}", deviceId, messageBody);

                    // Parse and process telemetry
                    await ProcessTelemetryMessageAsync(deviceId, messageBody);
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Error processing telemetry event");
                }
            }
        }
        catch (TaskCanceledException)
        {
            _logger.LogInformation("Telemetry reception cancelled");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error receiving telemetry from IoT Hub");
        }
    }

    private async Task ProcessTelemetryMessageAsync(string? deviceId, string messageBody)
    {
        if (string.IsNullOrEmpty(deviceId)) return;

        try
        {
            using var scope = _serviceProvider.CreateScope();
            var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();

            var device = await context.Devices.FindAsync(deviceId);
            if (device == null)
            {
                _logger.LogWarning("Received telemetry from unknown device: {DeviceId}", deviceId);
                return;
            }

            // Parse outer JSON envelope
            var envelope = JsonSerializer.Deserialize<JsonElement>(messageBody);

            // Extract and parse the inner "data" field which contains the actual telemetry
            JsonElement telemetry;
            if (envelope.TryGetProperty("data", out var dataField))
            {
                // The "data" field is a JSON string with HTML-encoded entities, decode and parse it
                var dataString = dataField.GetString();
                if (string.IsNullOrEmpty(dataString))
                {
                    _logger.LogWarning("Empty data field in telemetry from {DeviceId}", deviceId);
                    return;
                }

                // Decode HTML entities (&quot; -> ")
                var decodedDataString = WebUtility.HtmlDecode(dataString);
                telemetry = JsonSerializer.Deserialize<JsonElement>(decodedDataString);
            }
            else
            {
                // No "data" field, use the envelope directly
                telemetry = envelope;
            }

            // Get local time based on configured timezone
            var localTime = GetLocalTime();

            // Create sensor data entry using correct field names
            var sensorData = new SensorData
            {
                DeviceId = deviceId,
                Timestamp = localTime,
                EventType = telemetry.TryGetProperty("event", out var eventType) ? eventType.GetString() : "motion",
                MotionCount = telemetry.TryGetProperty("count", out var count) ? count.GetInt32() : (int?)null,
                Rssi = telemetry.TryGetProperty("rssi", out var rssi) ? rssi.GetInt32() : (int?)null,
                FreeMemory = telemetry.TryGetProperty("freeMemory", out var freeMemory) ? freeMemory.GetInt64() : (long?)null,
                Uptime = telemetry.TryGetProperty("uptime", out var uptime) ? uptime.GetInt64() : (long?)null,
                FirmwareVersion = telemetry.TryGetProperty("firmware_version", out var firmware) ? firmware.GetString() : null
            };

            context.SensorData.Add(sensorData);

            // Update device
            device.LastDataReceived = localTime;
            device.Status = "Connected";

            await context.SaveChangesAsync();

            // Notify SignalR clients
            await _hubContext.Clients.All.SendAsync("SensorDataReceived", new
            {
                deviceId = device.DeviceId,
                timestamp = sensorData.Timestamp,
                eventType = sensorData.EventType,
                motionCount = sensorData.MotionCount
            });

            _logger.LogInformation("Telemetry processed for {DeviceId}: event={Event}, count={Count}",
                deviceId, sensorData.EventType, sensorData.MotionCount);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing telemetry message for device {DeviceId}", deviceId);
        }
    }

    private string BuildEventHubConnectionString(string iotHubConnectionString, string eventHubEndpoint, string eventHubPath)
    {
        // Parse IoT Hub connection string to extract credentials
        var parts = iotHubConnectionString.Split(';');
        var sharedAccessKeyName = "";
        var sharedAccessKey = "";

        foreach (var part in parts)
        {
            if (part.StartsWith("SharedAccessKeyName="))
                sharedAccessKeyName = part.Substring("SharedAccessKeyName=".Length);
            else if (part.StartsWith("SharedAccessKey="))
                sharedAccessKey = part.Substring("SharedAccessKey=".Length);
        }

        // Build Event Hub compatible connection string with Event Hub compatible endpoint
        return $"Endpoint={eventHubEndpoint};SharedAccessKeyName={sharedAccessKeyName};SharedAccessKey={sharedAccessKey};EntityPath={eventHubPath}";
    }

    private DateTime GetLocalTime()
    {
        var timeZoneId = _configuration["TimeZone"] ?? "UTC";
        try
        {
            var timeZone = TimeZoneInfo.FindSystemTimeZoneById(timeZoneId);
            return TimeZoneInfo.ConvertTimeFromUtc(DateTime.UtcNow, timeZone);
        }
        catch
        {
            _logger.LogWarning("Invalid timezone '{TimeZone}', using UTC", timeZoneId);
            return DateTime.UtcNow;
        }
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

        if (_registryManager != null)
        {
            await _registryManager.CloseAsync();
            _registryManager.Dispose();
        }

        await base.StopAsync(cancellationToken);
    }
}
