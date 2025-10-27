using IoTDetectorDashboard.Data;
using IoTDetectorDashboard.Services;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container
builder.Services.AddControllersWithViews();

// Configure Database based on appsettings.json
var databaseProvider = builder.Configuration["Database:Provider"] ?? "InMemory";
var connectionString = builder.Configuration.GetConnectionString("DefaultConnection");

builder.Services.AddDbContext<ApplicationDbContext>(options =>
{
    if (databaseProvider == "PostgreSQL")
    {
        options.UseNpgsql(connectionString);
        builder.Logging.AddConsole();
        Console.WriteLine($"Using PostgreSQL database: {connectionString?.Split(';')[0]}");
    }
    else
    {
        options.UseInMemoryDatabase("IoTDetectorDb");
        Console.WriteLine("Using InMemory database (for demo purposes)");
    }
});

// Add SignalR for real-time updates
builder.Services.AddSignalR();

// Add IoT services
builder.Services.AddSingleton<AlertGenerationService>();
builder.Services.AddHostedService<IoTHubListenerService>();
builder.Services.AddHostedService<ParticleCloudListenerService>();
builder.Services.AddHostedService<DeviceMonitoringService>();

var app = builder.Build();

// Initialize and seed database
using (var scope = app.Services.CreateScope())
{
    var services = scope.ServiceProvider;
    var context = services.GetRequiredService<ApplicationDbContext>();
    var logger = services.GetRequiredService<ILogger<Program>>();

    try
    {
        // Apply migrations for PostgreSQL
        if (databaseProvider == "PostgreSQL")
        {
            logger.LogInformation("Applying database migrations...");
            context.Database.Migrate();
            logger.LogInformation("Database migrations applied successfully");
        }

        // Seed data if configured
        var shouldSeed = builder.Configuration.GetValue<bool>("Database:SeedData");
        if (shouldSeed)
        {
            DatabaseSeeder.SeedDatabase(context, logger);
        }
    }
    catch (Exception ex)
    {
        logger.LogError(ex, "An error occurred while initializing the database");
        if (databaseProvider == "PostgreSQL")
        {
            logger.LogError("Make sure PostgreSQL is running and connection string is correct");
            throw;
        }
    }
}

// Configure the HTTP request pipeline
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    app.UseHsts();
}
else
{
    app.UseDeveloperExceptionPage();
}

app.UseHttpsRedirection();
app.UseRouting();
app.UseAuthorization();

app.MapStaticAssets();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}")
    .WithStaticAssets();

// Map SignalR Hub
app.MapHub<IoTDetectorDashboard.Hubs.SensorHub>("/sensorHub");

app.Run();
