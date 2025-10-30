namespace IoTDetectorApi.Models;

public class Device
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty; // ESP32, Photon2
    public string Status { get; set; } = "inactive"; // active, inactive
    public DateTime? LastSeen { get; set; }
    public string? LastSignal { get; set; }
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}
