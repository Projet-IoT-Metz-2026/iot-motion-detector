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

### Services Background Automatiques

Le Dashboard inclut 3 services background qui tournent 24/7:

1. **IoTHubListenerService** - Écoute les événements Azure IoT Hub
   - Se connecte automatiquement au Event Hub built-in
   - Parse les messages télémétrie des ESP32/Photon2
   - Insère les données dans la base de données
   - Broadcast les updates via SignalR en temps réel

2. **AlertGenerationService** - Génération d'alertes intelligentes
   - Motion threshold: > 10 détections en 5 minutes
   - Weak signal: RSSI < -80 dBm
   - Low memory: Free memory < 100KB
   - Alertes automatiquement broadcast via SignalR

3. **DeviceMonitoringService** - Surveillance des devices
   - Vérifie les devices offline toutes les 5 minutes
   - Génère des alertes "device_offline" si > 1 heure sans données
   - Met à jour les statuts automatiquement

### Activation Azure IoT Hub

Mettez à jour `appsettings.json`:

```json
{
  "Azure": {
    "IoTHub": {
      "ConnectionString": "HostName=your-hub.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=YOUR_KEY"
    }
  }
}
```

**Redémarrez l'application**, les services se connecteront automatiquement!

### Format de Message Télémétrie

Les ESP32/Photon2 doivent envoyer des messages JSON:

```json
{
  "eventType": "motion_detected",
  "motionCount": 5,
  "rssi": -65,
  "freeMemory": 250000,
  "uptime": 3600,
  "firmwareVersion": "v3.0.0"
}
```

Le Dashboard créera automatiquement:
- ✅ Device si première connexion
- ✅ Entrée SensorData dans la DB
- ✅ Alertes si règles déclenchées
- ✅ Broadcast SignalR aux clients connectés

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
