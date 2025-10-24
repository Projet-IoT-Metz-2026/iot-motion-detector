// src/main.cpp
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include "secrets.h"
#include "led_handler.h"
#include "pir_handler.h"
#include "azure_handler.h"
#include "config_store.h"
#include "dps_handler.h"

static const int PIR_PIN = 13;
static const int LED_PIN = 2;

static const int  WDT_TIMEOUT_SEC = 30;
static const char* FW_VERSION = "2.4.0-DPS";

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  ESP32 PIR + Azure IoT Hub (DPS)     ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.printf("Firmware: %s\n", FW_VERSION);
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  // Watchdog setup
  Serial.println("[WDT] Initializing watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Init modules de base
  ledSetup(LED_PIN);
  configInit();
  pirSetup(PIR_PIN, LED_PIN);

  // ============================================
  // WORKFLOW DPS
  // ============================================

  Serial.println("\n[SETUP] ═══════════════════════════════");
  Serial.println("[SETUP] Phase 1 : Connexion WiFi");
  Serial.println("[SETUP] ═══════════════════════════════");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
  
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 30000)) {
    delay(500);
    Serial.print(".");
    esp_task_wdt_reset();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] ✅ Connecté !");
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\n[WiFi] ❌ Échec de connexion");
    Serial.println("[SETUP] 🔄 Redémarrage dans 10 secondes...");
    delay(10000);
    ESP.restart();
  }

  Serial.println("\n[SETUP] ═══════════════════════════════");
  Serial.println("[SETUP] Phase 2 : Initialisation DPS");
  Serial.println("[SETUP] ═══════════════════════════════");
  
  dpsInit();

  Serial.println("\n[SETUP] ═══════════════════════════════");
  Serial.println("[SETUP] Phase 3 : Provisioning");
  Serial.println("[SETUP] ═══════════════════════════════");

  String hubHostname = "";

  if (dpsHasCachedHub()) {
    // Hub déjà en cache (provisioning déjà fait)
    hubHostname = dpsGetCachedHub();
    Serial.printf("[SETUP] 📦 Hub en cache : %s\n", hubHostname.c_str());
    Serial.println("[SETUP] ⚡ Skip provisioning (cache valide)");
  } else {
    // Pas de cache → lancer le provisioning
    Serial.println("[SETUP] 🚀 Provisioning DPS requis...");
    
    DpsAssignment assignment = dpsProvision();
    
    if (assignment.success) {
      hubHostname = String(assignment.iotHubHostname);
      Serial.println("\n[SETUP] ✅ Provisioning réussi !");
      Serial.printf("[SETUP]    Hub: %s\n", hubHostname.c_str());
      Serial.printf("[SETUP]    DeviceId: %s\n", assignment.deviceId);
    } else {
      Serial.println("\n[SETUP] ❌ Échec du provisioning DPS");
      Serial.println("[SETUP] 🔄 Redémarrage dans 10 secondes...");
      
      startLedBlink(10, 100); // Clignotement rapide = erreur
      delay(10000);
      ESP.restart();
    }
  }

  Serial.println("\n[SETUP] ═══════════════════════════════");
  Serial.println("[SETUP] Phase 3 : Configuration Azure");
  Serial.println("[SETUP] ═══════════════════════════════");

  // Configuration Azure avec le Hub assigné
  azureSetIotHub(hubHostname.c_str());
  azureSetFirmwareVersion(FW_VERSION);
  azureInit();

  startLedBlink(3, 200);
  
  Serial.println("\n[SYSTEM] ═══════════════════════════════");
  Serial.println("[SYSTEM] 🚀 Ready!");
  Serial.println("[SYSTEM] ═══════════════════════════════\n");
}

void loop() {
  esp_task_wdt_reset();
  
  updateLedBlink();
  pirLoop();
  azureLoop();
  
  delayMicroseconds(100);
}