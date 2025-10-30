using Microsoft.EntityFrameworkCore;
using IoTDetectorApi.Data;
using IoTDetectorApi.Models;
using IoTDetectorApi.Services;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddOpenApi();

// Add background services
builder.Services.AddHostedService<IoTHubListenerService>();

// Add CORS
builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowFrontend", policy =>
    {
        policy.WithOrigins("http://localhost:5173", "http://localhost:5174", "http://localhost:3000", "http://localhost:3001")
              .AllowAnyMethod()
              .AllowAnyHeader()
              .AllowCredentials();
    });
});

// Add DbContext with InMemory database
builder.Services.AddDbContext<ApplicationDbContext>(options =>
    options.UseInMemoryDatabase("IoTDetectorDb"));

var app = builder.Build();

// Seed database with sample data
using (var scope = app.Services.CreateScope())
{
    var context = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();
    SeedData(context);
}

// Configure the HTTP request pipeline
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseCors("AllowFrontend");

app.UseAuthorization();

app.MapControllers();

app.Run();

void SeedData(ApplicationDbContext context)
{
    if (context.Devices.Any())
    {
        return; // Already seeded
    }

    // Add devices
    var devices = new[]
    {
        new Device
        {
            Id = "esp32-pir-01",
            Name = "ESP32-Sensor-01",
            Type = "ESP32",
            Status = "active",
            LastSeen = DateTime.UtcNow.AddMinutes(-2)
        },
        new Device
        {
            Id = "photon2-pir-01",
            Name = "Photon2-Hub-01",
            Type = "Photon2",
            Status = "active",
            LastSeen = DateTime.UtcNow.AddMinutes(-5)
        }
    };

    context.Devices.AddRange(devices);

    // Only add devices, no fake sensor data or events
    // Real data will come from Azure IoT Hub

    context.SaveChanges();
}
