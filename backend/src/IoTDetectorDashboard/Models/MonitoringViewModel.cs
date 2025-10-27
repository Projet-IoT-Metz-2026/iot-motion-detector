namespace IoTDetectorDashboard.Models;

public class MonitoringViewModel
{
    public List<SystemStatusItem> SystemStatus { get; set; } = new();
    public List<MessageRateData> MessageRateData { get; set; } = new();
    public List<LatencyData> LatencyData { get; set; } = new();
    public List<ProvisioningLog> ProvisioningLogs { get; set; } = new();
}

public class SystemStatusItem
{
    public string Component { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty; // ok, warning, error
    public string Message { get; set; } = string.Empty;
}

public class MessageRateData
{
    public string Time { get; set; } = string.Empty;
    public int Messages { get; set; }
    public int Errors { get; set; }
}

public class LatencyData
{
    public string Time { get; set; } = string.Empty;
    public int Latency { get; set; }
}

public class ProvisioningLog
{
    public string Device { get; set; } = string.Empty;
    public string Status { get; set; } = string.Empty;
    public string Timestamp { get; set; } = string.Empty;
    public string Duration { get; set; } = string.Empty;
}
