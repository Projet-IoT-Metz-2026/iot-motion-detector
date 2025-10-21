# 🏠 ESP32 Motion Detector - Azure IoT Hub

![Firmware](https://img.shields.io/badge/Firmware-v2.0.0-green)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Cloud](https://img.shields.io/badge/Cloud-Azure%20IoT-blue)
![Status](https://img.shields.io/badge/Status-Production%20Ready-success)

Système IoT professionnel de détection de mouvement connecté à Azure IoT Hub avec Device Twin, persistance EEPROM et architecture robuste niveau production.

---

## 📋 Table des matières

- [Présentation](#-présentation)
- [Caractéristiques](#-caractéristiques)
- [Architecture](#-architecture)
- [Prérequis](#-prérequis)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Utilisation](#-utilisation)
- [Tests](#-tests)
- [Monitoring](#-monitoring)
- [API](#-api)
- [Dépannage](#-dépannage)
- [Roadmap](#-roadmap)
- [Contribution](#-contribution)
- [License](#-license)

---

## 🎯 Présentation

Ce projet implémente un système de détection de mouvement intelligent basé sur ESP32, connecté à Azure IoT Hub pour le monitoring temps réel et le contrôle à distance.

### Cas d'usage

- 🏢 Surveillance de bureaux
- 🏠 Domotique intelligente
- 🔒 Système d'alarme
- 📊 Analyse de fréquentation
- 💡 Éclairage automatique

---

## ✨ Caractéristiques

### Firmware (v2.0.0)

#### 🔧 Fonctionnalités principales

- **Détection PIR** avec debounce et cooldown configurable
- **Communication bidirectionnelle** avec Azure IoT Hub (MQTT/TLS)
- **Device Twin** pour synchronisation de configuration
- **Persistance EEPROM** (configuration survit aux reboots)
- **Buffer de messages** anti-perte (50 messages max)
- **Reconnexion automatique** WiFi et MQTT
- **Watchdog timer** (protection contre les plantages)
- **Mode DEBUG** activable/désactivable

#### 📊 Métriques système

- Signal WiFi (RSSI)
- RAM disponible
- Temps de fonctionnement (uptime)
- Fréquence CPU
- Compteurs de reconnexion
- Statistiques de détection

#### 🏗️ Architecture logicielle

- **Machine à états non-bloquante** (pas de `while()` bloquant)
- **Structures de données organisées** (`DeviceConfig`, `DeviceMetrics`, `PirState`)
- **ArduinoJson** pour création/parsing JSON optimisé
- **Gestion mémoire optimisée** (~206 KB RAM libre)

---

## 🏛️ Architecture

### Schéma global

```
┌─────────────┐      WiFi/MQTT/TLS      ┌──────────────────┐
│   ESP32     │ ◄──────────────────────► │  Azure IoT Hub   │
│  + PIR HC   │                          │                  │
│   SR501     │   Device Twin / C2D      │  Device Twin     │
└─────────────┘                          └──────────────────┘
      │                                           │
      │                                           │
      ▼                                           ▼
 Détection                                  Monitoring
 mouvement                                  Commandes
```

### Stack technique

| Composant | Technologie |
|-----------|-------------|
| **Hardware** | ESP32-S3 |
| **Capteur** | PIR HC-SR501 |
| **Framework** | Arduino Core |
| **Build** | PlatformIO |
| **Cloud** | Azure IoT Hub |
| **Protocole** | MQTT over TLS 1.2 |
| **Auth** | SAS Token |
| **JSON** | ArduinoJson v6.21.3 |
| **Storage** | Preferences (EEPROM) |

---

## 🛠️ Prérequis

### Hardware

- **ESP32** (ESP32-S3, ESP32-WROOM, etc.)
- **Capteur PIR HC-SR501**
- **LED** (optionnel, GPIO 2 par défaut)
- Câbles de connexion
- Alimentation USB 5V

### Schéma de câblage

```
ESP32                    PIR HC-SR501
┌─────────┐             ┌──────────┐
│         │             │          │
│  GPIO13 │◄────────────│  OUT     │
│         │             │          │
│  5V     │─────────────│  VCC     │
│         │             │          │
│  GND    │─────────────│  GND     │
│         │             │          │
│  GPIO2  │──────────┐  └──────────┘
└─────────┘          │
                     │
                   ──┴──  LED (optionnel)
                   ─────
                     │
                    GND
```

### Software

- **PlatformIO CLI** (ou PlatformIO IDE)
- **Azure CLI** (pour les tests)
- **Git** (optionnel)

### Cloud

- **Compte Azure** (gratuit possible)
- **Azure IoT Hub** créé et configuré
- **Device** enregistré dans IoT Hub

---

## 📦 Installation

### 1. Cloner le repository

```bash
git clone https://github.com/mkzer0/iot-motion-detector.git
cd iot-motion-detector/firmware/esp32-pir-sensor
```

### 2. Installer les dépendances

```bash
pio lib install
```

### 3. Configurer les credentials

Créer le fichier `ESP32/src/secrets.h` :

```cpp
#ifndef SECRETS_H
#define SECRETS_H

// WiFi
#define WIFI_SSID "VotreSSID"
#define WIFI_PASSWORD "VotreMotDePasse"

// Azure IoT Hub
#define IOTHUB_HOST "votre-hub.azure-devices.net"
#define IOTHUB_DEVICE_ID "votre-device-id"
#define IOTHUB_DEVICE_KEY_BASE64 "votre-clé-primaire=="

#endif
```

**⚠️ Important :** Ne jamais committer `secrets.h` ! Ajoutez-le au `.gitignore`.

### 4. Compiler et uploader

```bash
cd ESP32

# Compiler
pio run

# Uploader vers l'ESP32
pio run --target upload

# Monitorer les logs
pio device monitor
```

---

## ⚙️ Configuration

### Variables modifiables

Dans `main.cpp` :

```cpp
// Hardware
const int PIR_PIN = 13;              // GPIO du capteur PIR
const int LED_PIN = 2;               // GPIO de la LED
const unsigned long DEBOUNCE_DELAY = 500;  // Anti-rebond (ms)

// Système
const int WDT_TIMEOUT = 30;          // Watchdog timeout (s)
const int MAX_BUFFER_SIZE = 50;      // Taille max buffer
const char* FIRMWARE_VERSION = "2.0.0";

// Mode DEBUG
#define DEBUG_MODE true              // false pour production
```

### Configuration par défaut

```cpp
DeviceConfig config = {
  .detectionEnabled = true,          // Détection activée
  .cooldownPeriod = 5000            // 5 secondes entre détections
};
```

### Configuration via Azure

La configuration peut être modifiée à distance via :
- **Device Twin** (propriétés desired)
- **Messages Cloud-to-Device** (commandes)

---

## 🚀 Utilisation

### Messages envoyés à Azure

#### Message de détection

```json
{
  "event": "motion",
  "count": 42,
  "ts": 123456,
  "config": {
    "detectionEnabled": true,
    "cooldown": 5000,
    "firmware": "2.0.0"
  },
  "system": {
    "rssi": -45,
    "freeHeap": 206624,
    "uptime": 3600,
    "cpuFreq": 240,
    "buffered": 0,
    "sentFromBuffer": 0,
    "wifiReconnects": 0,
    "mqttReconnects": 0
  }
}
```

#### Message de statut

```json
{
  "event": "status",
  "firmware": "2.0.0",
  "uptime": 3600,
  "detectionEnabled": true,
  "cooldown": 5000,
  "detectionCount": 42,
  "system": {
    "rssi": -45,
    "freeHeap": 206624,
    "buffered": 0,
    "wifiReconnects": 0,
    "mqttReconnects": 0,
    "failedPublishes": 0
  }
}
```

### Commandes Cloud-to-Device

#### Activer/Désactiver la détection

```json
{"command": "enable"}
{"command": "disable"}
```

#### Changer le cooldown

```json
{"command": "setCooldown", "value": 10000}
```

#### Obtenir le statut

```json
{"command": "getStatus"}
```

#### Redémarrer l'ESP32

```json
{"command": "reboot"}
```

#### Vider le buffer

```json
{"command": "clearBuffer"}
```

#### Obtenir le Device Twin

```json
{"command": "getTwin"}
```

### Device Twin

#### Propriétés desired (Azure → ESP32)

```json
{
  "properties": {
    "desired": {
      "detectionEnabled": true,
      "cooldown": 5000
    }
  }
}
```

#### Propriétés reported (ESP32 → Azure)

```json
{
  "properties": {
    "reported": {
      "firmware": "2.0.0",
      "uptime": 3600,
      "detectionEnabled": true,
      "cooldown": 5000,
      "detectionCount": 42,
      "system": {
        "rssi": -45,
        "freeHeap": 206624,
        "cpuFreq": 240,
        "buffered": 0
      }
    }
  }
}
```

---

## 🧪 Tests

### Tests automatisés

Le projet inclut des scripts de test automatisés dans le dossier `ESP32/`.

#### Windows (PowerShell)

```powershell
.\test-esp32-FIXED.ps1
```

#### Linux/macOS/Git Bash

```bash
chmod +x test-esp32-v2.sh
./test-esp32-v2.sh
```

### Tests manuels

#### Via Azure CLI

```bash
# Monitorer les événements
az iot hub monitor-events \
  --hub-name iot-detector-am2025 \
  --device-id esp32-pir-01

# Envoyer une commande
az iot device c2d-message send \
  --hub-name iot-detector-am2025 \
  --device-id esp32-pir-01 \
  --data '{"command":"getStatus"}'

# Modifier le Device Twin
az iot hub device-twin update \
  --hub-name iot-detector-am2025 \
  --device-id esp32-pir-01 \
  --set properties.desired='{"cooldown":8000}'
```

#### Via le portail Azure

1. Aller sur **portal.azure.com**
2. **IoT Hub** → Votre hub
3. **Devices** → Votre device
4. **Message to device** ou **Device Twin**

---

## 📊 Monitoring

### Voir les messages en temps réel

#### Option 1 : Azure CLI (Simple)

```bash
az iot hub monitor-events \
  --hub-name iot-detector-am2025 \
  --device-id esp32-pir-01 \
  --output table
```

#### Option 2 : Portail Azure (Visuel)

1. **Portal Azure** → IoT Hub → Votre hub
2. **Metrics** (à gauche)
3. Ajouter des métriques :
   - `Telemetry messages sent`
   - `Connected devices`
   - `Device to cloud messages`

#### Option 3 : Azure Monitor + Grafana (Avancé)

Pour créer des dashboards personnalisés, voir le guide : [docs/monitoring-setup.md](docs/monitoring-setup.md)

### Captures d'écran disponibles

Des exemples de dashboards sont disponibles dans le dossier `docs/images/` :
- `azure-metrics.png` - Métriques Azure natives
- `grafana-dashboard.png` - Dashboard Grafana personnalisé
- `device-twin.png` - État du Device Twin

---

## 📡 API

### États de connexion

```cpp
enum ConnectionState {
  DISCONNECTED,      // Pas de connexion
  CONNECTING_WIFI,   // Connexion WiFi en cours
  WIFI_CONNECTED,    // WiFi connecté
  CONNECTING_MQTT,   // Connexion MQTT en cours
  FULLY_CONNECTED    // Tout connecté
};
```

### Structures de données

```cpp
struct DeviceConfig {
  bool detectionEnabled;
  unsigned long cooldownPeriod;
  String firmwareVersion;
};

struct DeviceMetrics {
  unsigned long bootTime;
  int detectionCount;
  int bufferedMessagesCount;
  int sentFromBufferCount;
  unsigned long lastValidDetectionTime;
  int wifiReconnectCount;
  int mqttReconnectCount;
  int failedPublishCount;
};

struct PirState {
  bool lastState;
  unsigned long lastStateChangeTime;
  bool motionInProgress;
};
```

---

## 🔧 Dépannage

### L'ESP32 ne se connecte pas au WiFi

- Vérifier le SSID et mot de passe dans `secrets.h`
- Vérifier que le WiFi est en 2.4 GHz (pas 5 GHz)
- Vérifier la puissance du signal

### Impossible de se connecter à Azure

- Vérifier les credentials Azure dans `secrets.h`
- Vérifier que le device existe dans IoT Hub
- Vérifier l'heure système (NTP doit être synchronisé)
- Vérifier le certificat SSL

### Les détections ne fonctionnent pas

- Vérifier le câblage du PIR
- Vérifier que `detectionEnabled = true`
- Ajuster la sensibilité du PIR (potentiomètre)
- Vérifier le cooldown

### Mémoire insuffisante

- Réduire `MAX_BUFFER_SIZE`
- Passer `DEBUG_MODE` à `false`
- Vérifier les fuites mémoire (heap doit rester stable)

### Messages perdus

- Vérifier la stabilité WiFi
- Augmenter `MAX_BUFFER_SIZE` si nécessaire
- Vérifier les métriques `buffered` et `failedPublishes`

---

## 🗺️ Roadmap

### v2.1.0 (Prévu)

- [ ] Deep Sleep pour économie d'énergie
- [ ] OTA (Over-The-Air) updates
- [ ] Multi-capteurs (DHT22, BMP280)
- [ ] Logs sur carte SD

### v3.0.0 (Future)

- [ ] Interface web embarquée
- [ ] Support BLE
- [ ] Machine learning (détection d'anomalies)
- [ ] Réseau mesh (plusieurs ESP32)

---

## 🤝 Contribution

Les contributions sont les bienvenues !

### Comment contribuer

1. Fork le projet
2. Créer une branche (`git checkout -b feature/AmazingFeature`)
3. Commit les changements (`git commit -m 'Add AmazingFeature'`)
4. Push vers la branche (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

### Guidelines

- Code propre et commenté
- Tests passent (`./test-esp32-v2.sh`)
- Documentation mise à jour
- Commits atomiques et descriptifs

---

## 📄 License

Ce projet est sous licence MIT. Voir le fichier `LICENSE` pour plus de détails.

---

## 👤 Auteur

**Anis Makhezer**

- GitHub: [@mkzer0](https://github.com/mkzer0)
- LinkedIn: [Anis Makhezer](https://www.linkedin.com/in/anis-makhezer-046649309/)
- Email: mkzeranis@gmail.com

---

## 🙏 Remerciements

- [PlatformIO](https://platformio.org/) pour l'environnement de développement
- [ArduinoJson](https://arduinojson.org/) pour la gestion JSON
- [Azure IoT Hub](https://azure.microsoft.com/services/iot-hub/) pour la plateforme cloud
- La communauté ESP32 pour le support

---

## 📚 Ressources

- [Documentation ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Azure IoT Hub Documentation](https://docs.microsoft.com/azure/iot-hub/)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ArduinoJson Documentation](https://arduinojson.org/v6/doc/)

---

## 📊 Statistiques du projet

![GitHub stars](https://img.shields.io/github/stars/mkzer0/iot-motion-detector?style=social)
![GitHub forks](https://img.shields.io/github/forks/mkzer0/iot-motion-detector?style=social)
![GitHub issues](https://img.shields.io/github/issues/mkzer0/iot-motion-detector)
![GitHub license](https://img.shields.io/github/license/mkzer0/iot-motion-detector)

---

**⭐ Si ce projet vous a aidé, n'hésitez pas à lui donner une étoile !**