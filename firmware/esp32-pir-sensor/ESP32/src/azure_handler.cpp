// src/azure_handler.cpp
#include "azure_handler.h"
#include "secrets.h"
#include "config_store.h"
#include "ota_handler.h"
#include "message_queue.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

// ============================================
// HELPER : Gestion du débordement de millis()
// ============================================

/**
 * Vérifie si un intervalle de temps s'est écoulé depuis un timestamp.
 * Cette fonction gère correctement le débordement de millis() (après ~49 jours)
 * grâce à l'arithmétique modulaire des unsigned long.
 *
 * @param lastTime Timestamp de référence (en ms)
 * @param interval Intervalle à vérifier (en ms)
 * @return true si l'intervalle s'est écoulé
 */
inline bool hasElapsed(unsigned long lastTime, unsigned long interval) {
  return (millis() - lastTime) >= interval;
}

// ============================================
// CERTIFICAT AZURE (DigiCert Global Root G2)
// ============================================

static const char* AZURE_ROOT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n";

// ============================================
// CONSTANTES
// ============================================

static const int MQTT_PORT = 8883;
static const unsigned long WIFI_CONNECT_TIMEOUT = 30000;
static const unsigned long MQTT_RECONNECT_BASE_DELAY = 1000;  // Délai initial : 1 seconde
static const unsigned long MQTT_RECONNECT_MAX_DELAY = 30000;  // Délai max : 30 secondes
static const unsigned long STATUS_PUBLISH_INTERVAL = 60000;

// Limites pour les propriétés Device Twin
static const int COOLDOWN_MIN_MS = 1000;    // 1 seconde minimum
static const int COOLDOWN_MAX_MS = 300000;  // 5 minutes maximum

// ============================================
// VARIABLES GLOBALES (définies dynamiquement par DPS)
// ============================================

static String iotHubHostname = "";
static String deviceId = "";
static String firmwareVersion = "unknown";

// ============================================
// VARIABLES GLOBALES (MQTT, WiFi, etc.)
// ============================================

static WiFiClientSecure wifiClient;
static PubSubClient mqttClient(wifiClient);

static char mqttUsername[256];
static char mqttPassword[512];
static char topicTelemetry[128];
static char topicTwinGet[128];
static char topicTwinPatch[128];
static char topicTwinRes[128];

static unsigned long lastStatusPublish = 0;
static unsigned long lastMqttAttempt = 0;
static int mqttRetryCount = 0;  // Nombre de tentatives échouées
static unsigned long mqttCurrentBackoff = MQTT_RECONNECT_BASE_DELAY;  // Délai actuel

static void (*pirDetectionCallback)() = nullptr;

// ============================================
// ÉTATS (renommés pour éviter conflit avec PubSubClient)
// ============================================

enum AzureWiFiState {
  AZURE_WIFI_DISCONNECTED,
  AZURE_WIFI_CONNECTING,
  AZURE_WIFI_CONNECTED
};

enum AzureMqttState {
  AZURE_MQTT_DISCONNECTED,
  AZURE_MQTT_CONNECTING,
  AZURE_MQTT_CONNECTED
};

static AzureWiFiState wifiState = AZURE_WIFI_DISCONNECTED;
static AzureMqttState mqttState = AZURE_MQTT_DISCONNECTED;

// ============================================
// FONCTIONS UTILITAIRES
// ============================================

/**
 * Synchronisation NTP
 */
bool syncTime() {
  Serial.println("[NTP] Synchronisation de l'heure...");
  
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  unsigned long start = millis();
  while (time(nullptr) < 1000000000 && !hasElapsed(start, 15000)) {
    delay(100);
  }
  
  if (time(nullptr) < 1000000000) {
    Serial.println("[NTP] ⚠️ Échec de synchronisation");
    return false;
  }
  
  Serial.println("[NTP] ✅ Heure synchronisée");
  return true;
}

/**
 * Génère un SAS Token pour l'IoT Hub
 */
