// src/azure_handler.cpp
#include "azure_handler.h"
#include "config_store.h"
#include "led_handler.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <vector>
#include <esp_task_wdt.h>
#include <esp_system.h>

#include "mbedtls/base64.h"
#include "mbedtls/md.h"

// === AZURE ROOT CA (DigiCert Global Root G2) ===
static const char* AZURE_ROOT_CA =
"-----BEGIN CERTIFICATE-----\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
"MrY=\n"
"-----END CERTIFICATE-----\n";

// === ENUMS ===
enum WiFiState { WIFI_DISCONNECTED, WIFI_CONNECTING, WIFI_CONNECTED, WIFI_RECONNECTING };
enum MqttState { MQTT_STATE_DISCONNECTED, MQTT_STATE_CONNECTING, MQTT_STATE_CONNECTED };
enum BufferState { BUFFER_IDLE, BUFFER_SENDING };

// === BUFFER (utilisant char[] au lieu de String) ===
struct PendingMessage {
  char topic[128];
  char payload[512];
  unsigned long timestamp;
};

static std::vector<PendingMessage> s_buffer;
static const size_t MAX_BUFFER_SIZE = 50;
static BufferState s_bufferState = BUFFER_IDLE;
static size_t s_currentBufferIdx = 0;
static unsigned long s_lastBufferCheck = 0;
static const unsigned long BUFFER_CHECK_INTERVAL = 10000;

// === WIFI/MQTT STATE ===
static WiFiState s_wifi = WIFI_DISCONNECTED;
static MqttState s_mqttState = MQTT_STATE_DISCONNECTED;
static unsigned long s_wifiStart = 0;
static unsigned long s_lastWiFiTick = 0;
static unsigned long s_lastMqttAttempt = 0;
static unsigned long s_mqttBackoff = 5000;
static int s_mqttFails = 0;

static const unsigned long WIFI_CHECK_INTERVAL = 5000;
static const unsigned long WIFI_CONNECT_TIMEOUT = 10000;
static const unsigned long MQTT_RECONNECT_MIN_MS = 5000;
static const unsigned long MQTT_RECONNECT_MAX_MS = 60000;

// === TWIN ===
static unsigned long s_lastTwin = 0;
static const unsigned long TWIN_UPDATE_INTERVAL = 60000;
static int s_twinRid = 0;

// === STATUS ===
static unsigned long s_lastStatus = 0;
static const unsigned long STATUS_INTERVAL = 600000; // 10 min

// === FIRMWARE ===
static const char* s_fw = "0.0.0";

// === METRICS ===
static int s_totalPublished = 0;
static int s_messagesLost = 0;
static int s_mqttReconnects = 0;

// === CLIENTS ===
static WiFiClientSecure s_tls;
static PubSubClient s_mqtt(s_tls);

// === FORWARD DECLARATIONS ===
static bool connectIoTHub();
static void publishTwinReportedInternal();
static void sendBufferedMessages();

// ========== UTILS AZURE ==========

static String urlEncode(const String &s) {
  String o;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); ++i) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      o += (char)c;
    } else {
      o += '%';
      o += hex[(c >> 4) & 0xF];
      o += hex[c & 0xF];
    }
  }
  return o;
}

static bool base64Decode(const String &in, std::vector<uint8_t> &out) {
  size_t n = 0;
  int rc = mbedtls_base64_decode(nullptr, 0, &n, 
    (const unsigned char*)in.c_str(), in.length());
  if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) return false;
  out.resize(n);
  rc = mbedtls_base64_decode(out.data(), out.size(), &n, 
    (const unsigned char*)in.c_str(), in.length());
  if (rc != 0) return false;
  out.resize(n);
  return true;
}

static String base64Encode(const uint8_t* data, size_t len) {
  size_t outLen = 0;
  int rc = mbedtls_base64_encode(nullptr, 0, &outLen, data, len);
  if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) return "";
  std::vector<uint8_t> out(outLen);
  rc = mbedtls_base64_encode(out.data(), out.size(), &outLen, data, len);
  if (rc != 0) return "";
  return String(reinterpret_cast<const char*>(out.data()), outLen);
}

