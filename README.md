# 📡 ESP32 Motion Detector - Azure IoT Hub

Système de détection de mouvement intelligent connecté à Azure IoT Hub via MQTT sécurisé.

---

## 📋 Table des Matières

- [Vue d'ensemble](#-vue-densemble)
- [Matériel requis](#-matériel-requis)
- [Architecture](#-architecture)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Utilisation](#-utilisation)
- [Validation](#-validation)
- [Dépannage](#-dépannage)
- [Évolutions futures](#-évolutions-futures)

---

## 🎯 Vue d'ensemble

Ce projet permet de détecter des mouvements via un capteur PIR et d'envoyer les événements en temps réel vers Azure IoT Hub.

### Fonctionnalités

✅ **Détection de mouvement fiable**
- Anti-rebond (500ms)
- Cooldown entre détections (5s)
- Compteur de détections

✅ **Communication sécurisée**
- TLS/SSL (port 8883)
- Authentification Azure via SAS Token
- Certificat racine DigiCert Global Root G2

✅ **Monitoring temps réel**
- Publication MQTT automatique
- Messages au format JSON
- Logs détaillés en Serial Monitor

✅ **Robustesse**
- Reconnexion automatique WiFi/MQTT
- Synchronisation NTP
- Gestion des erreurs

---

## 🔧 Matériel requis

| Composant | Modèle | Quantité | Notes |
|-----------|--------|----------|-------|
| **Microcontrôleur** | ESP32 Dev Module | 1 | 4MB Flash minimum |
| **Capteur PIR** | HC-SR501 | 1 | Détection mouvement |
| **LED** | - | 1 | Indicateur visuel (optionnel) |
| **Câbles** | Dupont M-M | 3-5 | Connexions |
| **Alimentation** | USB 5V | 1 | Micro-USB ou USB-C |

### Schéma de câblage
```
ESP32                    HC-SR501 (PIR)
┌──────────┐            ┌──────────┐
│          │            │          │
│  GPIO 13 ├────────────┤ OUT      │
│      GND ├────────────┤ GND      │
│      5V  ├────────────┤ VCC      │
│          │            │          │
│  GPIO 2  ├────────────┤ LED      │ (optionnel)
│          │            │          │
└──────────┘            └──────────┘
```

---

## 🏗️ Architecture
```
┌─────────────┐
│ Capteur PIR │ (GPIO 13)
└──────┬──────┘
       │
       ↓
┌─────────────────────────────────┐
│          ESP32                   │
│  • Détection mouvement          │
│  • Anti-rebond + Cooldown       │
│  • Génération SAS Token         │
└──────────┬──────────────────────┘
           │ WiFi
           ↓
      [INTERNET]
           │ MQTT/TLS (port 8883)
           ↓
┌─────────────────────────────────┐
│     Azure IoT Hub               │
│  iot-detector-am2025            │
│  • Réception messages           │
│  • Stockage télémétrie          │
│  • Routage événements           │
└─────────────────────────────────┘
```

---

## 💻 Installation

### Prérequis

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)
- Compte Azure avec IoT Hub configuré

### 1. Cloner le projet
```bash
git clone https://github.com/votre-repo/iot-motion-detector.git
cd iot-motion-detector/firmware/esp32-pir-sensor/ESP32
```

### 2. Installer les dépendances

PlatformIO installera automatiquement les librairies au premier build :
- `PubSubClient v2.8` (MQTT)
- WiFi, WiFiClientSecure (intégrés ESP32)
- mbedTLS (intégré ESP32)

### 3. Configurer les secrets
```bash
# Copier le fichier exemple
cp src/secrets.example.h src/secrets.h

# Éditer avec vos credentials
nano src/secrets.h
```

**Fichier `secrets.h`** :
```cpp
#pragma once

// WiFi credentials
#define WIFI_SSID        "VotreSSID"
#define WIFI_PASSWORD    "VotreMotDePasse"

// Azure IoT Hub
#define IOTHUB_HOST                  "votre-hub.azure-devices.net"
#define IOTHUB_DEVICE_ID             "votre-device-id"
#define IOTHUB_DEVICE_KEY_BASE64     "votre-cle-base64=="
```

⚠️ **IMPORTANT** : Ne jamais commiter `secrets.h` ! (protégé par `.gitignore`)

### 4. Compiler et uploader
```bash
# Compiler
pio run

# Uploader vers l'ESP32
pio run --target upload

# Moniteur série
pio device monitor
```

Ou via l'interface PlatformIO dans VS Code :
- `Build` (✓)
- `Upload` (→)
- `Serial Monitor` (🔌)

---

## ⚙️ Configuration

### Configuration Azure IoT Hub

1. **Créer un IoT Hub** (si pas encore fait)
```bash
az iot hub create --name votre-hub --resource-group votre-rg --sku F1
```

2. **Créer un device**
```bash
az iot hub device-identity create --hub-name votre-hub --device-id esp32-pir-01
```

3. **Récupérer la clé de connexion**
```bash
az iot hub device-identity connection-string show --hub-name votre-hub --device-id esp32-pir-01
```

Format de la connection string :
```
HostName=votre-hub.azure-devices.net;DeviceId=esp32-pir-01;SharedAccessKey=VOTRE_CLE_BASE64==
```

Extraire :
- `IOTHUB_HOST` = `votre-hub.azure-devices.net`
- `IOTHUB_DEVICE_ID` = `esp32-pir-01`
- `IOTHUB_DEVICE_KEY_BASE64` = `VOTRE_CLE_BASE64==`

### Paramètres ajustables

Dans `main.cpp` :
```cpp
// GPIO
const int PIR_PIN = 13;                    // Pin du capteur PIR
const int LED_PIN = 2;                     // Pin LED indicateur

// Timings
const unsigned long DEBOUNCE_DELAY = 500;  // Anti-rebond (ms)
const unsigned long COOLDOWN_PERIOD = 5000; // Cooldown entre détections (ms)

// MQTT
#define MQTT_MAX_PACKET_SIZE 1024          // Taille max message MQTT

// SAS Token
uint32_t ttl = 3600;                       // Validité token (secondes)
```

---

## 🚀 Utilisation

### Démarrage

1. **Alimenter l'ESP32** via USB
2. **Ouvrir le Serial Monitor** (115200 baud)
3. **Observer les logs** :
```
=== ESP32 Motion Detector v1.6 (Azure Root CA) ===
[WiFi] Connexion à VotreSSID ...
..
[WiFi] Connecté, IP: 192.168.1.42

[TLS] Configuration du certificat Azure IoT Hub...
[NTP] Tentative de synchronisation de l'heure...
[NTP] ✅ Heure synchronisée : Thu Oct 16 16:34:05 2025

--- Informations de Connexion MQTT ---
Client ID: esp32-pir-01
Username: votre-hub.azure-devices.net/esp32-pir-01/?api-version=2020-09-30
SAS Token: SharedAccessSignature sr=...&sig=...&se=...
-------------------------------------

[MQTT] Connexion à IoT Hub...
[MQTT] ✅ Connecté à IoT Hub

[INIT] Calibration du capteur PIR (10s)...
[INIT] 10 s...
[INIT] 9 s...
...
[READY] Système prêt !
```

4. **Passer la main devant le capteur PIR**
```
[DEBUG] Signal PIR détecté (attente stabilisation...)
╔═════════════════════════════════════════╗
║       🚨 MOUVEMENT DÉTECTÉ ! 🚨         ║
╚═════════════════════════════════════════╝
[DETECTION #1]
  → Timestamp: 19298 ms
─────────────────────────────────────────
[MQTT] Publish ✅ OK : {"event":"motion","count":1,"ts":19330}
```

### Format des messages

Messages publiés sur le topic :
```
devices/esp32-pir-01/messages/events/
```

Payload JSON :
```json
{
  "event": "motion",
  "count": 1,
  "ts": 19330
}
```

| Champ | Type | Description |
|-------|------|-------------|
| `event` | string | Type d'événement (`"motion"`) |
| `count` | integer | Compteur de détections depuis le boot |
| `ts` | integer | Timestamp en millisecondes (depuis boot ESP32) |

---

## ✅ Validation

### Tests effectués (16/10/2025)

✅ **Compilation** : Réussie sans warnings  
✅ **Upload firmware** : OK  
✅ **Connexion WiFi** : Stable  
✅ **Synchronisation NTP** : OK  
✅ **Connexion TLS/SSL** : Certificat validé  
✅ **Authentification Azure** : SAS Token accepté  
✅ **Publication MQTT** : 100% de réussite  
✅ **Réception Azure** : Messages confirmés en temps réel  

### Monitorer les messages dans Azure

**Option 1 : Azure Cloud Shell**
```bash
az iot hub monitor-events --hub-name votre-hub --device-id esp32-pir-01
```

Output attendu :
```json
{
    "event": {
        "origin": "esp32-pir-01",
        "payload": "{\"event\":\"motion\",\"count\":1,\"ts\":19330}"
    }
}
```

**Option 2 : Azure Portal**
1. Aller dans votre IoT Hub
2. Menu `Surveillance` → `Métriques`
3. Ajouter métrique : `Telemetry messages sent`
4. Observer le graphique

**Option 3 : Device Explorer (Windows)**
- Télécharger [Device Explorer](https://github.com/Azure/azure-iot-sdk-csharp/tree/main/tools/DeviceExplorer)
- Configurer la connection string
- Onglet `Data` → `Monitor`

### Résultats validation
```
📊 Statistiques de test :
├── Messages envoyés : 23+
├── Messages reçus : 23+ (100%)
├── Latence moyenne : ~50ms
├── Pertes : 0%
├── Durée test : 3 minutes
└── Reconnexions : 0
```

**✅ Système validé en production !**

---

## 🐛 Dépannage

### Erreur : Compilation échoue

**Symptôme** :
```
error: no matching function for call to 'WiFiClientSecure::setCACertBundle(...)'
```

**Solution** :
- Vérifier que vous utilisez le code corrigé avec `setCACert(azure_root_ca)`
- Framework ESP32 v3.x requis
- Ne pas utiliser `setCACertBundle()`

---

### Erreur : `start_ssl_client: -1`

**Symptôme** :
```
[MQTT] Connexion à IoT Hub...
[E][WiFiClientSecure.cpp:144] connect(): start_ssl_client: -1
[MQTT] ❌ Échec, rc=-2
```

**Cause** : Certificat SSL non configuré ou incorrect

**Solution** :
1. Vérifier que le certificat `azure_root_ca` est bien dans `main.cpp`
2. Vérifier l'appel à `tlsClient.setCACert(azure_root_ca);`
3. Le certificat doit être DigiCert Global Root G2

---

### Erreur : `rc=4` (Bad Credentials)

**Symptôme** :
```
[MQTT] ❌ Échec, rc=4
```

**Causes possibles** :
1. **SAS Token invalide**
   - Vérifier que l'heure NTP est synchronisée
   - Token expiré (régénéré toutes les heures)
   - Clé device incorrecte dans `secrets.h`

2. **Credentials incorrects**
   - Vérifier `IOTHUB_HOST`
   - Vérifier `IOTHUB_DEVICE_ID`
   - Vérifier `IOTHUB_DEVICE_KEY_BASE64`

**Solution** :
```cpp
// Vérifier dans les logs :
[NTP] ✅ Heure synchronisée : Thu Oct 16 16:34:05 2025
// Si "Échec synchronisation" → vérifier NTP
```

---

### WiFi ne se connecte pas

**Symptôme** :
```
[WiFi] Connexion à VotreSSID ...
....................... (timeout)
```

**Solutions** :
1. Vérifier SSID et mot de passe dans `secrets.h`
2. Vérifier que le WiFi est en 2.4GHz (ESP32 ne supporte pas 5GHz)
3. Rapprocher l'ESP32 du routeur
4. Vérifier que le SSID est visible (pas caché)

---

### PIR ne détecte rien

**Symptôme** :
Aucun message `[DEBUG] Signal PIR détecté...`

**Solutions** :
1. **Vérifier le câblage**
   - OUT du PIR → GPIO 13 ESP32
   - VCC du PIR → 5V ESP32
   - GND du PIR → GND ESP32

2. **Attendre la calibration** (10 secondes au boot)

3. **Ajuster la sensibilité du PIR**
   - 2 potentiomètres sur le HC-SR501
   - Sensitivity (Sx) : Distance de détection
   - Time delay (Tx) : Durée du signal

4. **Tester le PIR** :
```cpp
// Code de test dans loop()
Serial.println(digitalRead(PIR_PIN));  // Doit afficher 0 ou 1
delay(500);
```

---

### Messages n'arrivent pas dans Azure

**Symptôme** :
```
[MQTT] Publish ✅ OK : {...}
```
Mais rien dans Azure Cloud Shell

**Solutions** :
1. Vérifier que le device existe dans IoT Hub
2. Vérifier que le device est `Enabled` (pas `Disabled`)
3. Vérifier le topic MQTT :
```
   devices/DEVICE_ID/messages/events/
```
4. Utiliser Azure Portal → Métriques pour voir les messages reçus

---

## 📊 Performances

### Consommation
```
Mode actif (WiFi + MQTT) : ~160 mA @ 3.3V
Mode idle : ~40 mA @ 3.3V
Deep sleep (non implémenté) : ~10 µA @ 3.3V
```

### Mémoire
```
Flash utilisée : ~900 KB / 4 MB (22%)
RAM utilisée : ~100 KB / 520 KB (19%)
```

### Réseau
```
Latence publication : ~50ms
Temps de reconnexion : ~3s
Keep-alive MQTT : 60s
```

---

## 🚀 Évolutions futures

### Communication

- [ ] **Communication bidirectionnelle** : Recevoir des commandes depuis Azure (activer/désactiver détection, changer cooldown)
- [ ] **Device Twin** : Synchroniser la configuration avec Azure (firmware version, uptime, état)
- [ ] **OTA updates** : Mise à jour du firmware à distance sans câble USB

### Robustesse

- [ ] **Watchdog timer** : Auto-reset en cas de freeze du système
- [ ] **Buffer local** : Sauvegarder les messages si perte de connexion temporaire
- [ ] **Métriques système** : Ajouter WiFi RSSI, mémoire libre, uptime dans les messages

### Backend & Monitoring

- [ ] **API Backend** : Créer une API REST (.NET/Node.js) pour exposer les données
- [ ] **Base de données** : Stocker l'historique (SQL Server, CosmosDB)
- [ ] **Dashboard web** : Interface temps réel avec graphiques (React/Vue.js)
- [ ] **Notifications** : Alertes email/SMS/Teams via Azure Functions

### Optimisation

- [ ] **Mode deep sleep** : Économie batterie avec réveil par interrupt PIR
- [ ] **Multi-devices** : Gérer plusieurs ESP32 avec le même backend
- [ ] **Machine Learning** : Détecter les patterns de mouvement anormaux

---

## 📚 Ressources

### Documentation

- [Azure IoT Hub MQTT Support](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
- [PlatformIO Documentation](https://docs.platformio.org/)

### Librairies

- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/)

### Outils

- [Azure CLI](https://docs.microsoft.com/en-us/cli/azure/)
- [MQTT Explorer](http://mqtt-explorer.com/)
- [Device Explorer](https://github.com/Azure/azure-iot-sdk-csharp/tree/main/tools/DeviceExplorer)

---

## 👥 Contributeurs

- **Développeur** : M. KHEZER
- **Date** : Octobre 2025
- **Version** : 1.6 (Azure Root CA)

---

## 📄 Licence

MIT License

---

## 🆘 Support

Pour toute question ou problème :
1. Consulter la section [Dépannage](#-dépannage)
2. Ouvrir une issue sur GitHub
3. Consulter les forums Azure/ESP32

---

**🎉 Projet validé et fonctionnel en production !**

*Dernière mise à jour : 16 octobre 2025*