// src/main.cpp
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "led_handler.h"
#include "pir_handler.h"
#include "azure_handler.h"
#include "config_store.h"

static const int PIR_PIN = 13;
static const int LED_PIN = 2;

static const int  WDT_TIMEOUT_SEC = 30;
static const char* FW_VERSION = "2.3.0-Refactored";

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  ESP32 PIR + Azure IoT Hub v2.3       ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.printf("Firmware: %s\n", FW_VERSION);
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  // Watchdog setup
  Serial.println("[WDT] Initializing watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Init modules
  ledSetup(LED_PIN);
  configInit();         // charge la config persistée (detectionEnabled / cooldown)
  pirSetup(PIR_PIN, LED_PIN);

  // Azure (WiFi, MQTT, Twin, OTA, buffer)
  azureSetFirmwareVersion(FW_VERSION);
  azureInit();

  startLedBlink(3, 200);
  
  Serial.println("\n[SYSTEM] 🚀 Ready!\n");
}

void loop() {
  esp_task_wdt_reset(); // Reset watchdog every loop iteration
  
  updateLedBlink(); // LED non bloquante
  pirLoop();        // logiques PIR + publication via callback
  azureLoop();      // state machines WiFi/MQTT + twin + buffer + status
  
  delayMicroseconds(100);
}
