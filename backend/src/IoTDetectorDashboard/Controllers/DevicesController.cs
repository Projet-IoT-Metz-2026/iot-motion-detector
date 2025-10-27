using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Models;

namespace IoTDetectorDashboard.Controllers;

public class DevicesController : Controller
{
    private readonly ApplicationDbContext _context;
    private readonly ILogger<DevicesController> _logger;

    public DevicesController(ApplicationDbContext context, ILogger<DevicesController> logger)
    {
        _context = context;
        _logger = logger;
    }

    // GET: Devices
    public async Task<IActionResult> Index()
    {
        var devices = await _context.Devices
            .OrderByDescending(d => d.LastDataReceived)
            .ToListAsync();

        return View(devices);
    }

    // GET: Devices/Details/esp32-pir-01
    public async Task<IActionResult> Details(string id)
    {
        if (string.IsNullOrEmpty(id))
        {
            return NotFound();
        }

        var device = await _context.Devices
            .Include(d => d.SensorDataList.OrderByDescending(s => s.Timestamp).Take(100))
            .FirstOrDefaultAsync(d => d.DeviceId == id);

        if (device == null)
        {
            return NotFound();
        }

        return View(device);
    }

    // POST: Devices/Delete/esp32-pir-01
    [HttpPost]
    [ValidateAntiForgeryToken]
    public async Task<IActionResult> Delete(string id)
    {
        var device = await _context.Devices.FindAsync(id);
        if (device != null)
        {
            _context.Devices.Remove(device);
            await _context.SaveChangesAsync();
            _logger.LogInformation("Device {DeviceId} deleted", id);
        }

        return RedirectToAction(nameof(Index));
    }
}
