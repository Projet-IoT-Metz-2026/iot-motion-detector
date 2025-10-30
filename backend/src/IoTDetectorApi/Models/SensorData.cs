namespace IoTDetectorApi.Models;

public class SensorData
{
    public int Id { get; set; }
    public string DeviceId { get; set; } = string.Empty;
    public string EventType { get; set; } = string.Empty; // motion, heartbeat, etc.
    public double? Value { get; set; }
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public string? RawData { get; set; }
}
