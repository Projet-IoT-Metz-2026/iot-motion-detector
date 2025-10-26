// src/pir_handler.cpp
#include "pir_handler.h"
#include "config_store.h"
#include "azure_handler.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

// ============================================
// HELPER : Gestion du débordement de millis()
// ============================================

/**
 * Vérifie si un intervalle de temps s'est écoulé depuis un timestamp.
 * Cette fonction gère correctement le débordement de millis() (après ~49 jours)
 * grâce à l'arithmétique modulaire des unsigned long.
 */
inline bool hasElapsed(unsigned long lastTime, unsigned long interval) {
  return (millis() - lastTime) >= interval;
}

// ============================================
// VARIABLES GLOBALES
// ============================================

static int s_pirPin = -1;
static int s_ledPin = -1;
static int lastPirState = LOW;
static unsigned long lastStateChange = 0;
static unsigned long lastValidDetection = 0;
static bool motionInProgress = false;
static int detectionCount = 0;

static const unsigned long DEBOUNCE_DELAY = 500;
static PublishMotionFn publishCallback = nullptr;

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void pirSetup(int pirPin, int ledPin) {
  s_pirPin = pirPin;
  s_ledPin = ledPin;
  
  pinMode(s_pirPin, INPUT);
  pinMode(s_ledPin, OUTPUT);
  digitalWrite(s_ledPin, LOW);
  
  Serial.printf("[PIR] Initialisé (pin %d)\n", s_pirPin);
}

int pirGetCount() {
  return detectionCount;
}

void pirLoop() {
  if (s_pirPin < 0) return;
  
  // Vérifier si la détection est activée
  if (!configGetDetectionEnabled()) return;
  
  int currentState = digitalRead(s_pirPin);
  unsigned long now = millis();
  
  // Détection d'un changement d'état (gère le débordement de millis)
  if (currentState != lastPirState) {
    if (hasElapsed(lastStateChange, DEBOUNCE_DELAY)) {

      // Mouvement détecté (LOW → HIGH)
      if (currentState == HIGH && !motionInProgress) {

        // Vérifier le cooldown (gère le débordement de millis)
        if (hasElapsed(lastValidDetection, configGetCooldown())) {
          
          detectionCount++;
          lastValidDetection = now;
          motionInProgress = true;
          
          digitalWrite(s_ledPin, HIGH);
          
          Serial.println("╔═══════════════════════════════════════╗");
          Serial.println("║      🚨 MOUVEMENT DÉTECTÉ ! 🚨       ║");
          Serial.println("╚═══════════════════════════════════════╝");
          Serial.printf("[PIR] Détection #%d\n", detectionCount);

          // Collecter les métriques enrichies
          float temperature = temperatureRead(); // Température interne ESP32
          int wifiChannel = WiFi.channel();
          int rssi = WiFi.RSSI();
          uint32_t freeHeap = ESP.getFreeHeap();

          // Publier vers Azure avec métriques enrichies
          char payload[512];
          snprintf(payload, sizeof(payload),
                   "{\"event\":\"motion_detected\",\"count\":%d,\"timestamp\":%lu,"
                   "\"temperature\":%.2f,\"wifiChannel\":%d,\"rssi\":%d,\"freeHeap\":%u}",
                   detectionCount, now, temperature, wifiChannel, rssi, freeHeap);

          azurePublishTelemetry(payload);
        } else {
          Serial.println("[PIR] ⏳ Cooldown actif, détection ignorée");
        }
      }
      
      // Fin du mouvement (HIGH → LOW)
      if (currentState == LOW && motionInProgress) {
        Serial.println("[PIR] Fin du mouvement");
        motionInProgress = false;
        digitalWrite(s_ledPin, LOW);
      }
      
      lastStateChange = now;
    }
  }
  
  lastPirState = currentState;
}

void pirResetMotion() {
  motionInProgress = false;
  digitalWrite(s_ledPin, LOW);
  Serial.println("[PIR] État de mouvement réinitialisé");
}

void pirSetPublishCallback(PublishMotionFn cb) {
  publishCallback = cb;
  Serial.println("[PIR] Callback de publication enregistré");
}