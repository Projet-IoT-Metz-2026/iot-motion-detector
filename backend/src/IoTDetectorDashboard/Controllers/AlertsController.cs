using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;

namespace IoTDetectorDashboard.Controllers;

public class AlertsController : Controller
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<AlertsController> _logger;

    public AlertsController(ApplicationDbContext context, ILogger<AlertsController> logger)
    {
        _context = context;
        _logger = logger;
    }

    // GET: Alerts
    public async Task<IActionResult> Index(string? deviceId = null, bool? acknowledged = null)
    {
        var alertsQuery = _context.Alerts.AsQueryable();

        // Filter by device if specified
        if (!string.IsNullOrEmpty(deviceId))
        {
            alertsQuery = alertsQuery.Where(a => a.DeviceId == deviceId);
        }

        // Filter by acknowledgement status if specified
        if (acknowledged.HasValue)
        {
            alertsQuery = alertsQuery.Where(a => a.Acknowledged == acknowledged.Value);
        }

        var alerts = await alertsQuery
            .OrderByDescending(a => a.Timestamp)
            .ToListAsync();

        ViewBag.DeviceId = deviceId;
        ViewBag.Acknowledged = acknowledged;

        return View(alerts);
    }

    // POST: Alerts/Acknowledge/5
    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Acknowledge(int id)
    {
        var alert = await _context.Alerts.FindAsync(id);

        if (alert != null)
        {
            alert.Acknowledged = true;
            alert.AcknowledgedAt = DateTime.UtcNow;
            await _context.SaveChangesAsync();

            _logger.LogInformation("Alert {AlertId} acknowledged", id);
        }

        return RedirectToAction(nameof(Index));
    }

    // POST: Alerts/Delete/5
    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Delete(int id)
    {
        var alert = await _context.Alerts.FindAsync(id);

        if (alert != null)
        {
            _context.Alerts.Remove(alert);
            await _context.SaveChangesAsync();

            _logger.LogInformation("Alert {AlertId} deleted", id);
        }

        return RedirectToAction(nameof(Index));
    }
}