static bool hmacSha256(const std::vector<uint8_t>& key, 
                       const uint8_t* msg, size_t len, uint8_t out[32]) {
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md) return false;
  return mbedtls_md_hmac(md, key.data(), key.size(), msg, len, out) == 0;
}

// ✅ FIX: Ajout de watchdog reset dans waitForTime
static bool waitForTime(uint32_t timeoutMs = 10000) {
  uint32_t start = millis();
  time_t now = 0;
  while ((millis() - start) < timeoutMs) {
    esp_task_wdt_reset(); // ✅ Reset watchdog pendant l'attente
    time(&now);
    if (now > 1700000000) return true;
    delay(200);
  }
  return false;
}

static String buildSasToken(const String& host, const String& dev, 
                           const String& keyB64, uint32_t ttl) {
  String res = host + "/devices/" + dev;
  res.toLowerCase();
  time_t now;
  time(&now);
  uint32_t exp = (uint32_t)now + ttl;
  String toSign = urlEncode(res) + "\n" + String(exp);
  
  std::vector<uint8_t> key;
  if (!base64Decode(keyB64, key)) {
    Serial.println("[AZURE] ❌ Invalid Base64 key");
    return "";
  }
  
  uint8_t mac[32];
  if (!hmacSha256(key, (const uint8_t*)toSign.c_str(), toSign.length(), mac)) {
    Serial.println("[AZURE] ❌ HMAC failed");
    return "";
  }
  
  String sig = urlEncode(base64Encode(mac, sizeof(mac)));
  return "SharedAccessSignature sr=" + urlEncode(res) + 
         "&sig=" + sig + "&se=" + String(exp);
}

// ========== BUFFER ==========

static void addToBuffer(const char* topic, const char* payload) {
  if (s_buffer.size() >= MAX_BUFFER_SIZE) {
    Serial.println("[BUFFER] ⚠️ Buffer full, dropping oldest");
    s_buffer.erase(s_buffer.begin());
    s_messagesLost++;
  }
  
  PendingMessage msg;
  strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
  strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
  msg.topic[sizeof(msg.topic) - 1] = '\0';
  msg.payload[sizeof(msg.payload) - 1] = '\0';
  msg.timestamp = millis();
  
  s_buffer.push_back(msg);
  Serial.printf("[BUFFER] Message queued (%d pending)\n", (int)s_buffer.size());
}

static void sendBufferedMessages() {
  if (s_buffer.empty()) {
    s_bufferState = BUFFER_IDLE;
    s_currentBufferIdx = 0;
    return;
  }
  
  if (!s_mqtt.connected()) {
    Serial.println("[BUFFER] MQTT disconnected");
    s_bufferState = BUFFER_IDLE;
    s_lastBufferCheck = millis(); // ✅ Force attente avant retry
    return;
  }
  
  if (s_bufferState == BUFFER_IDLE) {
    s_currentBufferIdx = 0;
    s_bufferState = BUFFER_SENDING;
  }
  
  if (s_currentBufferIdx < s_buffer.size()) {
    PendingMessage& msg = s_buffer[s_currentBufferIdx];
    if (s_mqtt.publish(msg.topic, msg.payload)) {
      Serial.printf("[BUFFER] ✅ Sent buffered message %d/%d\n", 
        (int)s_currentBufferIdx + 1, (int)s_buffer.size());
      s_currentBufferIdx++;
      s_totalPublished++;
    } else {
      Serial.println("[BUFFER] ❌ Publish failed, pausing");
      s_bufferState = BUFFER_IDLE;
    }
  } else {
    Serial.printf("[BUFFER] ✅ All %d messages sent!\n", (int)s_buffer.size());
    s_buffer.clear();
    s_bufferState = BUFFER_IDLE;
    s_currentBufferIdx = 0;
  }
}

// ========== PUBLISHING ==========

