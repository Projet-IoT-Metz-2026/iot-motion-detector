# 🚀 Déploiement IoT Motion Detector Dashboard

Guide complet pour déployer le dashboard sur **Railway.app**

---

## 📋 Prérequis

- Compte GitHub connecté à Railway
- Azure IoT Hub configuré (`iot-iotdetector-dev`)
- Devices enregistrés:
  - `esp32-pir-01` (ESP32 via Azure IoT Hub)
  - `photon2-pir-01` (Photon2 via Particle Cloud)

---

## 🎯 Déploiement Railway (Recommandé)

### Étape 1: Créer le Projet Railway

1. Va sur **https://railway.app/**
2. Connecte-toi avec **GitHub**
3. Clique sur **"New Project"** → **"Deploy from GitHub repo"**
4. Sélectionne **"Projet-IoT-Metz-2026/iot-motion-detector"**

### Étape 2: Configuration Automatique

Railway détecte automatiquement:
- ✅ `Dockerfile` à la racine
- ✅ `railway.json` pour la configuration
- ✅ Build automatique via Docker

### Étape 3: Ajouter les Variables d'Environnement

Dans **Settings → Variables**, ajoute:

```bash
# Azure IoT Hub Configuration
Azure__IoTHub__ConnectionString=HostName=YOUR-IOT-HUB.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=YOUR_KEY_HERE
Azure__IoTHub__Enabled=true

# Particle Cloud Configuration
Particle__Enabled=true
Particle__EventName=pir_motion

# Database Configuration
Database__Provider=InMemory
Database__SeedData=false

# ASP.NET Core
ASPNETCORE_ENVIRONMENT=Production
```

### Étape 4: Générer un Domaine Public

1. Va dans **Settings → Networking**
2. Clique sur **"Generate Domain"**
3. Railway crée une URL: `https://iot-motion-detector-production.up.railway.app`

### Étape 5: Redéployer

Clique sur **"Redeploy"** pour appliquer les variables d'environnement.

---

## 🏗️ Architecture du Build

### Dockerfile Multi-Stage

**Stage 1: Build (SDK)**
- Image: `mcr.microsoft.com/dotnet/sdk:9.0`
- Copie `backend/src/IoTDetectorDashboard/*.csproj`
- Restore NuGet packages
- Build en mode Release
- Output: `/app/publish`

**Stage 2: Runtime**
- Image: `mcr.microsoft.com/dotnet/aspnet:9.0`
- Install Node.js 20 + Particle CLI
- Copie les binaires publiés
- Port: 8080 (configurable via `PORT` env var)

### Services Background Actifs

1. **IoTHubListenerService** - Écoute Azure IoT Hub Event Hub
2. **ParticleCloudListenerService** - Écoute Particle Cloud events via CLI
3. **DeviceMonitoringService** - Vérifie devices offline (5 min)
4. **AlertGenerationService** - Génère alertes temps réel

---

## 🔧 Configuration Locale

### appsettings.json (Production - Commité sur Git)
```json
{
  "Database": {
    "Provider": "InMemory",
    "SeedData": false
  },
  "Azure": {
    "IoTHub": {
      "Enabled": false,
      "ConnectionString": "YOUR_KEY_HERE"
    }
  },
  "Particle": {
    "Enabled": true,
    "EventName": "pir_motion"
  }
}
```

### appsettings.Development.json (Local - Git Ignored)
Contient les vraies connection strings pour développement local.

---

## 🌐 Alternatives de Déploiement

### Option B: Fly.io
```bash
flyctl launch
flyctl secrets set Azure__IoTHub__ConnectionString="..."
flyctl deploy
```

### Option C: DigitalOcean App Platform
1. Connecte le repo GitHub
2. Détecte automatiquement le Dockerfile
3. Configure les variables d'environnement
4. Deploy

### Option D: Azure App Service (Déjà configuré)
- URL: https://iotdetector-dashboard.azurewebsites.net
- Status: Running (mais background services limités en Free tier)

---

## 📊 Monitoring

### Logs Railway
```bash
# Via l'interface web
Railway Dashboard → Logs → View Real-time

# Filtres disponibles:
- Build Logs
- Deploy Logs
- Runtime Logs
```

### Endpoints de Santé
- `/` - Page d'accueil (Dashboard)
- `/Devices` - Liste des devices
- `/Alerts` - Liste des alertes
- `/sensorHub` - SignalR WebSocket

---

## ⚠️ Troubleshooting

### Build Échoue

**Erreur: "project or solution file not found"**
→ Vérifier que `Dockerfile` est à la racine du repo
→ Vérifier les chemins COPY dans le Dockerfile

**Erreur: "Railpack build failed"**
→ Vérifier que `railway.json` est à la racine
→ Vérifier `dockerfilePath: "Dockerfile"`

### Runtime Échoue

**App ne démarre pas**
→ Vérifier les variables d'environnement
→ Vérifier les logs: "Listening on port 8080"

**Devices ne s'affichent pas**
→ Vérifier `Azure__IoTHub__Enabled=true`
→ Vérifier la connection string Azure

---

## 🔐 Sécurité

### Secrets Gérés
- ✅ `appsettings.Development.json` → `.gitignore`
- ✅ Connection strings sur Railway (variables env)
- ✅ Aucun secret commité sur GitHub

### Accès IoT Hub
- Connection String utilise `SharedAccessKey`
- Permissions: `iothubowner` (lecture + écriture)
- Alternative recommandée: Managed Identity (pour production)

---

## 📈 Limites Railway Free Tier

- ✅ $5 crédit/mois inclus
- ✅ 500h d'exécution/mois
- ✅ Background services OK
- ✅ Builds illimités
- ⚠️ Pas de Always On (se réveille à la requête)
- ⚠️ 1GB RAM max

### Upgrade vers Hobby ($5/mois)
- Always On activé
- 8GB RAM
- Build prioritaire

---

## 🎉 Résultat Final

Dashboard accessible 24/7 avec:
- ✅ 2 devices IoT connectés (ESP32 + Photon2)
- ✅ Surveillance temps réel
- ✅ Alertes automatiques
- ✅ SignalR pour updates live
- ✅ HTTPS automatique

---

**Généré avec Claude Code**
Last Updated: 2025-10-27
