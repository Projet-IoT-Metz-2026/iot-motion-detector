// src/dps_handler.cpp
#include "dps_handler.h"
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

// ============================================
// CONSTANTES
// ============================================

static const char* DPS_CACHE_FILE = "/dps_hub_cache.txt";
static const int DPS_MQTT_PORT = 8883;
static const unsigned long DPS_TIMEOUT_MS = 30000;

// Certificat DigiCert Global Root G2 (pour DPS)
static const char* DPS_ROOT_CA = \
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
// VARIABLES GLOBALES
// ============================================

static WiFiClientSecure dpsWifiClient;
static PubSubClient dpsMqttClient(dpsWifiClient);

static String cachedHubHostname = "";
static bool assignmentReceived = false;
static bool assignmentPending = false;
static String assignedHub = "";
static String assignedDeviceId = "";
static String operationId = "";

// ============================================
// FONCTIONS UTILITAIRES
// ============================================

/**
 * Génère un SAS Token pour le DPS
 */
String generateDpsSasToken() {
  const char* idScope = DPS_ID_SCOPE;
  const char* registrationId = REGISTRATION_ID;
  const char* deviceKey = DEVICE_KEY;
  
  // Calcul expiry (24h)
  time_t now = time(nullptr);
  time_t expiry = now + 86400;
  
  // String to sign: {idScope}/registrations/{registrationId}\n{expiry}
  char stringToSign[256];
  snprintf(stringToSign, sizeof(stringToSign),
           "%s/registrations/%s\n%ld",
           idScope, registrationId, expiry);
  
  // Decode device key (base64)
  uint8_t decodedKey[64];
  size_t decodedLen;
  mbedtls_base64_decode(decodedKey, sizeof(decodedKey), &decodedLen,
                        (const unsigned char*)deviceKey, strlen(deviceKey));
  
  // HMAC-SHA256
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, decodedKey, decodedLen);
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign, strlen(stringToSign));
  mbedtls_md_hmac_finish(&ctx, hash);
  mbedtls_md_free(&ctx);
  
  // Encode signature en base64
  char signature[64];
  size_t sigLen;
  mbedtls_base64_encode((unsigned char*)signature, sizeof(signature), &sigLen,
                        hash, sizeof(hash));
  signature[sigLen] = '\0';
  
  // URL encode de la signature
  String encodedSig = "";
  for (size_t i = 0; i < sigLen; i++) {
    char c = signature[i];
    if (c == '+') encodedSig += "%2B";
    else if (c == '/') encodedSig += "%2F";
    else if (c == '=') encodedSig += "%3D";
    else encodedSig += c;
  }
  
  // Format final du SAS Token pour DPS (sans skn pour symmetric key)
  // NOTE: Le sr ne doit PAS être URL-encodé (contrairement à ce qu'on pensait)
  char sasToken[512];
  snprintf(sasToken, sizeof(sasToken),
           "SharedAccessSignature sr=%s/registrations/%s&sig=%s&se=%ld",
           idScope, registrationId, encodedSig.c_str(), expiry);

  return String(sasToken);
}

/**
 * Callback MQTT pour les messages DPS
 */
void dpsMqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[DPS] Message reçu sur topic: %s\n", topic);

  // Afficher la payload brute
  Serial.print("[DPS] Payload: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Parse JSON response
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.printf("[DPS] ❌ Erreur parsing JSON: %s\n", error.c_str());
    return;
  }

  const char* status = doc["status"] | "";
  const char* opId = doc["operationId"] | "";

  Serial.printf("[DPS] Status: %s\n", status);

  // Si "assigned", récupérer le Hub
  if (strcmp(status, "assigned") == 0) {
    JsonObject regState = doc["registrationState"];
    if (regState) {
      const char* hub = regState["assignedHub"] | "";
      const char* devId = regState["deviceId"] | "";

      if (strlen(hub) > 0) {
        assignedHub = String(hub);
        assignedDeviceId = String(devId);
        assignmentReceived = true;
        assignmentPending = false;

        Serial.println("[DPS] ✅ Assignment reçu !");
        Serial.printf("[DPS]    Hub: %s\n", hub);
        Serial.printf("[DPS]    DeviceId: %s\n", devId);
      }
    }
  } else if (strcmp(status, "assigning") == 0) {
    Serial.println("[DPS] ⏳ Assignment en cours...");
    if (strlen(opId) > 0) {
      operationId = String(opId);
      assignmentPending = true;
      Serial.printf("[DPS]    OperationId: %s\n", opId);
    }
  } else {
    Serial.printf("[DPS] ⚠️  Status inattendu: %s\n", status);
  }
}

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void dpsInit() {
  Serial.println("[DPS] Initialisation du module DPS...");
  
  // Init LittleFS pour le cache
  if (!LittleFS.begin(true)) {
    Serial.println("[DPS] ⚠️  LittleFS non disponible");
  }
  
  // Charger le cache si disponible
  if (LittleFS.exists(DPS_CACHE_FILE)) {
    File f = LittleFS.open(DPS_CACHE_FILE, "r");
    if (f) {
      cachedHubHostname = f.readStringUntil('\n');
      cachedHubHostname.trim();
      f.close();
      Serial.printf("[DPS] 📦 Hub en cache: %s\n", cachedHubHostname.c_str());
    }
  }
  
  Serial.println("[DPS] ✅ Module initialisé");
}

bool dpsHasCachedHub() {
  return cachedHubHostname.length() > 0;
}

String dpsGetCachedHub() {
  return cachedHubHostname;
}

