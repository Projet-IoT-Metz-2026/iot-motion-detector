#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <vector>
#include <ctype.h>

#include "secrets.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "esp_task_wdt.h"
#include <esp_system.h>
#include <Preferences.h>

// === MODE DEBUG ===
#define DEBUG_MODE true  // Mettre à false pour production

#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

// === CERTIFICAT AZURE ===
const char* azure_root_ca = \
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

// === CONFIGURATION HARDWARE ===
const int PIR_PIN = 13;
const int LED_PIN = 2;
const unsigned long DEBOUNCE_DELAY = 500;

// === CONFIGURATION SYSTÈME ===
const int WDT_TIMEOUT = 30;
const int MAX_BUFFER_SIZE = 50;
const char* FIRMWARE_VERSION = "2.0.0";

// === STRUCTURES D'ÉTAT ===
struct DeviceConfig {
  bool detectionEnabled = true;
  unsigned long cooldownPeriod = 5000;
  String firmwareVersion = FIRMWARE_VERSION;
};

struct DeviceMetrics {
  unsigned long bootTime = 0;
  int detectionCount = 0;
  int bufferedMessagesCount = 0;
  int sentFromBufferCount = 0;
  unsigned long lastValidDetectionTime = 0;
  int wifiReconnectCount = 0;
  int mqttReconnectCount = 0;
  int failedPublishCount = 0;
};

struct PirState {
  bool lastState = LOW;
  unsigned long lastStateChangeTime = 0;
  bool motionInProgress = false;
};

struct PendingMessage {
  String topic;
  String payload;
  unsigned long timestamp;
};

// === ÉTATS DE CONNEXION ===
enum ConnectionState {
  DISCONNECTED,
  CONNECTING_WIFI,
  WIFI_CONNECTED,
  CONNECTING_MQTT,
  FULLY_CONNECTED
};

// === INSTANCES GLOBALES ===
DeviceConfig config;
DeviceMetrics metrics;
PirState pirState;
ConnectionState connectionState = DISCONNECTED;

// === VARIABLES GLOBALES ===
std::vector<PendingMessage> messageBuffer;
unsigned long lastConnectionAttempt = 0;
unsigned long lastBufferCheck = 0;
unsigned long lastTwinUpdate = 0;
int twinRequestId = 0;

const unsigned long CONNECTION_RETRY_INTERVAL = 5000;
const unsigned long BUFFER_CHECK_INTERVAL = 10000;
const unsigned long TWIN_UPDATE_INTERVAL = 60000;

// === MQTT / Azure ===
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

// === PREFERENCES (EEPROM) ===
Preferences preferences;

// === DÉCLARATIONS FORWARD ===
void handleConnection();
void connectMQTT();
void publishStatus();
void sendBufferedMessages();
void publishTwinReported();
void saveConfig();
void loadConfig();

// ============================================
// FONCTIONS UTILITAIRES AZURE
// ============================================

String urlEncode(const String &s) {
  String o; 
  const char*h="0123456789ABCDEF";
  for (size_t i=0;i<s.length();++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') o+= (char)c;
    else { o+='%'; o+=h[(c>>4)&0xF]; o+=h[c&0xF]; }
  }
  return o;
}

bool base64Decode(const String &in, std::vector<uint8_t> &out) {
  size_t n=0;
  int rc=mbedtls_base64_decode(nullptr,0,&n,(const unsigned char*)in.c_str(),in.length());
  if(rc!=MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) return false;
  out.resize(n);
  rc=mbedtls_base64_decode(out.data(),out.size(),&n,(const unsigned char*)in.c_str(),in.length());
  if(rc!=0) return false;
  out.resize(n);
  return true;
}

String base64Encode(const uint8_t*data,size_t len){
  size_t outLen = 0;
  int rc = mbedtls_base64_encode(nullptr, 0, &outLen, data, len);
  if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
    return "";
  }

  std::vector<uint8_t> out(outLen);
  rc = mbedtls_base64_encode(out.data(), out.size(), &outLen, data, len);
  if (rc != 0) {
    return "";
  }

  return String(reinterpret_cast<const char*>(out.data()), outLen);
}

