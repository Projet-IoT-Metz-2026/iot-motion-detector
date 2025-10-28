# Photon2 PIR Sensor (Photon2-v3)

Emplacement : `firmware/photon2-pir-sensor/Photon2-v3`

But : code et artefacts liés au firmware Photon2 (version v3 si fournie).

Contenu et actions :
- `compile-output.log` peut contenir la sortie de compilation et aider au debug.
- Le build est spécifique à l'écosystème Photon/Particle — suivez les instructions locales du dossier si présentes.

Commandes générales :
- Utiliser l'outil de build/flash du vendor (Particle CLI ou outil fourni).

Notes :
- Gardez les dépendances et bibliothèques tierces à jour.
- Ne placer aucune clé secrète dans les sources commitées.
# 📡 Photon2 PIR Motion Detector - Azure IoT Hub v3.0.0

![Firmware](https://img.shields.io/badge/Firmware-v3.0.0-green)
![Platform](https://img.shields.io/badge/Platform-Particle_Photon2-blue)
![Device_OS](https://img.shields.io/badge/Device_OS-6.x-orange)

Système IoT professionnel de détection de mouvement connecté à Azure IoT Hub via Particle Cloud + Webhooks.

---

## 📋 Table des Matières

- [Vue d'ensemble](#-vue-densemble)
- [Fonctionnalités v3.0.0](#-fonctionnalités-v30)
- [Matériel requis](#-matériel-requis)
- [Architecture](#-architecture)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Commandes Cloud](#-commandes-cloud)
- [Télémétrie](#-télémétrie)
- [Limitations vs ESP32](#-limitations-vs-esp32)

---

## 🎯 Vue d'ensemble

Firmware v3.0.0 pour Photon2 avec **les mêmes fonctionnalités que l'ESP32**, adapté à l'écosystème Particle Device OS.

### Différences vs ESP32

| Fonctionnalité | ESP32 | Photon2 v3.0.0 |
|----------------|-------|----------------|
| Communication | MQTT direct vers Azure | Particle Cloud + Webhooks |
| DPS Provisioning | Oui (HTTPS) | Via secrets.h (manuel) |
| Device Twin | Oui (MQTT) | Via Cloud Functions |
| Télémétrie enrichie | ✅ | ✅ |
| Message Queue offline | ✅ (LittleFS) | ✅ (RAM) |
| Cloud Commands | ✅ (C2D) | ✅ (Functions) |
| OTA Updates | ✅ (Azure Blob) | ✅ (Particle OTA) |
| Buffer overflow fix | ✅ | ✅ |
| millis() overflow fix | ✅ | ✅ |

---

## ✨ Fonctionnalités v3.0.0

### 🔒 Sécurité & Fiabilité
- ✅ **Buffer overflow protection** avec `strncpy` + null-termination
- ✅ **millis() overflow handling** avec fonction `hasElapsed()`
- ✅ **EEPROM persistence** pour configuration et compteurs

### 📡 Connectivité Robuste
- ✅ **Particle Cloud** avec reconnexion automatique
- ✅ **Message queue offline** (50 messages, FIFO)
- ✅ **Flush automatique** des messages en attente (5s)
- ✅ **Webhooks Azure** pour intégration IoT Hub

### 📊 Télémétrie Enrichie
```json
{
  "event": "motion_detected",
  "count": 42,
  "timestamp": 1729945678,
  "firmware_version": "3.0.0",
  "rssi": -45,
  "channel": 6,
  "freeMemory": 85000,
  "uptime": 3600
}
```

### ☁️ Commandes Cloud
Contrôle à distance via **Particle Functions** :

| Fonction | Description | Paramètres |
|----------|-------------|------------|
| `reboot` | Redémarre le Photon2 | aucun |
| `clearCache` | Vide la queue des messages | aucun |
| `setDetection` | Active/désactive la détection | `true` ou `false` |
| `calibratePIR` | Calibre le capteur PIR (30s) | aucun |
| `setCooldown` | Définit le cooldown (1000-300000ms) | millisecondes |

### 📈 Variables Cloud
Lecture en temps réel via **Particle Variables** :

| Variable | Description |
|----------|-------------|
| `version` | Version firmware (ex: "3.0.0") |
| `detCount` | Nombre total de détections |
| `cooldownMs` | Cooldown actuel en ms |
| `queueSize` | Taille de la queue de messages |

---

## 🔧 Matériel requis

### Composants

| Composant | Référence | Quantité |
|-----------|-----------|----------|
| **Microcontrôleur** | Particle Photon2 | 1 |
| **Capteur PIR** | HC-SR501 ou équivalent | 1 |
| **Câbles** | Jumpers Dupont M-F | 3 |
| **Alimentation** | USB-C 5V ou batterie | 1 |

### Connexions

```
HC-SR501          Photon2
────────          ───────
VCC      ────────  3V3 ou VIN (5V)
OUT      ────────  D2
GND      ────────  GND
```

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    PHOTON2 FIRMWARE                     │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────┐  ┌───────────┐  ┌─────────────────────┐ │
│  │  PIR     │  │ Config    │  │  Message Queue     │ │
│  │ Handler  │  │  Store    │  │  (50 msg FIFO)     │ │
│  │  (D2)    │  │ (EEPROM)  │  │  Offline Buffer    │ │
│  └──────────┘  └───────────┘  └─────────────────────┘ │
│       │              │                  │              │
│       └──────────────┴──────────────────┘              │
│                      │                                 │
│            ┌─────────▼──────────┐                      │
│            │   Main Loop        │                      │
│            │  - PIR monitoring  │                      │
│            │  - Queue flush     │                      │
│            │  - Heartbeat       │                      │
│            └─────────┬──────────┘                      │
│                      │                                 │
└──────────────────────┼─────────────────────────────────┘
                       │
                       ▼
            ┌──────────────────────┐
            │   Particle Cloud     │
            │   - Events           │
            │   - Functions        │
            │   - Variables        │
            └──────────┬───────────┘
                       │
                       ▼
            ┌──────────────────────┐
            │  Azure IoT Hub       │
            │  (via Webhooks)      │
            └──────────────────────┘
```

---

## 🚀 Installation

### 1. Prérequis

- [Particle Workbench](https://www.particle.io/workbench/) ou [Particle CLI](https://docs.particle.io/getting-started/developer-tools/cli/)
- Compte Particle Cloud
- Photon2 flashé avec Device OS 6.x+

### 2. Cloner le projet

```bash
cd firmware/photon2-pir-sensor/Photon2-v3
```

### 3. Configurer les secrets

```bash
cp src/secrets.h.template src/secrets.h
```

Éditer `src/secrets.h` :

```cpp
#define WIFI_SSID "VotreSSID"
#define WIFI_PASSWORD "VotreMotDePasse"
#define DPS_ID_SCOPE "0neXXXXXXXX"
#define REGISTRATION_ID "photon2-pir-01"
#define DEVICE_KEY "VotreCléBase64=="
#define FW_VERSION "3.0.0"
```

### 4. Compiler et flasher

#### Via Particle Workbench (VSCode)

1. Ouvrir le dossier `Photon2-v3` dans VSCode
2. `Ctrl+Shift+P` → `Particle: Cloud Compile`
3. `Ctrl+Shift+P` → `Particle: Cloud Flash`

#### Via Particle CLI

```bash
# Compiler
particle compile photon2 .

# Flasher via cloud
particle flash <device-name> firmware.bin

# Ou flasher via USB (DFU mode)
particle flash --usb firmware.bin
```

### 5. Vérifier le fonctionnement

```bash
# Console série
particle serial monitor

# Logs cloud
particle logs <device-name> --follow
```

---

## ⚙️ Configuration

### Configuration Azure (Webhooks)

Pour envoyer les événements à Azure IoT Hub, créer un **Particle Webhook** :

```bash
particle webhook create azure_iot_hub "https://<iot-hub-name>.azure-devices.net/devices/<device-id>/messages/events?api-version=2020-09-30" \
  --event motion_telemetry \
  --method POST \
  --headers "Content-Type: application/json" \
  --headers "Authorization: SharedAccessSignature sr=..."
```

Ou via [Particle Console](https://console.particle.io/integrations/webhooks/create).

### Configuration PIR

Le capteur HC-SR501 possède 2 potentiomètres :
- **Sensitivity (Sx)** : Portée de détection (3-7m)
- **Time Delay (Tx)** : Durée du signal HIGH (3s-300s)

Recommandé :
- Sensitivity : ~50% (5m)
- Time Delay : Minimum (3s)

---

## ☁️ Commandes Cloud

### Via Particle CLI

```bash
# Redémarrer le device
particle call <device-name> reboot

# Vider le cache des messages
particle call <device-name> clearCache

# Activer la détection
particle call <device-name> setDetection "true"

# Désactiver la détection
particle call <device-name> setDetection "false"

# Calibrer le PIR (30s)
particle call <device-name> calibratePIR

# Définir cooldown à 5 secondes
particle call <device-name> setCooldown "5000"
```

### Via Particle Console

1. Aller sur [console.particle.io](https://console.particle.io)
2. Sélectionner votre device
3. Onglet **Functions**
4. Exécuter les commandes

---

## 📊 Télémétrie

### Événements publiés

#### Motion Detection

```json
{
  "event": "motion_detected",
  "count": 42,
  "timestamp": 1729945678,
  "firmware_version": "3.0.0",
  "rssi": -45,
  "channel": 6,
  "freeMemory": 85000,
  "uptime": 3600
}
```

#### Heartbeat (toutes les 5 minutes)

```json
{
  "event": "heartbeat",
  "uptime": 3600,
  "detectionCount": 42,
  "queueSize": 0,
  "freeMemory": 85000,
  "rssi": -45
}
```

### Lecture via CLI

```bash
# Écouter les événements en temps réel
particle subscribe motion_telemetry <device-name>

# Lire les variables
particle get <device-name> version
particle get <device-name> detCount
particle get <device-name> cooldownMs
particle get <device-name> queueSize
```

---

## ⚠️ Limitations vs ESP32

| Aspect | ESP32 | Photon2 v3.0.0 |
|--------|-------|----------------|
| **Communication** | MQTT direct | Particle Cloud (webhook vers Azure) |
| **DPS Auto-provisioning** | ✅ Automatique | ❌ Manuel (secrets.h) |
| **Device Twin** | ✅ MQTT topics | ⚠️ Via Cloud Functions |
| **Message Queue** | ✅ LittleFS (flash) | ⚠️ RAM (volatile) |
| **SAS Token** | ✅ Génération auto | ❌ Non implémenté |
| **OTA Updates** | Azure Blob Storage | Particle OTA natif |

### Pourquoi ces limitations ?

Le Photon2 utilise **Particle Device OS**, qui n'a pas de bibliothèque MQTT robuste compatible avec Azure IoT Hub (TLS 1.2 + SAS Token). L'approche **Particle Cloud + Webhooks** est plus stable et maintenue.

### Avantages du Photon2

✅ **OTA updates** plus simple (Particle OTA natif)
✅ **Cellular ready** (avec Photon2 LTE)
✅ **Console web** intuitive pour debug
✅ **Faible consommation** (sleep modes optimisés)
✅ **Support commercial** Particle

---

## 🐛 Dépannage

### Device ne se connecte pas au WiFi

```bash
# Réinitialiser les credentials WiFi
particle serial wifi

# Vérifier le signal
particle get <device-name> rssi
```

### Message queue se remplit

```bash
# Vider la queue
particle call <device-name> clearCache

# Vérifier la taille
particle get <device-name> queueSize
```

### LED clignote en boucle

Vérifier les logs :

```bash
particle serial monitor
```

---

## 📄 License

Copyright © 2025 - IoT Motion Detector Project

---

## 🤝 Support

- [Particle Community](https://community.particle.io)
- [Azure IoT Hub Docs](https://docs.microsoft.com/azure/iot-hub/)
- [Project Repository](https://github.com/Projet-IoT-Metz-2026/iot-motion-detector)
