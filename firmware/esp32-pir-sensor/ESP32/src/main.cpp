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

// === CONFIGURATION PIR ===
const int PIR_PIN = 13;
const int LED_PIN = 2;
const unsigned long DEBOUNCE_DELAY = 500;

// === CONFIGURATION SYSTÈME ===
const int WDT_TIMEOUT = 30;
const int MAX_BUFFER_SIZE = 50;
const char* FIRMWARE_VERSION = "1.9.0";

// === VARIABLES GLOBALES PIR ===
bool lastPirState = LOW;
unsigned long lastStateChangeTime = 0;
unsigned long lastValidDetectionTime = 0;
int detectionCount = 0;
bool motionInProgress = false;

// === VARIABLES GLOBALES CONFIGURATION (MODIFIABLES) ===
bool detectionEnabled = true;
unsigned long cooldownPeriod = 5000;

// === VARIABLES GLOBALES MÉTRIQUES ===
unsigned long bootTime = 0;

// === STRUCTURES ===
struct PendingMessage {
  String topic;
  String payload;
  unsigned long timestamp;
};

// === VARIABLES GLOBALES BUFFER ===
std::vector<PendingMessage> messageBuffer;
int bufferedMessagesCount = 0;
int sentFromBufferCount = 0;
unsigned long lastBufferCheck = 0;
const unsigned long BUFFER_CHECK_INTERVAL = 10000;

// === VARIABLES GLOBALES WIFI ===
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000;

// === VARIABLES DEVICE TWIN ===
unsigned long lastTwinUpdate = 0;
const unsigned long TWIN_UPDATE_INTERVAL = 60000;
int twinRequestId = 0;

// === MQTT / Azure ===
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

// === DÉCLARATIONS FORWARD ===
bool connectIoTHub();
void publishStatus();
void sendBufferedMessages();
void publishTwinReported();

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
    Serial.println("[AZURE] Clé Base64 invalide"); 
    return ""; 
  }
  uint8_t mac[32];
  if(!hmacSha256(key,(const uint8_t*)toSign.c_str(),toSign.length(),mac)){ 
    Serial.println("[AZURE] HMAC échec"); 
    return ""; 
  }
  String sig=urlEncode(base64Encode(mac,sizeof(mac)));
  return "SharedAccessSignature sr="+urlEncode(res)+"&sig="+sig+"&se="+String(exp);
}

// ============================================
// FONCTIONS BUFFER
// ============================================

void addToBuffer(const String& topic, const String& payload) {
  if (messageBuffer.size() >= MAX_BUFFER_SIZE) {
    Serial.println("[BUFFER] ⚠️ Buffer plein, suppression du plus ancien");
    messageBuffer.erase(messageBuffer.begin());
  }
  
  PendingMessage msg;
  msg.topic = topic;
  msg.payload = payload;
  msg.timestamp = millis();
  
  messageBuffer.push_back(msg);
  bufferedMessagesCount++;
  
  Serial.printf("[BUFFER] Message ajouté (#%d en attente)\n", messageBuffer.size());
}

void sendBufferedMessages() {
  if (messageBuffer.empty()) {
    return;
  }
  
  if (!mqtt.connected()) {
    Serial.println("[BUFFER] MQTT déconnecté, impossible d'envoyer");
    return;
  }
  
  Serial.printf("[BUFFER] 📤 Envoi de %d messages en attente...\n", messageBuffer.size());
  
  std::vector<PendingMessage> toSend = messageBuffer;
  messageBuffer.clear();
  
  int successCount = 0;
  int failCount = 0;
  
  for (size_t i = 0; i < toSend.size(); i++) {
    bool ok = mqtt.publish(toSend[i].topic.c_str(), toSend[i].payload.c_str());
    
    if (ok) {
      successCount++;
      sentFromBufferCount++;
      Serial.printf("[BUFFER] ✅ %d/%d envoyé\n", i+1, toSend.size());
      delay(100);
    } else {
      failCount++;
      Serial.printf("[BUFFER] ❌ %d/%d échoué\n", i+1, toSend.size());
      addToBuffer(toSend[i].topic, toSend[i].payload);
    }
    
    esp_task_wdt_reset();
  }
  
  Serial.printf("[BUFFER] 📊 Résumé: %d ✅, %d ❌\n", successCount, failCount);
}

// ============================================
// FONCTIONS DEVICE TWIN
// ============================================