void dpsSaveHub(const char* hostname) {
  File f = LittleFS.open(DPS_CACHE_FILE, "w");
  if (f) {
    f.println(hostname);
    f.close();
    cachedHubHostname = String(hostname);
    Serial.printf("[DPS] 💾 Hub sauvegardé: %s\n", hostname);
  }
}

void dpsClearCache() {
  LittleFS.remove(DPS_CACHE_FILE);
  cachedHubHostname = "";
  Serial.println("[DPS] 🗑️  Cache effacé");
}

DpsAssignment dpsProvision() {
  DpsAssignment result;
  result.success = false;
  result.iotHubHostname[0] = '\0';
  result.deviceId[0] = '\0';
  
  Serial.println("\n[DPS] 🚀 Démarrage du provisioning...");
  
  // Vérifier que le WiFi est connecté
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[DPS] ❌ WiFi non connecté ! Impossible de provisionner.");
    return result;
  }
  
  Serial.printf("[DPS] ✅ WiFi connecté (IP: %s)\n", WiFi.localIP().toString().c_str());
  
  // Reset variables
  assignmentReceived = false;
  assignmentPending = false;
  assignedHub = "";
  assignedDeviceId = "";
  operationId = "";
  
  // Configuration TLS
  dpsWifiClient.setCACert(DPS_ROOT_CA);
  
  // Configuration MQTT
  dpsMqttClient.setServer(DPS_GLOBAL_ENDPOINT, DPS_MQTT_PORT);
  dpsMqttClient.setCallback(dpsMqttCallback);
  dpsMqttClient.setBufferSize(2048);
  
  // Username et ClientId pour DPS
  const char* idScope = DPS_ID_SCOPE;
  const char* registrationId = REGISTRATION_ID;
  
  char username[256];
  snprintf(username, sizeof(username), 
           "%s/registrations/%s/api-version=2019-03-31",
           idScope, registrationId);
  
  String sasToken = generateDpsSasToken();
  
  Serial.printf("[DPS] Connexion à %s:%d\n", DPS_GLOBAL_ENDPOINT, DPS_MQTT_PORT);
  Serial.printf("[DPS] Username: %s\n", username);
  Serial.printf("[DPS] ClientId: %s\n", registrationId);
  
  // Connexion MQTT
  if (!dpsMqttClient.connect(registrationId, username, sasToken.c_str())) {
    Serial.printf("[DPS] ❌ Échec connexion MQTT (code: %d)\n", dpsMqttClient.state());
    return result;
  }
  
  Serial.println("[DPS] ✅ Connecté au DPS");
  
  // Subscribe au topic de réponse
  char subTopic[256];
  snprintf(subTopic, sizeof(subTopic),
           "$dps/registrations/res/#");
  
  if (!dpsMqttClient.subscribe(subTopic)) {
    Serial.println("[DPS] ❌ Échec subscription");
    dpsMqttClient.disconnect();
    return result;
  }
  
  Serial.printf("[DPS] ✅ Subscribed: %s\n", subTopic);
  
  // Publier la requête de provisioning
  char pubTopic[256];
  snprintf(pubTopic, sizeof(pubTopic),
           "$dps/registrations/PUT/iotdps-register/?$rid=1");
  
  const char* payload = "{\"registrationId\":\"%s\"}";
  char jsonPayload[128];
  snprintf(jsonPayload, sizeof(jsonPayload), payload, registrationId);
  
  Serial.println("[DPS] 📤 Envoi de la requête de provisioning...");
  
  if (!dpsMqttClient.publish(pubTopic, jsonPayload)) {
    Serial.println("[DPS] ❌ Échec publication");
    dpsMqttClient.disconnect();
    return result;
  }
  
  Serial.println("[DPS] ✅ Requête envoyée");
  
  // Attendre la réponse
  Serial.println("[DPS] ⏳ Attente de l'assignment...");

  unsigned long startTime = millis();
  unsigned long lastPoll = 0;
  int ridCounter = 2; // $rid=1 était pour la requête PUT initiale

  while (!assignmentReceived && (millis() - startTime < DPS_TIMEOUT_MS)) {
    dpsMqttClient.loop();
    esp_task_wdt_reset(); // Réinitialiser le watchdog pendant l'attente

    // Si on est en mode "assigning", poller le statut toutes les 3 secondes
    if (assignmentPending && operationId.length() > 0 && (millis() - lastPoll > 3000)) {
      Serial.println("[DPS] 🔄 Polling du statut...");

      char pollTopic[256];
      snprintf(pollTopic, sizeof(pollTopic),
               "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=%d&operationId=%s",
               ridCounter++, operationId.c_str());

      if (dpsMqttClient.publish(pollTopic, "")) {
        Serial.println("[DPS] ✅ Requête GET envoyée");
        lastPoll = millis();
      } else {
        Serial.println("[DPS] ❌ Échec envoi GET");
      }
    }

    delay(100);
  }
  
  dpsMqttClient.disconnect();
  
  if (assignmentReceived) {
    strncpy(result.iotHubHostname, assignedHub.c_str(), sizeof(result.iotHubHostname) - 1);
    strncpy(result.deviceId, assignedDeviceId.c_str(), sizeof(result.deviceId) - 1);
    result.success = true;
    
    // Sauvegarder en cache
    dpsSaveHub(assignedHub.c_str());
    
    Serial.println("[DPS] ✅ Provisioning réussi !");
    Serial.printf("[DPS]    Hub: %s\n", result.iotHubHostname);
    Serial.printf("[DPS]    DeviceId: %s\n", result.deviceId);
  } else {
    Serial.println("[DPS] ❌ Timeout - pas d'assignment reçu");
  }
  
  return result;
}