String generateSasToken() {
  time_t now = time(nullptr);
  time_t expiry = now + 86400; // 24h
  
  // String to sign
  char stringToSign[512];
  snprintf(stringToSign, sizeof(stringToSign),
           "%s/devices/%s\n%ld",
           iotHubHostname.c_str(), deviceId.c_str(), expiry);
  
  // Decode device key
  uint8_t decodedKey[64];
  size_t decodedLen;
  int ret = mbedtls_base64_decode(decodedKey, sizeof(decodedKey), &decodedLen,
                                   (const unsigned char*)DEVICE_KEY, strlen(DEVICE_KEY));

  // Vérifier que le décodage a réussi et que la taille est valide
  if (ret != 0 || decodedLen > sizeof(decodedKey)) {
    Serial.println("[Azure] ❌ Erreur: DEVICE_KEY invalide ou trop longue");
    return String("");
  }

  // HMAC-SHA256
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, decodedKey, decodedLen);
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign, strlen(stringToSign));
  mbedtls_md_hmac_finish(&ctx, hash);
  mbedtls_md_free(&ctx);
  
  // Encode en base64
  char signature[64];
  size_t sigLen;
  mbedtls_base64_encode((unsigned char*)signature, sizeof(signature), &sigLen,
                        hash, sizeof(hash));
  signature[sigLen] = '\0';
  
  // URL encode
  String encodedSig = "";
  for (size_t i = 0; i < sigLen; i++) {
    char c = signature[i];
    if (c == '+') encodedSig += "%2B";
    else if (c == '/') encodedSig += "%2F";
    else if (c == '=') encodedSig += "%3D";
    else encodedSig += c;
  }
  
  // Format SAS Token
  char sasToken[512];
  snprintf(sasToken, sizeof(sasToken),
           "SharedAccessSignature sr=%s/devices/%s&sig=%s&se=%ld",
           iotHubHostname.c_str(), deviceId.c_str(), encodedSig.c_str(), expiry);
  
  return String(sasToken);
}