void publishTwinReported() {
  String topic = "$iothub/twin/PATCH/properties/reported/?$rid=" + String(twinRequestId++);
  
  StaticJsonDocument<512> doc;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["uptime"] = millis() / 1000;
  doc["detectionEnabled"] = detectionEnabled;
  doc["cooldown"] = cooldownPeriod;
  doc["detectionCount"] = detectionCount;
  
  JsonObject system = doc.createNestedObject("system");
  system["rssi"] = WiFi.RSSI();
  system["freeHeap"] = ESP.getFreeHeap();
  system["cpuFreq"] = ESP.getCpuFreqMHz();
  system["buffered"] = messageBuffer.size();
  
  String payload;
  serializeJson(doc, payload);
  
  if (mqtt.connected()) {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    Serial.printf("[TWIN] Reported %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

void requestTwinGet() {
  String topic = "$iothub/twin/GET/?$rid=" + String(twinRequestId++);
  
  if (mqtt.connected()) {
    bool ok = mqtt.publish(topic.c_str(), "");
    Serial.printf("[TWIN] GET request %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

void handleTwinDesired(const JsonObject& doc) {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  🔄 DEVICE TWIN DESIRED UPDATE       ║");
  Serial.println("╚═══════════════════════════════════════╝");
  
  bool changed = false;
  
  if (doc.containsKey("detectionEnabled")) {
    bool newValue = doc["detectionEnabled"];
    if (newValue != detectionEnabled) {
      detectionEnabled = newValue;
      Serial.printf("[TWIN] detectionEnabled: %s → %s\n", 
                    !newValue ? "true" : "false",
                    newValue ? "true" : "false");
      
      if (detectionEnabled) {
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
    if (newValue >= 1000 && newValue <= 60000 && newValue != cooldownPeriod) {
      Serial.printf("[TWIN] cooldown: %lu ms → %lu ms\n", 
                    cooldownPeriod, newValue);
      cooldownPeriod = newValue;
      changed = true;
    }
  }
  
  if (changed) {
    Serial.println("[TWIN] ✅ Configuration mise à jour depuis Azure");
    publishTwinReported();
  } else {
    Serial.println("[TWIN] ℹ️ Aucun changement nécessaire");
  }
  
  Serial.println("───────────────────────────────────────");
}

// ============================================
// FONCTIONS PUBLICATION
// ============================================

void publishStatus() {
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  String payload = "{";
  payload += "\"event\":\"status\",";
  payload += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
  payload += "\"uptime\":" + String(millis() / 1000) + ",";
  payload += "\"detectionEnabled\":" + String(detectionEnabled ? "true" : "false") + ",";
  payload += "\"cooldown\":" + String(cooldownPeriod) + ",";
  payload += "\"detectionCount\":" + String(detectionCount) + ",";
  payload += "\"system\":{";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  payload += "\"buffered\":" + String(messageBuffer.size());
  payload += "}";
  payload += "}";
  
  if (mqtt.connected()) {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    Serial.printf("[STATUS] Publish %s\n", ok ? "✅ OK" : "❌ FAIL");
  }
}

void publishDetectionJson(){
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  String payload = "{";
  payload += "\"event\":\"motion\",";
  payload += "\"count\":" + String(detectionCount) + ",";
  payload += "\"ts\":" + String(millis()) + ",";
  payload += "\"config\":{";
  payload += "\"detectionEnabled\":" + String(detectionEnabled ? "true" : "false") + ",";
  payload += "\"cooldown\":" + String(cooldownPeriod) + ",";
  payload += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"";
  payload += "},";
  payload += "\"system\":{";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  payload += "\"uptime\":" + String(millis() / 1000) + ",";
  payload += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ",";
  payload += "\"buffered\":" + String(bufferedMessagesCount) + ",";
  payload += "\"sentFromBuffer\":" + String(sentFromBufferCount);
  payload += "}";
  payload += "}";
  
  if(!mqtt.connected()) {
    Serial.println("[MQTT] Déconnecté. Tentative de reconnexion...");
    
    if (connectIoTHub()) {
      Serial.println("[MQTT] ✅ Reconnecté !");
      sendBufferedMessages();
      
      bool ok = mqtt.publish(topic.c_str(), payload.c_str());
      Serial.printf("[MQTT] Publish %s : %s\n", ok ? "✅ OK" : "❌ FAIL", payload.c_str());
      
    } else {
      Serial.println("[MQTT] ❌ Reconnexion échouée");
      addToBuffer(topic, payload);
    }
    
  } else {
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] Publish %s : %s\n", ok ? "✅ OK" : "❌ FAIL", payload.c_str());
    
    if (ok && !messageBuffer.empty()) {
      sendBufferedMessages();
    }
  }
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
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  📨 TWIN DESIRED PATCH REÇU !         ║");
    Serial.println("╚═══════════════════════════════════════╝");
    Serial.printf("[TWIN] Payload: %s\n", message.c_str());
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.printf("[TWIN] ❌ Erreur parsing JSON: %s\n", error.c_str());
      return;
    }
    
    handleTwinDesired(doc.as<JsonObject>());
    return;
  }
  
  // TRAITER TWIN RESPONSE
  if (topicStr.startsWith("$iothub/twin/res/")) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  📋 TWIN GET RESPONSE REÇUE !         ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    int statusStart = topicStr.indexOf("/res/") + 5;
    int statusEnd = topicStr.indexOf("/", statusStart);
    String statusCode = topicStr.substring(statusStart, statusEnd);
    
    Serial.printf("[TWIN] Status Code: %s\n", statusCode.c_str());
    
    if (statusCode == "200") {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (!error) {
        if (doc.containsKey("desired")) {
          JsonObject desired = doc["desired"];
          Serial.println("[TWIN] Propriétés desired trouvées:");
          serializeJsonPretty(desired, Serial);
          Serial.println();
          
          handleTwinDesired(desired);
        }
      }
    }
    
    Serial.println("───────────────────────────────────────");
    return;
  }
  
  // TRAITER C2D MESSAGES
  if (topicStr.startsWith("devices/")) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  📨 MESSAGE C2D REÇU DEPUIS AZURE !   ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    Serial.printf("[C2D] Payload: %s\n", message.c_str());
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.printf("[C2D] ❌ Erreur parsing JSON: %s\n", error.c_str());
      return;
    }
    
    const char* command = doc["command"];
    
    if (command == nullptr) {
      Serial.println("[C2D] ❌ Aucune commande trouvée");
      return;
    }
    
    Serial.printf("[C2D] 🎯 Commande: %s\n", command);
    
    if (strcmp(command, "enable") == 0) {
      detectionEnabled = true;
      Serial.println("[C2D] ✅ Détection ACTIVÉE");
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "disable") == 0) {
      detectionEnabled = false;
      Serial.println("[C2D] ⛔ Détection DÉSACTIVÉE");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "setCooldown") == 0) {
      if (doc.containsKey("value")) {
        unsigned long newCooldown = doc["value"];
        if (newCooldown >= 1000 && newCooldown <= 60000) {
          cooldownPeriod = newCooldown;
          Serial.printf("[C2D] ✅ Cooldown changé: %lu ms\n", cooldownPeriod);
          publishStatus();
          publishTwinReported();
        }
      }
      
    } else if (strcmp(command, "getStatus") == 0) {
      Serial.println("[C2D] 📊 Envoi du statut...");
      publishStatus();
      publishTwinReported();
      
    } else if (strcmp(command, "getTwin") == 0) {
      Serial.println("[C2D] 🔍 Demande du Device Twin...");
      requestTwinGet();
      
    } else if (strcmp(command, "reboot") == 0) {
      Serial.println("[C2D] 🔄 REDÉMARRAGE dans 3 secondes...");
      delay(3000);
      ESP.restart();
      
    } else if (strcmp(command, "clearBuffer") == 0) {
      messageBuffer.clear();
      Serial.println("[C2D] ✅ Buffer vidé");
      publishStatus();
      
    } else {
      Serial.printf("[C2D] ❌ Commande inconnue: %s\n", command);
    }
    
    Serial.println("───────────────────────────────────────");
  }
}