void azurePublishMotion(int count, time_t timestamp) {
  StaticJsonDocument<256> doc;
  doc["deviceId"] = IOTHUB_DEVICE_ID;
  doc["messageType"] = "motion";
  doc["detectionCount"] = count;
  doc["timestamp"] = timestamp;
  doc["cooldownMs"] = getCooldownMs();
  doc["enabled"] = getDetectionEnabled();
  
  char payload[256];
  serializeJson(doc, payload);
  
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  if (s_mqttState == MQTT_STATE_CONNECTED && s_mqtt.connected()) {
    if (s_mqtt.publish(topic.c_str(), payload)) {
      Serial.println("[AZURE] ✅ Motion published");
      s_totalPublished++;
    } else {
      Serial.println("[AZURE] ❌ Publish failed, buffering");
      addToBuffer(topic.c_str(), payload);
    }
  } else {
    Serial.println("[AZURE] Offline, buffering motion");
    addToBuffer(topic.c_str(), payload);
  }
}

void publishStatus() {
  StaticJsonDocument<512> doc;
  doc["deviceId"] = IOTHUB_DEVICE_ID;
  doc["messageType"] = "status";
  doc["firmware"] = s_fw;
  doc["uptime"] = millis() / 1000;
  doc["detectionCount"] = getDetectionCount();
  doc["cooldownMs"] = getCooldownMs();
  doc["enabled"] = getDetectionEnabled();
  doc["wifiRssi"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["minFreeHeap"] = ESP.getMinFreeHeap();
  doc["bufferedMessages"] = s_buffer.size();
  doc["totalPublished"] = s_totalPublished;
  doc["messagesLost"] = s_messagesLost;
  doc["mqttReconnects"] = s_mqttReconnects;
  
  char payload[512];
  serializeJson(doc, payload);
  
  String topic = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/events/";
  
  if (s_mqtt.connected()) {
    s_mqtt.publish(topic.c_str(), payload);
    Serial.println("[AZURE] 📊 Status published");
  }
  
  // Log heap status
  int freeHeap = ESP.getFreeHeap();
  if (freeHeap < 20000) {
    Serial.printf("[HEAP] ⚠️ LOW MEMORY: %d bytes free\n", freeHeap);
  }
}

static void publishTwinReportedInternal() {
  StaticJsonDocument<256> doc;
  doc["firmware"] = s_fw;
  doc["detectionEnabled"] = getDetectionEnabled();
  doc["cooldownMs"] = getCooldownMs();
  doc["detectionCount"] = getDetectionCount();
  doc["uptime"] = millis() / 1000;
  
  char payload[256];
  serializeJson(doc, payload);
  
  String topic = "$iothub/twin/PATCH/properties/reported/?$rid=" + String(s_twinRid++);
  
  if (s_mqtt.connected()) {
    s_mqtt.publish(topic.c_str(), payload);
    Serial.println("[TWIN] 📡 Reported state updated");
  }
}

// ========== TWIN / C2D HANDLING ==========

static void handleTwinDesired(JsonObject desired) {
  bool changed = false;
  
  if (desired.containsKey("detectionEnabled")) {
    bool newVal = desired["detectionEnabled"];
    if (newVal != getDetectionEnabled()) {
      setDetectionEnabled(newVal);
      changed = true;
    }
  }
  
  if (desired.containsKey("cooldownMs")) {
    unsigned long newCd = desired["cooldownMs"];
    if (setCooldownMs(newCd)) {
      changed = true;
    }
  }
  
  if (changed) {
    Serial.println("[TWIN] ✅ Configuration updated from cloud");
    publishTwinReportedInternal();
  }
}

static void requestTwinGet() {
  String topic = "$iothub/twin/GET/?$rid=" + String(s_twinRid++);
  s_mqtt.publish(topic.c_str(), "");
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t(topic);
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  // Twin desired properties
  if (t.startsWith("$iothub/twin/PATCH/properties/desired/")) {
    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, msg)) {
      handleTwinDesired(doc.as<JsonObject>());
    }
    return;
  }
  
  // Twin GET response
  if (t.startsWith("$iothub/twin/res/")) {
    int sStart = t.indexOf("/res/") + 5;
    int sEnd = t.indexOf("/", sStart);
    String code = t.substring(sStart, sEnd);
    if (code == "200") {
      StaticJsonDocument<1024> doc;
      if (!deserializeJson(doc, msg) && doc.containsKey("desired")) {
        handleTwinDesired(doc["desired"].as<JsonObject>());
      }
    }
    return;
  }
  
  // C2D messages
  if (t.startsWith("devices/")) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, msg)) return;
    const char* cmd = doc["command"];
    if (!cmd) return;
    
    if (strcmp(cmd, "enable") == 0) {
      setDetectionEnabled(true);
      startLedBlink(1, 200);
      publishStatus();
      publishTwinReportedInternal();
    } else if (strcmp(cmd, "disable") == 0) {
      setDetectionEnabled(false);
      startLedBlink(3, 100);
      publishStatus();
      publishTwinReportedInternal();
    } else if (strcmp(cmd, "setCooldown") == 0 && doc.containsKey("value")) {
      unsigned long v = doc["value"];
      if (setCooldownMs(v)) {
        publishStatus();
        publishTwinReportedInternal();
      }
    } else if (strcmp(cmd, "getStatus") == 0) {
      publishStatus();
      publishTwinReportedInternal();
    } else if (strcmp(cmd, "getTwin") == 0) {
      requestTwinGet();
    } else if (strcmp(cmd, "reboot") == 0) {
      Serial.println("[C2D] 🔄 Rebooting...");
      delay(1500);
      ESP.restart();
    } else if (strcmp(cmd, "clearBuffer") == 0) {
      s_buffer.clear();
      Serial.println("[C2D] 🗑️ Buffer cleared");
      publishStatus();
    }
  }
}