/**
 * Callback MQTT pour les messages entrants
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] Message sur topic: %s\n", topic);
  
  // Device Twin updates
  if (strstr(topic, "$iothub/twin/PATCH/properties/desired") != nullptr) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (!error) {
      if (doc.containsKey("detectionEnabled")) {
        bool enabled = doc["detectionEnabled"];
        configSetDetectionEnabled(enabled);
        Serial.printf("[Twin] detectionEnabled → %s\n", enabled ? "true" : "false");
      }
      
      if (doc.containsKey("cooldownMs")) {
        int cooldown = doc["cooldownMs"];

        // Validation des limites
        if (cooldown < COOLDOWN_MIN_MS || cooldown > COOLDOWN_MAX_MS) {
          Serial.printf("[Twin] ❌ cooldownMs rejeté: %d (limites: %d-%d)\n",
                        cooldown, COOLDOWN_MIN_MS, COOLDOWN_MAX_MS);

          // Publier le rejet dans les propriétés reportées
          char reportedProps[256];
          snprintf(reportedProps, sizeof(reportedProps),
                   "{\"cooldownMs\":{\"value\":%d,\"ac\":400,\"av\":%d,\"ad\":\"Valeur hors limites [%d-%d]\"}}",
                   configGetCooldown(), doc.containsKey("$version") ? (int)doc["$version"] : 0,
                   COOLDOWN_MIN_MS, COOLDOWN_MAX_MS);
          mqttClient.publish(topicTwinPatch, reportedProps);
        } else {
          // Valeur valide : appliquer
          configSetCooldown(cooldown);
          Serial.printf("[Twin] ✅ cooldownMs → %d\n", cooldown);

          // Confirmer dans les propriétés reportées
          char reportedProps[256];
          snprintf(reportedProps, sizeof(reportedProps),
                   "{\"cooldownMs\":{\"value\":%d,\"ac\":200,\"av\":%d,\"ad\":\"Succès\"}}",
                   cooldown, doc.containsKey("$version") ? (int)doc["$version"] : 0);
          mqttClient.publish(topicTwinPatch, reportedProps);
        }
      }

      // Commande OTA : mise à jour du firmware
      if (doc.containsKey("firmwareUrl")) {
        const char* firmwareUrl = doc["firmwareUrl"];
        Serial.printf("[Twin] Commande OTA reçue : %s\n", firmwareUrl);

        // Publier l'état "en cours" dans les propriétés reportées
        char reportedProps[512];
        snprintf(reportedProps, sizeof(reportedProps),
                 "{\"firmwareUrl\":{\"value\":\"%s\",\"ac\":202,\"av\":%d,\"ad\":\"Mise à jour en cours...\"}}",
                 firmwareUrl, doc.containsKey("$version") ? (int)doc["$version"] : 0);
        mqttClient.publish(topicTwinPatch, reportedProps);

        // Démarrer la mise à jour OTA (bloquant, redémarre l'ESP32 en cas de succès)
        bool otaSuccess = otaStartUpdate(firmwareUrl);

        if (!otaSuccess) {
          // Publier l'échec dans les propriétés reportées
          snprintf(reportedProps, sizeof(reportedProps),
                   "{\"firmwareUrl\":{\"value\":\"%s\",\"ac\":500,\"av\":%d,\"ad\":\"Échec: %s\"}}",
                   firmwareUrl, doc.containsKey("$version") ? (int)doc["$version"] : 0,
                   otaGetLastError().c_str());
          mqttClient.publish(topicTwinPatch, reportedProps);
        }
        // Si succès, l'ESP32 redémarre automatiquement
      }

      // Commandes cloud
      if (doc.containsKey("command")) {
        const char* command = doc["command"];
        Serial.printf("[Twin] Commande cloud reçue : %s\n", command);

        char reportedProps[512];
        bool commandSuccess = false;
        String commandResult = "";

        if (strcmp(command, "reboot") == 0) {
          // Commande : redémarrer l'ESP32
          Serial.println("[Command] Redémarrage dans 3 secondes...");
          commandSuccess = true;
          commandResult = "Redémarrage en cours";

          // Publier confirmation
          snprintf(reportedProps, sizeof(reportedProps),
                   "{\"command\":{\"value\":\"%s\",\"ac\":200,\"av\":%d,\"ad\":\"Succès: %s\"}}",
                   command, doc.containsKey("$version") ? (int)doc["$version"] : 0, commandResult.c_str());
          mqttClient.publish(topicTwinPatch, reportedProps);

          delay(3000);
          ESP.restart();

        } else if (strcmp(command, "clearCache") == 0) {
          // Commande : vider le cache DPS et la file de messages
          Serial.println("[Command] Vidage du cache...");

          // Vider la file de messages
          messageQueueClear();

          // Supprimer le fichier de cache DPS
          if (LittleFS.begin(true)) {
            if (LittleFS.exists("/littlefs/dps_hub_cache.txt")) {
              LittleFS.remove("/littlefs/dps_hub_cache.txt");
            }
          }

          commandSuccess = true;
          commandResult = "Cache vidé (file de messages + cache DPS)";

        } else if (strcmp(command, "setDetectionEnabled") == 0) {
          // Commande : activer/désactiver la détection
          bool enabled = doc.containsKey("enabled") ? (bool)doc["enabled"] : true;
          configSetDetectionEnabled(enabled);

          commandSuccess = true;
          commandResult = enabled ? "Détection activée" : "Détection désactivée";
          Serial.printf("[Command] %s\n", commandResult.c_str());

        } else if (strcmp(command, "calibratePIR") == 0) {
          // Commande : calibrer le capteur PIR
          Serial.println("[Command] Calibration du capteur PIR...");
          Serial.println("[Command] Ne pas bouger pendant 30 secondes...");

          // Simulation de calibration (attente de 30 secondes avec reset du watchdog)
          unsigned long calibStart = millis();
          while (!hasElapsed(calibStart, 30000)) {
            delay(100);
            esp_task_wdt_reset(); // Reset watchdog pour éviter le reboot
          }

          commandSuccess = true;
          commandResult = "Calibration terminée";
          Serial.println("[Command] ✅ Calibration terminée");

        } else {
          // Commande inconnue
          commandSuccess = false;
          commandResult = "Commande inconnue: " + String(command);
          Serial.printf("[Command] ❌ %s\n", commandResult.c_str());
        }

        // Publier le résultat dans les propriétés reportées
        if (strcmp(command, "reboot") != 0) { // Ne pas publier pour reboot (déjà fait)
          snprintf(reportedProps, sizeof(reportedProps),
                   "{\"command\":{\"value\":\"%s\",\"ac\":%d,\"av\":%d,\"ad\":\"%s\"}}",
                   command, commandSuccess ? 200 : 400,
                   doc.containsKey("$version") ? (int)doc["$version"] : 0,
                   commandResult.c_str());
          mqttClient.publish(topicTwinPatch, reportedProps);
        }
      }
    }
  }
}

/**
 * Connexion WiFi
 */
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiState = AZURE_WIFI_CONNECTED;
    return true;
  }
  
  if (wifiState != AZURE_WIFI_CONNECTING) {
    Serial.printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiState = AZURE_WIFI_CONNECTING;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiState = AZURE_WIFI_CONNECTED;
    Serial.println("[WiFi] ✅ Connecté");
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    return true;
  }
  
  return false;
}

