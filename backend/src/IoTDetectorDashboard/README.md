# IoT Motion Detector Dashboard

Dashboard web ASP.NET MVC Razor pour superviser et gérer les capteurs PIR IoT connectés à Azure IoT Hub.

## Fonctionnalités

- **Dashboard temps réel** avec graphiques Chart.js
- **Gestion des devices** IoT (ESP32, Photon2)
- **Système d'alertes** avec filtres et acknowledgement
- **SignalR** pour mises à jour temps réel
- **Support PostgreSQL** et InMemory database

## Technologies

- ASP.NET Core 9.0
- Entity Framework Core 9.0
- PostgreSQL / InMemory Database
- SignalR 8.0
- Bootstrap 5
- Chart.js 4.4.1

## Configuration

### Option 1: Mode Développement (InMemory Database)

Fichier `appsettings.json`:
```json
{
  "Database": {
    "Provider": "InMemory",
    "SeedData": true
  }
}
```

### Option 2: Mode Production (PostgreSQL)

Fichier `appsettings.Production.json`:
```json
{
  "Database": {
    "Provider": "PostgreSQL",
    "SeedData": false
  },
  "ConnectionStrings": {
    "DefaultConnection": "Host=localhost;Database=iot_detector;Username=postgres;Password=yourpassword"
  }
}
```

## Installation

### Prérequis

- .NET 9.0 SDK
- PostgreSQL 16+ (pour production)
- Azure IoT Hub (optionnel)

### Étapes

1. **Cloner le repository**
   ```bash
   git clone <repo-url>
   cd backend/src/IoTDetectorDashboard
   ```

2. **Restaurer les packages**
   ```bash
   dotnet restore
   ```

3. **Configurer la base de données**

   **Option A: InMemory (développement)**
   - Aucune configuration nécessaire
   - Données de test automatiquement créées

   **Option B: PostgreSQL (production)**
   ```bash
   # Installer PostgreSQL
   # Créer la base de données
   createdb iot_detector

   # Mettre à jour appsettings.json avec vos credentials
   # Appliquer les migrations
   dotnet ef database update
   ```

4. **Lancer l'application**
   ```bash
   dotnet run
   ```

5. **Ouvrir le navigateur**
   ```
   http://localhost:5281
   ```

## Utilisation

### Pages disponibles

- **`/`** - Dashboard principal avec statistiques et graphiques
- **`/Devices`** - Liste des capteurs IoT
- **`/Devices/Details/{id}`** - Détails d'un capteur avec graphiques
- **`/Alerts`** - Gestion des alertes

### API SignalR

Endpoint: `/sensorHub`

Méthodes disponibles:
- `ReceiveSensorData` - Données capteur en temps réel
- `ReceiveAlert` - Nouvelle alerte
- `ReceiveDeviceStatus` - Changement statut device

## Structure du projet

```
IoTDetectorDashboard/
├── Controllers/          # Contrôleurs MVC
│   ├── HomeController.cs
│   ├── DevicesController.cs
│   └── AlertsController.cs
├── Models/              # Modèles de données
│   ├── Device.cs
│   ├── SensorData.cs
│   └── Alert.cs
├── Views/               # Vues Razor
│   ├── Home/
│   ├── Devices/
│   ├── Alerts/
│   └── Shared/
├── Data/                # Contexte EF Core
│   └── ApplicationDbContext.cs
├── Hubs/                # SignalR Hubs
│   └── SensorHub.cs
├── Services/            # Services métier
│   └── DatabaseSeeder.cs
└── Migrations/          # Migrations EF Core
```

## Configuration Azure IoT Hub

Pour connecter à Azure IoT Hub, mettez à jour `appsettings.json`:

```json
{
  "Azure": {
    "IoTHub": {
      "ConnectionString": "HostName=your-hub.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=YOUR_KEY"
    }
  }
}
```

## Déploiement

### Azure App Service

1. Publier l'application:
   ```bash
   dotnet publish -c Release -o ./publish
   ```

2. Déployer sur Azure:
   ```bash
   az webapp up --name your-app-name --resource-group your-rg
   ```

3. Configurer les variables d'environnement:
   - `Database__Provider=PostgreSQL`
   - `ConnectionStrings__DefaultConnection=<your-connection-string>`

## Développement

### Ajouter une migration

```bash
dotnet ef migrations add MigrationName
```

### Mettre à jour la base de données

```bash
dotnet ef database update
```

### Supprimer la dernière migration

```bash
dotnet ef migrations remove
```

## License

MIT License - Projet M2 IoT Metz 2026
