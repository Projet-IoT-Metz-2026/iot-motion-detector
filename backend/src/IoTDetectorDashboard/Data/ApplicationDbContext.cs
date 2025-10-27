using Microsoft.EntityFrameworkCore;
using IoTDetectorDashboard.Models;

namespace IoTDetectorDashboard.Data;

public class ApplicationDbContext : DbContext
{
    public ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
        : base(options)
    {
    }

    public DbSet<Device> Devices { get; set; }
    public DbSet<SensorData> SensorData { get; set; }
    public DbSet<Alert> Alerts { get; set; }

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        // Configuration Device
        modelBuilder.Entity<Device>()
            .HasKey(d => d.DeviceId);

        modelBuilder.Entity<Device>()
            .HasMany(d => d.SensorDataList)
            .WithOne(s => s.Device)
            .HasForeignKey(s => s.DeviceId)
            .OnDelete(DeleteBehavior.Cascade);

        // Index pour améliorer les performances
        modelBuilder.Entity<SensorData>()
            .HasIndex(s => s.Timestamp);

        modelBuilder.Entity<Alert>()
            .HasIndex(a => a.Timestamp);
    }
}
