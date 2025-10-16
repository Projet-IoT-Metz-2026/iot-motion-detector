#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <vector>
#include <ctype.h>

#include "secrets.h"
#include <PubSubClient.h>
#include "mbedtls/base64.h"
#include "mbedtls/md.h"

// Certificat racine Azure IoT Hub (DigiCert Global Root G2)
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
const unsigned long COOLDOWN_PERIOD = 5000;

// === VARIABLES GLOBALES ===
bool lastPirState = LOW;
unsigned long lastStateChangeTime = 0;
unsigned long lastValidDetectionTime = 0;
int detectionCount = 0;
bool motionInProgress = false;

// === MQTT / Azure ===
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

// === FONCTIONS UTILITAIRES AZURE ===
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

void connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connexion à %s ...\n", WIFI_SSID);
  while(WiFi.status()!=WL_CONNECTED){ 
    delay(500); 
    Serial.print("."); 
  }
  Serial.printf("\n[WiFi] Connecté, IP: %s\n", WiFi.localIP().toString().c_str());
}

bool connectIoTHub(){
  // CORRECTION: Utilisation du certificat racine Azure spécifique
  Serial.println("[TLS] Configuration du certificat Azure IoT Hub...");
  tlsClient.setCACert(azure_root_ca);
  
  mqtt.setServer(IOTHUB_HOST, 8883);

  Serial.println("[NTP] Tentative de synchronisation de l'heure...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  if (!waitForTime()) {
      Serial.println("[NTP] ❌ Échec de la synchronisation. Le SAS Token sera probablement invalide.");
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
  return true;
}

void publishDetectionJson(){
  String topic="devices/"+String(IOTHUB_DEVICE_ID)+"/messages/events/";
  String payload="{\"event\":\"motion\",\"count\":"+String(detectionCount)+",\"ts\":"+String(millis())+"}";
  
  if(!mqtt.connected()) {
    Serial.println("[MQTT] Déconnecté. Tentative de reconnexion...");
    if (!connectIoTHub()) {
      Serial.println("[MQTT] Publish ❌ FAIL : reconnexion impossible.");
      return;
    }
  }

  bool ok=mqtt.publish(topic.c_str(),payload.c_str());
  Serial.printf("[MQTT] Publish %s : %s\n",ok?"✅ OK":"❌ FAIL",payload.c_str());
}

// === SETUP ===
void setup(){
  Serial.begin(115200);
  delay(200);

  Serial.println("\n=== ESP32 Motion Detector v1.6 (Azure Root CA) ===");
  pinMode(LED_PIN,OUTPUT);
  pinMode(PIR_PIN,INPUT);

  connectWiFi();
  connectIoTHub();

  Serial.println("\n[INIT] Calibration du capteur PIR (10s)...");
  for(int i=10;i>0;--i){
    Serial.printf("[INIT] %d s...\n",i);
    delay(1000);
  }
  lastPirState=digitalRead(PIR_PIN);
  Serial.println("[READY] Système prêt !");
}

// === LOOP ===
void loop(){
  if (mqtt.connected()) {
    mqtt.loop();
  }

  unsigned long currentTime = millis();
  bool currentPirState = digitalRead(PIR_PIN);

  if(currentPirState==HIGH && lastPirState==LOW && !motionInProgress){
    lastStateChangeTime=currentTime;
    motionInProgress=true;
    Serial.println("[DEBUG] Signal PIR détecté (attente stabilisation...)");
  }

  if(motionInProgress && currentPirState==HIGH && (currentTime-lastStateChangeTime)>=DEBOUNCE_DELAY){
    if(currentTime - lastValidDetectionTime >= COOLDOWN_PERIOD){ 
      unsigned long previous = lastValidDetectionTime;
      lastValidDetectionTime = currentTime;
      detectionCount++;

      digitalWrite(LED_PIN,HIGH);
      Serial.println("╔═════════════════════════════════════════╗");
      Serial.println("║       🚨 MOUVEMENT DÉTECTÉ ! 🚨         ║");
      Serial.println("╚═════════════════════════════════════════╝");
      Serial.printf("[DETECTION #%d]\n",detectionCount);
      Serial.printf("  → Timestamp: %lu ms\n",currentTime);
      if(detectionCount>1 && previous>0)
        Serial.printf("  → Durée depuis dernière: %.2f s\n",(currentTime-previous)/1000.0f);
      Serial.println("─────────────────────────────────────────");

      publishDetectionJson();
      
      digitalWrite(LED_PIN,LOW);
      motionInProgress=false;
    } else {
      float rem=(COOLDOWN_PERIOD-(currentTime-lastValidDetectionTime))/1000.0f;
      Serial.printf("[DEBUG] Détection ignorée (cooldown: %.1f s)\n",rem<0?0:rem);
      motionInProgress=false;
    }
  }

  if(currentPirState==LOW && lastPirState==HIGH && motionInProgress){
    motionInProgress=false;
  }

  lastPirState=currentPirState;
  delay(50);
}
