using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorApi.Data;
using IoTDetectorApi.Models;

namespace IoTDetectorApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class DevicesController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<DevicesController> _logger;

    public DevicesController(ApplicationDbContext context, ILogger<DevicesController> logger)
    {
        _context = context;
        _logger = logger;
    }

    [HttpGet]
    public async Task<ActionResult<IEnumerable<Device>>> GetDevices()
    {
        var devices = await _context.Devices.ToListAsync();

        // Update last signal text
        foreach (var device in devices)
        {
            if (device.LastSeen.HasValue)
            {
                var timeAgo = DateTime.UtcNow - device.LastSeen.Value;
                device.LastSignal = timeAgo.TotalMinutes < 1
                    ? "Il y a moins d'1 min"
                    : timeAgo.TotalMinutes < 60
                        ? $"Il y a {(int)timeAgo.TotalMinutes} min"
                        : timeAgo.TotalHours < 24
                            ? $"Il y a {(int)timeAgo.TotalHours} h"
                            : $"Il y a {(int)timeAgo.TotalDays} j";
            }
            else
            {
                device.LastSignal = "Jamais vu";
            }
        }

        return Ok(devices);
    }

    [HttpGet("{id}")]
    public async Task<ActionResult<Device>> GetDevice(string id)
    {
        var device = await _context.Devices.FindAsync(id);

        if (device == null)
        {
            return NotFound();
        }

        return Ok(device);
    }

    [HttpGet("stats")]
    public async Task<ActionResult<object>> GetStats()
    {
        var totalDevices = await _context.Devices.CountAsync();
        var activeDevices = await _context.Devices.CountAsync(d => d.Status == "active");

        var today = DateTime.UtcNow.Date;
        var eventsToday = await _context.SensorData.CountAsync(s => s.Timestamp >= today);

        var activeAlerts = await _context.EventLogs.CountAsync(e =>
            e.Level == "warning" || e.Level == "error");

        var deviceTypes = await _context.Devices
            .GroupBy(d => d.Type)
            .Select(g => new { Type = g.Key, Count = g.Count() })
            .ToListAsync();

        var typeBreakdown = string.Join(", ", deviceTypes.Select(t => $"{t.Count} {t.Type}"));

        return Ok(new
        {
            totalDevices,
            activeDevices,
            eventsToday,
            activeAlerts,
            availability = totalDevices > 0 ? (int)((double)activeDevices / totalDevices * 100) : 0,
            deviceTypeBreakdown = typeBreakdown
        });
    }
}
