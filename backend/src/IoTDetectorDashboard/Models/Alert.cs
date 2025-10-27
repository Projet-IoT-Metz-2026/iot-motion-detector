using System.ComponentModel.DataAnnotations;

namespace IoTDetectorDashboard.Models;

public class Alert
{
    [Key]
    public int Id { get; set; }

    [Required]
    public string DeviceId { get; set; } = string.Empty;

    public string AlertType { get; set; } = string.Empty; // "MotionDetected", "LowBattery", "Disconnected"

    public string Message { get; set; } = string.Empty;

    public DateTime Timestamp { get; set; } = DateTime.UtcNow;

    public bool Acknowledged { get; set; } = false;

    public DateTime? AcknowledgedAt { get; set; }
}