// ========== WIFI / MQTT CONNECTION ==========

static bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    esp_task_wdt_reset(); // ✅ Reset watchdog
    tries++;
  }
  
  return WiFi.status() == WL_CONNECTED;
}

static bool connectIoTHub() {
  if (!ensureWiFiConnected()) return false;
  
  String broker = String(IOTHUB_HOST);
  String clientId = String(IOTHUB_DEVICE_ID);
  String username = broker + "/" + clientId + "/?api-version=2021-04-12";
  String sas = buildSasToken(broker, clientId, String(IOTHUB_DEVICE_KEY_BASE64), 86400);
  
  if (sas.isEmpty()) return false;
  
  s_tls.setCACert(AZURE_ROOT_CA);
  s_mqtt.setServer(broker.c_str(), 8883);
  s_mqtt.setCallback(mqttCallback);
  s_mqtt.setBufferSize(2048);
  s_mqtt.setKeepAlive(45);
  s_mqtt.setSocketTimeout(10);
  
  bool ok = s_mqtt.connect(clientId.c_str(), username.c_str(), sas.c_str());
  
  if (ok) {
    String c2d = "devices/" + String(IOTHUB_DEVICE_ID) + "/messages/devicebound/#";
    s_mqtt.subscribe(c2d.c_str());
    s_mqtt.subscribe("$iothub/twin/PATCH/properties/desired/#");
    s_mqtt.subscribe("$iothub/twin/res/#");
    delay(300);
    requestTwinGet();
    s_mqttReconnects++;
  }
  
  return ok;
}

// ========== STATE MACHINES ==========

static void wifiStateMachine() {
  unsigned long now = millis();
  
  switch (s_wifi) {
    case WIFI_DISCONNECTED:
      if ((now - s_lastWiFiTick) >= WIFI_CHECK_INTERVAL) {
        s_lastWiFiTick = now;
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("[WiFi] 🔄 Connecting...");
          WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
          s_wifi = WIFI_CONNECTING;
          s_wifiStart = now;
        } else {
          s_wifi = WIFI_CONNECTED;
        }
      }
      break;
      
    case WIFI_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] ✅ Connected! IP: %s, RSSI: %d dBm\n", 
          WiFi.localIP().toString().c_str(), WiFi.RSSI());
        s_wifi = WIFI_CONNECTED;
        s_mqttState = MQTT_STATE_DISCONNECTED;
      } else if ((now - s_wifiStart) >= WIFI_CONNECT_TIMEOUT) {
        Serial.println("[WiFi] ❌ Connection timeout");
        s_wifi = WIFI_DISCONNECTED;
      }
      break;
      
    case WIFI_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] ⚠️ Connection lost!");
        s_wifi = WIFI_RECONNECTING;
        s_mqttState = MQTT_STATE_DISCONNECTED;
        s_lastWiFiTick = now;
      }
      break;
      
    case WIFI_RECONNECTING:
      if ((now - s_lastWiFiTick) >= WIFI_CHECK_INTERVAL) {
        s_lastWiFiTick = now;
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("[WiFi] ✅ Reconnected!");
          s_wifi = WIFI_CONNECTED;
        } else {
          WiFi.reconnect();
        }
      }
      break;
  }
}