bool hmacSha256(const std::vector<uint8_t>&key,const uint8_t*msg,size_t len,uint8_t out[32]){
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if(!md) return false;
  return mbedtls_md_hmac(md, key.data(), key.size(), msg, len, out) == 0;
}

bool waitForTime(uint32_t ms=10000){
  uint32_t s=millis();
  time_t now=0;
  while(millis()-s<ms){ 
    time(&now); 
    if(now>1700000000) return true; 
    delay(200); 
    esp_task_wdt_reset();
  }
  return false;
}

String buildSasToken(const String&host,const String&dev,const String&keyB64,uint32_t ttl){
  String res=host+"/devices/"+dev; 
  res.toLowerCase();
  time_t now; 
  time(&now); 
  uint32_t exp=(uint32_t)now+ttl;
  String toSign=urlEncode(res)+"\n"+String(exp);

  std::vector<uint8_t> key;
  if(!base64Decode(keyB64,key)){ 
    DEBUG_PRINTLN("[AZURE] Clé Base64 invalide"); 
    return ""; 
  }
  uint8_t mac[32];
  if(!hmacSha256(key,(const uint8_t*)toSign.c_str(),toSign.length(),mac)){ 
    DEBUG_PRINTLN("[AZURE] HMAC échec"); 
    return ""; 
  }
  String sig=urlEncode(base64Encode(mac,sizeof(mac)));
  return "SharedAccessSignature sr="+urlEncode(res)+"&sig="+sig+"&se="+String(exp);
}

// ============================================
// FONCTIONS CONFIGURATION (EEPROM)
// ============================================

void saveConfig() {
  preferences.begin("iot-detector", false);
  preferences.putBool("detectionEnabled", config.detectionEnabled);
  preferences.putULong("cooldown", config.cooldownPeriod);
  preferences.end();
  DEBUG_PRINTLN("[CONFIG] ✅ Sauvegardé en EEPROM");
}

void loadConfig() {
  preferences.begin("iot-detector", true);
  config.detectionEnabled = preferences.getBool("detectionEnabled", true);
  config.cooldownPeriod = preferences.getULong("cooldown", 5000);
  preferences.end();
  DEBUG_PRINTF("[CONFIG] ✅ Chargé: detectionEnabled=%s, cooldown=%lu ms\n", 
               config.detectionEnabled ? "true" : "false", 
               config.cooldownPeriod);
}

// ============================================
// FONCTIONS BUFFER
// ============================================

void addToBuffer(const String& topic, const String& payload) {
  if (messageBuffer.size() >= MAX_BUFFER_SIZE) {
    DEBUG_PRINTLN("[BUFFER] ⚠️ Buffer plein, suppression du plus ancien");
    messageBuffer.erase(messageBuffer.begin());
  }
  
  PendingMessage msg;
  msg.topic = topic;
  msg.payload = payload;
  msg.timestamp = millis();
  
  messageBuffer.push_back(msg);
  metrics.bufferedMessagesCount++;
  
  DEBUG_PRINTF("[BUFFER] Message ajouté (#%d en attente)\n", messageBuffer.size());
}

