using System.ComponentModel.DataAnnotations;

namespace IoTDetectorDashboard.Models;

public class Device
{
    [Key]
    public string DeviceId { get; set; } = string.Empty;

    public string DeviceType { get; set; } = string.Empty; // "ESP32", "Photon2"

    public string Status { get; set; } = "Disconnected"; // "Connected", "Disconnected"

    public DateTime? LastConnected { get; set; }

    public DateTime? LastDataReceived { get; set; }

    // Navigation property
    public ICollection<SensorData> SensorDataList { get; set; } = new List<SensorData>();
}
