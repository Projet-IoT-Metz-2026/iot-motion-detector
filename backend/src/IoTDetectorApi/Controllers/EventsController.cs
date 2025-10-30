using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorApi.Data;
using IoTDetectorApi.Models;

namespace IoTDetectorApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class EventsController : ControllerBase
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<EventsController> _logger;

    public EventsController(ApplicationDbContext context, ILogger<EventsController> logger)
    {
        _context = context;
        _logger = logger;
    }

    [HttpGet("recent")]
    public async Task<ActionResult<IEnumerable<EventLog>>> GetRecentEvents([FromQuery] int limit = 50)
    {
        var events = await _context.EventLogs
            .OrderByDescending(e => e.Timestamp)
            .Take(limit)
            .ToListAsync();

        return Ok(events);
    }

    [HttpGet]
    public async Task<ActionResult<IEnumerable<EventLog>>> GetEvents(
        [FromQuery] string? level = null,
        [FromQuery] int skip = 0,
        [FromQuery] int take = 100)
    {
        var query = _context.EventLogs.AsQueryable();

        if (!string.IsNullOrEmpty(level))
        {
            query = query.Where(e => e.Level == level);
        }

        var events = await query
            .OrderByDescending(e => e.Timestamp)
            .Skip(skip)
            .Take(take)
            .ToListAsync();

        return Ok(events);
    }
}