void sendBufferedMessages() {
  if (messageBuffer.empty()) {
    return;
  }
  
  if (connectionState != FULLY_CONNECTED) {
    DEBUG_PRINTLN("[BUFFER] Pas connecté, impossible d'envoyer");
    return;
  }
  
  DEBUG_PRINTF("[BUFFER] 📤 Envoi de %d messages en attente...\n", messageBuffer.size());
  
  std::vector<PendingMessage> toSend = messageBuffer;
  messageBuffer.clear();
  
  int successCount = 0;
  int failCount = 0;
  
  for (size_t i = 0; i < toSend.size(); i++) {
    bool ok = mqtt.publish(toSend[i].topic.c_str(), toSend[i].payload.c_str());
    
    if (ok) {
      successCount++;
      metrics.sentFromBufferCount++;
      DEBUG_PRINTF("[BUFFER] ✅ %d/%d envoyé\n", i+1, toSend.size());
      delay(100);
    } else {
      failCount++;
      DEBUG_PRINTF("[BUFFER] ❌ %d/%d échoué\n", i+1, toSend.size());
      addToBuffer(toSend[i].topic, toSend[i].payload);
    }
    
    esp_task_wdt_reset();
  }
  
  DEBUG_PRINTF("[BUFFER] 📊 Résumé: %d ✅, %d ❌\n", successCount, failCount);
}

// ============================================
// FONCTIONS PUBLICATION (AVEC ARDUINOJSON)
// ============================================

void publishDetectionJson(){
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  StaticJsonDocument<512> doc;
  
  doc["event"] = "motion";
  doc["count"] = metrics.detectionCount;
  doc["ts"] = millis();
  
  JsonObject configObj = doc.createNestedObject("config");
  configObj["detectionEnabled"] = config.detectionEnabled;
  configObj["cooldown"] = config.cooldownPeriod;
  configObj["firmware"] = config.firmwareVersion;
  
  JsonObject system = doc.createNestedObject("system");
  system["rssi"] = WiFi.RSSI();
  system["freeHeap"] = ESP.getFreeHeap();
  system["uptime"] = millis() / 1000;
  system["cpuFreq"] = ESP.getCpuFreqMHz();
  system["buffered"] = metrics.bufferedMessagesCount;
  system["sentFromBuffer"] = metrics.sentFromBufferCount;
  system["wifiReconnects"] = metrics.wifiReconnectCount;
  system["mqttReconnects"] = metrics.mqttReconnectCount;
  
  String payload;
  serializeJson(doc, payload);
  
  if(connectionState != FULLY_CONNECTED) {
    DEBUG_PRINTLN("[MQTT] Déconnecté, ajout au buffer");
    addToBuffer(topic, payload);
  } else {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    
    if (ok) {
      DEBUG_PRINTF("[MQTT] Publish ✅ OK : %s\n", payload.c_str());
      
      if (!messageBuffer.empty()) {
        sendBufferedMessages();
      }
    } else {
      DEBUG_PRINTLN("[MQTT] ❌ Publish échoué, ajout au buffer");
      metrics.failedPublishCount++;
      addToBuffer(topic, payload);
    }
  }
}