/**
 * Connexion MQTT à l'IoT Hub
 */
bool connectMqtt() {
  if (mqttClient.connected()) {
    mqttState = AZURE_MQTT_CONNECTED;
    // Reset du backoff en cas de connexion réussie
    mqttRetryCount = 0;
    mqttCurrentBackoff = MQTT_RECONNECT_BASE_DELAY;
    return true;
  }

  // Utiliser le délai de backoff exponentiel (gère le débordement de millis)
  if (!hasElapsed(lastMqttAttempt, mqttCurrentBackoff)) {
    return false;
  }

  lastMqttAttempt = millis();

  Serial.printf("[MQTT] Connexion à %s:%d... (tentative #%d, backoff: %lums)\n",
                iotHubHostname.c_str(), MQTT_PORT, mqttRetryCount + 1, mqttCurrentBackoff);
  
  // Générer SAS Token
  String sasToken = generateSasToken();
  
  // Préparer les topics
  snprintf(topicTelemetry, sizeof(topicTelemetry),
           "devices/%s/messages/events/", deviceId.c_str());
  snprintf(topicTwinGet, sizeof(topicTwinGet),
           "$iothub/twin/GET/?$rid=0");
  snprintf(topicTwinPatch, sizeof(topicTwinPatch),
           "$iothub/twin/PATCH/properties/reported/?$rid=1");
  snprintf(topicTwinRes, sizeof(topicTwinRes),
           "$iothub/twin/res/#");
  
  // Username
  snprintf(mqttUsername, sizeof(mqttUsername),
           "%s/%s/?api-version=2021-04-12",
           iotHubHostname.c_str(), deviceId.c_str());
  
  // Connexion
  if (mqttClient.connect(deviceId.c_str(), mqttUsername, sasToken.c_str())) {
    Serial.println("[MQTT] ✅ Connecté à l'IoT Hub");
    
    // Subscribe aux topics Device Twin
    mqttClient.subscribe("$iothub/twin/PATCH/properties/desired/#");
    mqttClient.subscribe(topicTwinRes);
    
    mqttState = AZURE_MQTT_CONNECTED;
    
    // Publier les propriétés initiales
    char reportedProps[256];
    snprintf(reportedProps, sizeof(reportedProps),
             "{\"firmwareVersion\":\"%s\",\"detectionEnabled\":%s}",
             firmwareVersion.c_str(),
             configGetDetectionEnabled() ? "true" : "false");
    
    mqttClient.publish(topicTwinPatch, reportedProps);
    
    return true;
  } else {
    Serial.printf("[MQTT] ❌ Échec connexion (code: %d)\n", mqttClient.state());
    mqttState = AZURE_MQTT_DISCONNECTED;

    // Incrémenter le compteur et doubler le backoff
    mqttRetryCount++;
    mqttCurrentBackoff = min(mqttCurrentBackoff * 2, MQTT_RECONNECT_MAX_DELAY);

    return false;
  }
}

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void azureSetIotHub(const char* hostname) {
  iotHubHostname = String(hostname);
  deviceId = String(REGISTRATION_ID);
  
  Serial.printf("[Azure] Hub défini: %s\n", hostname);
  Serial.printf("[Azure] DeviceId: %s\n", deviceId.c_str());
}