static void mqttStateMachine() {
  if (s_wifi != WIFI_CONNECTED) {
    s_mqttState = MQTT_STATE_DISCONNECTED;
    return;
  }
  
  unsigned long now = millis();
  
  switch (s_mqttState) {
    case MQTT_STATE_DISCONNECTED:
      if ((now - s_lastMqttAttempt) >= s_mqttBackoff) {
        s_lastMqttAttempt = now;
        Serial.println("[MQTT] 🔄 Connecting to Azure IoT Hub...");
        s_mqttState = MQTT_STATE_CONNECTING;
      }
      break;
      
    case MQTT_STATE_CONNECTING:
      if (connectIoTHub()) {
        Serial.println("[MQTT] ✅ Connected to Azure IoT Hub!");
        s_mqttState = MQTT_STATE_CONNECTED;
        s_mqttFails = 0;
        s_mqttBackoff = MQTT_RECONNECT_MIN_MS;
        startLedBlink(2, 120);
        publishStatus();
        publishTwinReportedInternal();
      } else {
        Serial.printf("[MQTT] ❌ Connection failed (state: %d)\n", s_mqtt.state());
        s_mqttState = MQTT_STATE_DISCONNECTED;
        s_mqttFails++;
        if (s_mqttFails >= 5) {
          s_mqttBackoff = min(s_mqttBackoff * 2, MQTT_RECONNECT_MAX_MS);
          Serial.printf("[MQTT] ⏱️ Backoff: next attempt in %lu s\n", 
            s_mqttBackoff / 1000);
        }
      }
      break;
      
    case MQTT_STATE_CONNECTED:
      if (!s_mqtt.connected()) {
        Serial.println("[MQTT] ⚠️ Connection lost!");
        s_mqttState = MQTT_STATE_DISCONNECTED;
      } else {
        s_mqtt.loop();
      }
      break;
  }
}

// ========== PUBLIC API ==========

void azureSetFirmwareVersion(const char* version) {
  s_fw = version ? version : "0.0.0";
}

void azureInit() {
  Serial.println("\n[AZURE] Initializing...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  s_wifi = WIFI_CONNECTING;
  s_wifiStart = millis();
  
  // Register PIR publish callback
  extern void pirSetPublishCallback(void (*)(int, time_t));
  pirSetPublishCallback(azurePublishMotion);
  
  // NTP sync
  Serial.println("[NTP] Synchronizing time...");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  if (waitForTime(15000)) {
    time_t now;
    time(&now);
    Serial.printf("[NTP] ✅ Time synced (timestamp: %ld)\n", now);
  } else {
    Serial.println("[NTP] ⚠️ Time sync failed");
  }
}

void azureLoop() {
  wifiStateMachine();
  mqttStateMachine();
  
  unsigned long now = millis();
  
  // Buffer handling
  if (s_mqttState == MQTT_STATE_CONNECTED) {
    if (s_bufferState == BUFFER_SENDING) {
      sendBufferedMessages();
    } else if (!s_buffer.empty() && (now - s_lastBufferCheck) >= BUFFER_CHECK_INTERVAL) {
      s_lastBufferCheck = now;
      s_bufferState = BUFFER_IDLE;
      sendBufferedMessages();
    }
  }
  
  // Twin update
  if ((now - s_lastTwin) >= TWIN_UPDATE_INTERVAL && 
      s_mqttState == MQTT_STATE_CONNECTED) {
    s_lastTwin = now;
    publishTwinReportedInternal();
  }
  
  // Status update
  if ((now - s_lastStatus) >= STATUS_INTERVAL && 
      s_mqttState == MQTT_STATE_CONNECTED) {
    s_lastStatus = now;
    publishStatus();
  }
}