void publishStatus() {
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  StaticJsonDocument<512> doc;
  
  doc["event"] = "status";
  doc["firmware"] = config.firmwareVersion;
  doc["uptime"] = millis() / 1000;
  doc["detectionEnabled"] = config.detectionEnabled;
  doc["cooldown"] = config.cooldownPeriod;
  doc["detectionCount"] = metrics.detectionCount;
  
  JsonObject system = doc.createNestedObject("system");
  system["rssi"] = WiFi.RSSI();
  system["freeHeap"] = ESP.getFreeHeap();
  system["buffered"] = messageBuffer.size();
  system["wifiReconnects"] = metrics.wifiReconnectCount;
  system["mqttReconnects"] = metrics.mqttReconnectCount;
  system["failedPublishes"] = metrics.failedPublishCount;
  
  String payload;
  serializeJson(doc, payload);
  
  if (connectionState == FULLY_CONNECTED) {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    DEBUG_PRINTF("[STATUS] Publish %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

void publishTwinReported() {
  String topic = "$iothub/twin/PATCH/properties/reported/?$rid=" + String(twinRequestId++);
  
  StaticJsonDocument<512> doc;
  doc["firmware"] = config.firmwareVersion;
  doc["uptime"] = millis() / 1000;
  doc["detectionEnabled"] = config.detectionEnabled;
  doc["cooldown"] = config.cooldownPeriod;
  doc["detectionCount"] = metrics.detectionCount;
  
  JsonObject system = doc.createNestedObject("system");
  system["rssi"] = WiFi.RSSI();
  system["freeHeap"] = ESP.getFreeHeap();
  system["cpuFreq"] = ESP.getCpuFreqMHz();
  system["buffered"] = messageBuffer.size();
  
  String payload;
  serializeJson(doc, payload);
  
  if (connectionState == FULLY_CONNECTED) {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    DEBUG_PRINTF("[TWIN] Reported %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

void requestTwinGet() {
  String topic = "$iothub/twin/GET/?$rid=" + String(twinRequestId++);
  
  if (connectionState == FULLY_CONNECTED) {
    bool ok = mqtt.publish(topic.c_str(), "");
    DEBUG_PRINTF("[TWIN] GET request %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

// ============================================
// FONCTIONS DEVICE TWIN
// ============================================

void handleTwinDesired(const JsonObject& doc) {
  DEBUG_PRINTLN("\n╔═══════════════════════════════════════╗");
  DEBUG_PRINTLN("║  🔄 DEVICE TWIN DESIRED UPDATE       ║");
  DEBUG_PRINTLN("╚═══════════════════════════════════════╝");
  
  bool changed = false;
  
  if (doc.containsKey("detectionEnabled")) {
    bool newValue = doc["detectionEnabled"];
    if (newValue != config.detectionEnabled) {
      config.detectionEnabled = newValue;
      DEBUG_PRINTF("[TWIN] detectionEnabled: %s → %s\n", 
                    !newValue ? "true" : "false",
                    newValue ? "true" : "false");
      
      if (config.detectionEnabled) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
      } else {
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(100);
          digitalWrite(LED_PIN, LOW);
          delay(100);
        }
      }
      
      changed = true;
    }
  }
  
  if (doc.containsKey("cooldown")) {
    unsigned long newValue = doc["cooldown"];
    if (newValue >= 1000 && newValue <= 60000 && newValue != config.cooldownPeriod) {
      DEBUG_PRINTF("[TWIN] cooldown: %lu ms → %lu ms\n", 
                    config.cooldownPeriod, newValue);
      config.cooldownPeriod = newValue;
      changed = true;
    }
  }
  
  if (changed) {
    DEBUG_PRINTLN("[TWIN] ✅ Configuration mise à jour depuis Azure");
    saveConfig();
    publishTwinReported();
  } else {
    DEBUG_PRINTLN("[TWIN] ℹ️ Aucun changement nécessaire");
  }
  
  DEBUG_PRINTLN("───────────────────────────────────────");
}

// ============================================
// CALLBACK MQTT
// ============================================

void messageCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // TRAITER DEVICE TWIN DESIRED
  if (topicStr.startsWith("$iothub/twin/PATCH/properties/desired/")) {
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════╗");
    DEBUG_PRINTLN("║  📨 TWIN DESIRED PATCH REÇU !         ║");
    DEBUG_PRINTLN("╚═══════════════════════════════════════╝");
    DEBUG_PRINTF("[TWIN] Payload: %s\n", message.c_str());
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      DEBUG_PRINTF("[TWIN] ❌ Erreur parsing JSON: %s\n", error.c_str());
      return;
    }
    
    handleTwinDesired(doc.as<JsonObject>());
    return;
  }
  
  // TRAITER TWIN RESPONSE
  if (topicStr.startsWith("$iothub/twin/res/")) {
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════╗");
    DEBUG_PRINTLN("║  📋 TWIN GET RESPONSE REÇUE !         ║");
    DEBUG_PRINTLN("╚═══════════════════════════════════════╝");
    
    int statusStart = topicStr.indexOf("/res/") + 5;
    int statusEnd = topicStr.indexOf("/", statusStart);
    String statusCode = topicStr.substring(statusStart, statusEnd);
    
    DEBUG_PRINTF("[TWIN] Status Code: %s\n", statusCode.c_str());
    
    if (statusCode == "200") {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (!error) {
        if (doc.containsKey("desired")) {
          JsonObject desired = doc["desired"];
          DEBUG_PRINTLN("[TWIN] Propriétés desired trouvées:");
          serializeJsonPretty(desired, Serial);
          Serial.println();
          
          handleTwinDesired(desired);
        }
      }
    }
    
    DEBUG_PRINTLN("───────────────────────────────────────");
    return;
  }
  
  // TRAITER C2D MESSAGES
  if (topicStr.startsWith("devices/")) {
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════╗");
    DEBUG_PRINTLN("║  📨 MESSAGE C2D REÇU DEPUIS AZURE !   ║");
    DEBUG_PRINTLN("╚═══════════════════════════════════════╝");
    
    DEBUG_PRINTF("[C2D] Payload: %s\n", message.c_str());
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      DEBUG_PRINTF("[C2D] ❌ Erreur parsing JSON: %s\n", error.c_str());
      return;
    }
    
    const char* command = doc["command"];
    
    if (command == nullptr) {
      DEBUG_PRINTLN("[C2D] ❌ Aucune commande trouvée");
      return;
    }
    
    DEBUG_PRINTF("[C2D] 🎯 Commande: %s\n", command);
    
    if (strcmp(command, "enable") == 0) {
      config.detectionEnabled = true;
      DEBUG_PRINTLN("[C2D] ✅ Détection ACTIVÉE");
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      saveConfig();
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "disable") == 0) {
      config.detectionEnabled = false;
      DEBUG_PRINTLN("[C2D] ⛔ Détection DÉSACTIVÉE");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      saveConfig();
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "setCooldown") == 0) {
      if (doc.containsKey("value")) {
        unsigned long newCooldown = doc["value"];
        if (newCooldown >= 1000 && newCooldown <= 60000) {
          config.cooldownPeriod = newCooldown;
          DEBUG_PRINTF("[C2D] ✅ Cooldown changé: %lu ms\n", config.cooldownPeriod);
          saveConfig();
          publishStatus();
          publishTwinReported();
        }
      }
      
    } else if (strcmp(command, "getStatus") == 0) {
      DEBUG_PRINTLN("[C2D] 📊 Envoi du statut...");
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "getTwin") == 0) {
      DEBUG_PRINTLN("[C2D] 🔍 Demande du Device Twin...");
      requestTwinGet();
      
    } else if (strcmp(command, "reboot") == 0) {
      DEBUG_PRINTLN("[C2D] 🔄 REDÉMARRAGE dans 3 secondes...");
      delay(3000);
      ESP.restart();
      
    } else if (strcmp(command, "clearBuffer") == 0) {
      messageBuffer.clear();
      DEBUG_PRINTLN("[C2D] ✅ Buffer vidé");
      publishStatus();
      
    } else {
      DEBUG_PRINTF("[C2D] ❌ Commande inconnue: %s\n", command);
    }
    
    DEBUG_PRINTLN("───────────────────────────────────────");
  }
}

// ============================================
// FONCTIONS CONNEXION (MACHINE À ÉTATS)
// ============================================

void connectMQTT() {
  DEBUG_PRINTLN("[TLS] Configuration du certificat Azure IoT Hub...");
  tlsClient.setCACert(azure_root_ca);
  
  mqtt.setCallback(messageCallback);
  mqtt.setServer(IOTHUB_HOST, 8883);

  DEBUG_PRINTLN("[NTP] Synchronisation de l'heure...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  if (!waitForTime()) {
      DEBUG_PRINTLN("[NTP] ❌ Échec de la synchronisation.");
      return;
  } else {
      time_t now;
      time(&now);
      DEBUG_PRINTF("[NTP] ✅ Heure synchronisée : %s", ctime(&now));
  }

  String sas = buildSasToken(IOTHUB_HOST, IOTHUB_DEVICE_ID, IOTHUB_DEVICE_KEY_BASE64, 3600);
  String clientId = IOTHUB_DEVICE_ID;
  String username = String(IOTHUB_HOST) + "/" + IOTHUB_DEVICE_ID + "/?api-version=2020-09-30";

  DEBUG_PRINTLN("\n--- Informations de Connexion MQTT ---");
  DEBUG_PRINTLN("Client ID: " + clientId);
  DEBUG_PRINTLN("Username: " + username);
  DEBUG_PRINTLN("SAS Token: " + sas);
  DEBUG_PRINTLN("-------------------------------------\n");

  DEBUG_PRINTLN("[MQTT] Connexion à IoT Hub...");
  if (!mqtt.connect(clientId.c_str(), username.c_str(), sas.c_str())) {
    DEBUG_PRINTF("[MQTT] ❌ Échec, rc=%d\n", mqtt.state());
    return;
  }
  
  DEBUG_PRINTLN("[MQTT] ✅ Connecté à IoT Hub");
  
  String subscribeC2D = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/devicebound/#";
  if (mqtt.subscribe(subscribeC2D.c_str())) {
    DEBUG_PRINTLN("[C2D] ✅ Abonné aux messages Cloud-to-Device");
  }
  
  if (mqtt.subscribe("$iothub/twin/PATCH/properties/desired/#")) {
    DEBUG_PRINTLN("[TWIN] ✅ Abonné aux PATCH desired");
  }
  
  if (mqtt.subscribe("$iothub/twin/res/#")) {
    DEBUG_PRINTLN("[TWIN] ✅ Abonné aux réponses twin");
  }
  
  delay(1000);
  
  requestTwinGet();
  publishTwinReported();
  publishStatus();
}

void handleConnection() {
  unsigned long now = millis();
  
  switch(connectionState) {
    case DISCONNECTED:
      if (now - lastConnectionAttempt > CONNECTION_RETRY_INTERVAL) {
        DEBUG_PRINTLN("[CONN] ⚡ Tentative de connexion WiFi...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        connectionState = CONNECTING_WIFI;
        lastConnectionAttempt = now;
      }
      break;
      
    case CONNECTING_WIFI:
      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTF("\n[WiFi] ✅ Connecté, IP: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
        connectionState = WIFI_CONNECTED;
        lastConnectionAttempt = now;
      } else if (now - lastConnectionAttempt > 20000) {
        DEBUG_PRINTLN("\n[WiFi] ❌ Timeout");
        WiFi.disconnect();
        connectionState = DISCONNECTED;
        metrics.wifiReconnectCount++;
      }
      break;
      
    case WIFI_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("[WiFi] ⚠️ Déconnecté");
        connectionState = DISCONNECTED;
      } else if (!mqtt.connected()) {
        DEBUG_PRINTLN("[MQTT] Tentative de connexion...");
        connectMQTT();
        connectionState = CONNECTING_MQTT;
        lastConnectionAttempt = now;
      }
      break;
      
    case CONNECTING_MQTT:
      if (mqtt.connected()) {
        DEBUG_PRINTLN("[MQTT] ✅ État: FULLY_CONNECTED");
        connectionState = FULLY_CONNECTED;
        
        // Clignotement LED pour signaler la connexion complète
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(100);
          digitalWrite(LED_PIN, LOW);
          delay(100);
        }
        
        // Envoyer les messages en attente
        if (!messageBuffer.empty()) {
          sendBufferedMessages();
        }
      } else if (now - lastConnectionAttempt > 15000) {
        DEBUG_PRINTLN("[MQTT] ❌ Timeout");
        connectionState = WIFI_CONNECTED;
        metrics.mqttReconnectCount++;
      }
      break;
      
    case FULLY_CONNECTED:
      if (!mqtt.connected()) {
        DEBUG_PRINTLN("[MQTT] ⚠️ Déconnecté");
        connectionState = WIFI_CONNECTED;
      } else if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("[WiFi] ⚠️ Déconnecté");
        connectionState = DISCONNECTED;
      }
      break;
  }
}

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n╔═══════════════════════════════════════╗");
  Serial.println("║   ESP32 - Azure IoT Hub PIR Sensor    ║");
  Serial.printf("║   Firmware: %-25s ║\n", FIRMWARE_VERSION);
  Serial.println("╚═══════════════════════════════════════╝\n");
  
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  DEBUG_PRINTLN("[WDT] Configuration du watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  DEBUG_PRINTLN("[CONFIG] Chargement de la configuration...");
  loadConfig();
  
  metrics.bootTime = millis();
  
  DEBUG_PRINTLN("[SYSTEM] ✅ Initialisation terminée\n");
  DEBUG_PRINTF("[SYSTEM] Mode DEBUG: %s\n", DEBUG_MODE ? "ACTIVÉ" : "DÉSACTIVÉ");
  DEBUG_PRINTF("[SYSTEM] RAM libre: %d bytes\n", ESP.getFreeHeap());
  DEBUG_PRINTLN("\n[CONN] Démarrage de la connexion...\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  esp_task_wdt_reset();
  
  // Gestion de la connexion (non-bloquante)
  handleConnection();
  
  // Traiter les messages MQTT seulement si connecté
  if (connectionState == FULLY_CONNECTED) {
    mqtt.loop();
  }
  
  // Vérifier et envoyer les messages en buffer périodiquement
  if (millis() - lastBufferCheck > BUFFER_CHECK_INTERVAL) {
    lastBufferCheck = millis();
    if (!messageBuffer.empty() && connectionState == FULLY_CONNECTED) {
      sendBufferedMessages();
    }
  }
  
  // Mise à jour périodique du Device Twin
  if (millis() - lastTwinUpdate > TWIN_UPDATE_INTERVAL) {
    lastTwinUpdate = millis();
    if (connectionState == FULLY_CONNECTED) {
      publishTwinReported();
    }
  }
  
  // === LOGIQUE PIR (FONCTIONNE MÊME SI DÉCONNECTÉ) ===
  if (!config.detectionEnabled) {
    delay(100);
    return;
  }
  
  bool currentState = digitalRead(PIR_PIN);
  unsigned long now = millis();
  
  // Détection du front montant (début de mouvement)
  if (currentState == HIGH && pirState.lastState == LOW) {
    if ((now - pirState.lastStateChangeTime) > DEBOUNCE_DELAY) {
      if (!pirState.motionInProgress) {
        if ((now - metrics.lastValidDetectionTime) > config.cooldownPeriod) {
          pirState.motionInProgress = true;
          metrics.detectionCount++;
          metrics.lastValidDetectionTime = now;
          
          DEBUG_PRINTLN("\n╔═══════════════════════════════════════╗");
          DEBUG_PRINTF("║  🚨 DÉTECTION #%-4d                  ║\n", metrics.detectionCount);
          DEBUG_PRINTLN("╚═══════════════════════════════════════╝");
          
          digitalWrite(LED_PIN, HIGH);
          
          publishDetectionJson();
          
          DEBUG_PRINTLN("───────────────────────────────────────\n");
        } else {
          DEBUG_PRINTF("[PIR] ⏳ Cooldown actif (%lu ms restant)\n", 
                        config.cooldownPeriod - (now - metrics.lastValidDetectionTime));
        }
      }
    }
    pirState.lastStateChangeTime = now;
  }
  
  // Détection du front descendant (fin de mouvement)
  if (currentState == LOW && pirState.lastState == HIGH) {
    if ((now - pirState.lastStateChangeTime) > DEBOUNCE_DELAY) {
      if (pirState.motionInProgress) {
        pirState.motionInProgress = false;
        digitalWrite(LED_PIN, LOW);
        DEBUG_PRINTLN("[PIR] ✅ Mouvement terminé");
      }
    }
    pirState.lastStateChangeTime = now;
  }
  
  pirState.lastState = currentState;
  
  delay(50);
}