// ============================================
// FONCTIONS WIFI
// ============================================

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  
  Serial.println("[WiFi] ⚠️ WiFi déconnecté, reconnexion...");
  
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    esp_task_wdt_reset();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WiFi] ❌ Impossible de reconnecter le WiFi");
    return false;
  }
  
  Serial.printf("\n[WiFi] ✅ WiFi reconnecté, IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
  
  delay(2000);
  
  return true;
}

void connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connexion à %s ...\n", WIFI_SSID);
  while(WiFi.status()!=WL_CONNECTED){ 
    delay(500); 
    Serial.print("."); 
    esp_task_wdt_reset();
  }
  Serial.printf("\n[WiFi] Connecté, IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
}

// ============================================
// FONCTIONS CONNEXION AZURE
// ============================================

bool connectIoTHub(){
  if (!ensureWiFiConnected()) {
    return false;
  }
  
  Serial.println("[TLS] Configuration du certificat Azure IoT Hub...");
  tlsClient.setCACert(azure_root_ca);
  
  mqtt.setCallback(messageCallback);
  mqtt.setServer(IOTHUB_HOST, 8883);

  Serial.println("[NTP] Tentative de synchronisation de l'heure...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  if (!waitForTime()) {
      Serial.println("[NTP] ❌ Échec de la synchronisation.");
      return false;
  } else {
      time_t now;
      time(&now);
      Serial.printf("[NTP] ✅ Heure synchronisée : %s", ctime(&now));
  }

  String sas = buildSasToken(IOTHUB_HOST, IOTHUB_DEVICE_ID, IOTHUB_DEVICE_KEY_BASE64, 3600);
  String clientId = IOTHUB_DEVICE_ID;
  String username = String(IOTHUB_HOST) + "/" + IOTHUB_DEVICE_ID + "/?api-version=2020-09-30";

  Serial.println("\n--- Informations de Connexion MQTT ---");
  Serial.println("Client ID: " + clientId);
  Serial.println("Username: " + username);
  Serial.println("SAS Token: " + sas);
  Serial.println("-------------------------------------\n");

  Serial.println("[MQTT] Connexion à IoT Hub...");
  if (!mqtt.connect(clientId.c_str(), username.c_str(), sas.c_str())) {
    Serial.printf("[MQTT] ❌ Échec, rc=%d\n", mqtt.state());
    return false;
  }
  
  Serial.println("[MQTT] ✅ Connecté à IoT Hub");
  
  String subscribeC2D = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/devicebound/#";
  if (mqtt.subscribe(subscribeC2D.c_str())) {
    Serial.println("[C2D] ✅ Abonné aux messages Cloud-to-Device");
  }
  
  if (mqtt.subscribe("$iothub/twin/PATCH/properties/desired/#")) {
    Serial.println("[TWIN] ✅ Abonné aux PATCH desired");
  }
  
  if (mqtt.subscribe("$iothub/twin/res/#")) {
    Serial.println("[TWIN] ✅ Abonné aux réponses twin");
  }
  
  delay(1000);
  
  requestTwinGet();
  publishTwinReported();
  publishStatus();
  
  return true;
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
  
  Serial.println("[WDT] Configuration du watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  bootTime = millis();
  
  connectWiFi();
  
  if (!connectIoTHub()) {
    Serial.println("[AZURE] ❌ Échec de la connexion initiale");
    Serial.println("[SYSTEM] Redémarrage dans 10 secondes...");
    delay(10000);
    ESP.restart();
  }
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  Serial.println("\n[SYSTEM] ✅ Démarrage terminé, en attente de détections...\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  esp_task_wdt_reset();
  
  if (!mqtt.connected()) {
    Serial.println("[MQTT] Déconnecté, tentative de reconnexion...");
    if (connectIoTHub()) {
      Serial.println("[MQTT] ✅ Reconnecté");
      sendBufferedMessages();
    } else {
      Serial.println("[MQTT] ❌ Reconnexion échouée, retry dans 5s");
      delay(5000);
    }
  }
  
  mqtt.loop();
  
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = millis();
    ensureWiFiConnected();
  }
  
  if (millis() - lastBufferCheck > BUFFER_CHECK_INTERVAL) {
    lastBufferCheck = millis();
    if (!messageBuffer.empty() && mqtt.connected()) {
      sendBufferedMessages();
    }
  }
  
  if (millis() - lastTwinUpdate > TWIN_UPDATE_INTERVAL) {
    lastTwinUpdate = millis();
    if (mqtt.connected()) {
      publishTwinReported();
    }
  }
  
  if (!detectionEnabled) {
    delay(100);
    return;
  }
  
  bool currentState = digitalRead(PIR_PIN);
  unsigned long now = millis();
  
  if (currentState == HIGH && lastPirState == LOW) {
    if ((now - lastStateChangeTime) > DEBOUNCE_DELAY) {
      if (!motionInProgress) {
        if ((now - lastValidDetectionTime) > cooldownPeriod) {
          motionInProgress = true;
          detectionCount++;
          lastValidDetectionTime = now;
          
          Serial.println("\n╔═══════════════════════════════════════╗");
          Serial.printf("║  🚨 DÉTECTION #%-4d                  ║\n", detectionCount);
          Serial.println("╚═══════════════════════════════════════╝");
          
          digitalWrite(LED_PIN, HIGH);
          
          publishDetectionJson();
          
          Serial.println("───────────────────────────────────────\n");
        } else {
          Serial.printf("[PIR] ⏳ Cooldown actif (%lu ms restant)\n", 
                        cooldownPeriod - (now - lastValidDetectionTime));
        }
      }
    }
    lastStateChangeTime = now;
  }
  
  if (currentState == LOW && lastPirState == HIGH) {
    if ((now - lastStateChangeTime) > DEBOUNCE_DELAY) {
      if (motionInProgress) {
        motionInProgress = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("[PIR] ✅ Mouvement terminé");
      }
    }
    lastStateChangeTime = now;
  }
  
  lastPirState = currentState;
  
  delay(50);
}