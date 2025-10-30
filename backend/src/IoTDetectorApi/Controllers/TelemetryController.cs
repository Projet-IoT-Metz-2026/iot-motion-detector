using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorApi.Data;

namespace IoTDetectorApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class TelemetryController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<TelemetryController> _logger;

    public TelemetryController(ApplicationDbContext context, ILogger<TelemetryController> logger)
    {
        _context = context;
        _logger = logger;
    }

    [HttpGet("activity")]
    public async Task<ActionResult<object>> GetActivityData([FromQuery] int hours = 24)
    {
        var startTime = DateTime.UtcNow.AddHours(-hours);

        var data = await _context.SensorData
            .Where(s => s.Timestamp >= startTime && s.EventType == "motion")
            .OrderBy(s => s.Timestamp)
            .ToListAsync();

        // Group by hour
        var grouped = data
            .GroupBy(s => new DateTime(s.Timestamp.Year, s.Timestamp.Month, s.Timestamp.Day, s.Timestamp.Hour, 0, 0))
            .Select(g => new
            {
                time = g.Key.ToString("HH:mm"),
                value = g.Count()
            })
            .ToList();

        return Ok(grouped);
    }

    [HttpGet("recent")]
    public async Task<ActionResult<object>> GetRecentData([FromQuery] int limit = 100)
    {
        var data = await _context.SensorData
            .OrderByDescending(s => s.Timestamp)
            .Take(limit)
            .ToListAsync();

        return Ok(data);
    }
}