void azureSetFirmwareVersion(const char* version) {
  firmwareVersion = String(version);
}

void azureInit() {
  Serial.println("[Azure] Initialisation...");

  // Vérifier que le Hub est défini
  if (iotHubHostname.length() == 0) {
    Serial.println("[Azure] ❌ ERREUR: Hub non défini ! Appelez azureSetIotHub() d'abord.");
    return;
  }

  // Initialiser le module OTA
  otaSetup();

  // Initialiser la file de messages
  messageQueueInit();

  // Configuration TLS
  wifiClient.setCACert(AZURE_ROOT_CA);
  
  // Configuration MQTT
  mqttClient.setServer(iotHubHostname.c_str(), MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);
  
  // Connexion WiFi
  connectWiFi();
  
  // Attente connexion WiFi (gère le débordement de millis)
  unsigned long wifiStart = millis();
  while (wifiState != AZURE_WIFI_CONNECTED && !hasElapsed(wifiStart, WIFI_CONNECT_TIMEOUT)) {
    connectWiFi();
    delay(100);
  }
  
  if (wifiState == AZURE_WIFI_CONNECTED) {
    // Synchronisation NTP
    syncTime();
    
    // Connexion MQTT
    connectMqtt();
  }
  
  Serial.println("[Azure] ✅ Initialisation terminée");
}

void azureLoop() {
  // Gestion WiFi
  if (wifiState != AZURE_WIFI_CONNECTED) {
    connectWiFi();
  }
  
  // Gestion MQTT
  if (wifiState == AZURE_WIFI_CONNECTED) {
    if (mqttState != AZURE_MQTT_CONNECTED) {
      connectMqtt();
    } else {
      mqttClient.loop();
      
      // Publication status périodique (gère le débordement de millis)
      if (hasElapsed(lastStatusPublish, STATUS_PUBLISH_INTERVAL)) {
        char statusMsg[256];
        snprintf(statusMsg, sizeof(statusMsg),
                 "{\"deviceId\":\"%s\",\"status\":\"online\",\"uptime\":%lu,\"rssi\":%d,\"freeHeap\":%d}",
                 deviceId.c_str(), millis() / 1000, WiFi.RSSI(), ESP.getFreeHeap());

        mqttClient.publish(topicTelemetry, statusMsg);
        lastStatusPublish = millis();
      }
    }
  }
}

bool azurePublishTelemetry(const char* jsonPayload) {
  // Si MQTT est connecté, envoyer les messages en file d'attente d'abord
  if (mqttState == AZURE_MQTT_CONNECTED) {
    // Envoyer tous les messages en attente
    while (!messageQueueIsEmpty()) {
      char queuedMessage[MESSAGE_MAX_LENGTH];
      if (messageQueuePeek(queuedMessage, sizeof(queuedMessage))) {
        if (mqttClient.publish(topicTelemetry, queuedMessage)) {
          Serial.println("[Azure] ✅ Message en file d'attente publié");
          messageQueuePop();
        } else {
          Serial.println("[Azure] ❌ Échec publication message en file");
          break; // Arrêter si échec
        }
      } else {
        break;
      }
    }

    // Envoyer le nouveau message
    if (mqttClient.publish(topicTelemetry, jsonPayload)) {
      Serial.println("[Azure] ✅ Télémétrie publiée");
      return true;
    } else {
      Serial.println("[Azure] ❌ Échec publication, ajout à la file");
      messageQueuePush(jsonPayload);
      return false;
    }
  } else {
    // MQTT non connecté : ajouter à la file d'attente
    Serial.println("[Azure] ⚠️ MQTT non connecté, ajout à la file d'attente");
    return messageQueuePush(jsonPayload);
  }
}

void azureSetPirCallback(void (*callback)()) {
  pirDetectionCallback = callback;
}