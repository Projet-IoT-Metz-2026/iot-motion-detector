using Microsoft.AspNetCore.SignalR;

namespace IoTDetectorDashboard.Hubs;

public class SensorHub : Hub
{
    private readonly ILogger<SensorHub> _logger;

    public SensorHub(ILogger<SensorHub> logger)
    {
        _logger = logger;
    }

    /// <summary>
    /// Called when a client connects to the hub
    /// </summary>
    public override async Task OnConnectedAsync()
    {
        _logger.LogInformation("Client connected: {ConnectionId}", Context.ConnectionId);
        await base.OnConnectedAsync();
    }

    /// <summary>
    /// Called when a client disconnects from the hub
    /// </summary>
    public override async Task OnDisconnectedAsync(Exception? exception)
    {
        _logger.LogInformation("Client disconnected: {ConnectionId}", Context.ConnectionId);
        await base.OnDisconnectedAsync(exception);
    }

    /// <summary>
    /// Send sensor data to all connected clients
    /// </summary>
    public async Task SendSensorData(object sensorData)
    {
        await Clients.All.SendAsync("ReceiveSensorData", sensorData);
        _logger.LogDebug("Sensor data broadcasted to all clients");
    }

    /// <summary>
    /// Send alert to all connected clients
    /// </summary>
    public async Task SendAlert(object alert)
    {
        await Clients.All.SendAsync("ReceiveAlert", alert);
        _logger.LogDebug("Alert broadcasted to all clients");
    }

    /// <summary>
    /// Send device status update to all connected clients
    /// </summary>
    public async Task SendDeviceStatus(string deviceId, string status)
    {
        await Clients.All.SendAsync("ReceiveDeviceStatus", new { deviceId, status });
        _logger.LogDebug("Device status update broadcasted: {DeviceId} - {Status}", deviceId, status);
    }

    /// <summary>
    /// Subscribe to specific device updates
    /// </summary>
    public async Task SubscribeToDevice(string deviceId)
    {
        await Groups.AddToGroupAsync(Context.ConnectionId, $"device_{deviceId}");
        _logger.LogInformation("Client {ConnectionId} subscribed to device {DeviceId}", Context.ConnectionId, deviceId);
    }

    /// <summary>
    /// Unsubscribe from specific device updates
    /// </summary>
    public async Task UnsubscribeFromDevice(string deviceId)
    {
        await Groups.RemoveFromGroupAsync(Context.ConnectionId, $"device_{deviceId}");
        _logger.LogInformation("Client {ConnectionId} unsubscribed from device {DeviceId}", Context.ConnectionId, deviceId);
    }

    /// <summary>
    /// Send sensor data to subscribers of a specific device
    /// </summary>
    public async Task SendDeviceSensorData(string deviceId, object sensorData)
    {
        await Clients.Group($"device_{deviceId}").SendAsync("ReceiveSensorData", sensorData);
        _logger.LogDebug("Sensor data sent to subscribers of device {DeviceId}", deviceId);
    }
}
