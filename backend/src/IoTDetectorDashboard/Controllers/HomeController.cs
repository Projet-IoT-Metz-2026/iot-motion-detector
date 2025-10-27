using System.Diagnostics;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using IoTDetectorDashboard.Models;
using IoTDetectorDashboard.Data;

namespace IoTDetectorDashboard.Controllers;

public class HomeController : Controller
{
    private readonly ILogger<HomeController> _logger;
    private readonly ApplicationDbContext _context;

    public HomeController(ILogger<HomeController> logger, ApplicationDbContext context)
    {
        _logger = logger;
        _context = context;
    }

    public async Task<IActionResult> Index()
    {
        // Dashboard statistics
        ViewBag.TotalDevices = await _context.Devices.CountAsync();
        ViewBag.ActiveDevices = await _context.Devices.CountAsync(d => d.Status == "Connected");
        ViewBag.TotalAlerts = await _context.Alerts.CountAsync(a => !a.Acknowledged);
        ViewBag.TotalDetections = await _context.SensorData.CountAsync(s => s.EventType == "motion_detected");

        // Recent sensor data (last 24 hours)
        var recentData = await _context.SensorData
            .Where(s => s.Timestamp >= DateTime.UtcNow.AddHours(-24))
            .OrderByDescending(s => s.Timestamp)
            .Take(50)
            .ToListAsync();

        return View(recentData);
    }

    public IActionResult Privacy()
    {
        return View();
    }

    [ResponseCache(Duration = 0, Location = ResponseCacheLocation.None, NoStore = true)]
    public IActionResult Error()
    {
        return View(new ErrorViewModel { RequestId = Activity.Current?.Id ?? HttpContext.TraceIdentifier });
    }
}
