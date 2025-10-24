// src/azure_handler.cpp
#include "azure_handler.h"
#include "secrets.h"
#include "config_store.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

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
static const unsigned long MQTT_RECONNECT_DELAY = 5000;
static const unsigned long STATUS_PUBLISH_INTERVAL = 60000;

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
  while (time(nullptr) < 1000000000 && (millis() - start < 15000)) {
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
  mbedtls_base64_decode(decodedKey, sizeof(decodedKey), &decodedLen,
                        (const unsigned char*)DEVICE_KEY, strlen(DEVICE_KEY));
  
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
        configSetCooldown(cooldown);
        Serial.printf("[Twin] cooldownMs → %d\n", cooldown);
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
    return true;
  }
  
  if (millis() - lastMqttAttempt < MQTT_RECONNECT_DELAY) {
    return false;
  }
  
  lastMqttAttempt = millis();
  
  Serial.printf("[MQTT] Connexion à %s:%d...\n", iotHubHostname.c_str(), MQTT_PORT);
  
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
  
  // Configuration TLS
  wifiClient.setCACert(AZURE_ROOT_CA);
  
  // Configuration MQTT
  mqttClient.setServer(iotHubHostname.c_str(), MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);
  
  // Connexion WiFi
  connectWiFi();
  
  // Attente connexion WiFi
  unsigned long wifiStart = millis();
  while (wifiState != AZURE_WIFI_CONNECTED && (millis() - wifiStart < WIFI_CONNECT_TIMEOUT)) {
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
      
      // Publication status périodique
      if (millis() - lastStatusPublish > STATUS_PUBLISH_INTERVAL) {
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
  if (mqttState != AZURE_MQTT_CONNECTED) {
    Serial.println("[Azure] ⚠️ MQTT non connecté, impossible de publier");
    return false;
  }
  
  if (mqttClient.publish(topicTelemetry, jsonPayload)) {
    Serial.println("[Azure] ✅ Télémétrie publiée");
    return true;
  } else {
    Serial.println("[Azure] ❌ Échec publication");
    return false;
  }
}

void azureSetPirCallback(void (*callback)()) {
  pirDetectionCallback = callback;
}