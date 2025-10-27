using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace IoTDetectorDashboard.Models;

public class SensorData
{
    [Key]
    public int Id { get; set; }

    [Required]
    public string DeviceId { get; set; } = string.Empty;

    public string EventType { get; set; } = string.Empty; // "motion_detected", "heartbeat"

    public int? MotionCount { get; set; }

    public int? Rssi { get; set; } // Signal WiFi

    public long? FreeMemory { get; set; }

    public long? Uptime { get; set; } // Secondes

    public string? FirmwareVersion { get; set; }

    public DateTime Timestamp { get; set; } = DateTime.UtcNow;

    // Foreign key
    [ForeignKey("DeviceId")]
    public Device? Device { get; set; }
}
