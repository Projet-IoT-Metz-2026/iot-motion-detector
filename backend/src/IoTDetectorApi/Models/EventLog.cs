namespace IoTDetectorApi.Models;

public class EventLog
{
    public int Id { get; set; }
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public string Message { get; set; } = string.Empty;
    public string Level { get; set; } = "info"; // info, warning, error
    public string? DeviceId { get; set; }
}
