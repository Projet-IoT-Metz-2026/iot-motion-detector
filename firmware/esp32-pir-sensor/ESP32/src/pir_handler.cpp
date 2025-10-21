// src/pir_handler.cpp - VERSION OPTIMISÉE (Moins d'écritures flash)
#include "pir_handler.h"
#include "config_store.h"

static int s_pirPin = -1;
static int s_ledPin = -1;

static bool lastPirState = LOW;
static bool inMotion = false;
static unsigned long lastStateChange = 0;
static unsigned long lastValidDetection = 0;
static const unsigned long DEBOUNCE_MS = 500;

static PublishMotionFn s_publishCallback = nullptr;

// ✅ NOUVEAU : Compteur en mémoire + dernier compte sauvé
static int s_detectionCountMemory = 0;
static int s_lastSavedCount = 0;
static const int SAVE_EVERY_N_DETECTIONS = 10;  // Sauvegarder toutes les 10 détections

void pirSetPublishCallback(PublishMotionFn cb) { 
  s_publishCallback = cb; 
}

void pirSetup(int pirPin, int ledPin) {
  s_pirPin = pirPin;
  s_ledPin = ledPin;
  
  if (s_pirPin >= 0) {
    pinMode(s_pirPin, INPUT_PULLDOWN);
    Serial.printf("[PIR] Initialized on GPIO %d\n", s_pirPin);
  }
  if (s_ledPin >= 0) {
    pinMode(s_ledPin, OUTPUT);
    digitalWrite(s_ledPin, LOW);
  }
  
  // ✅ Charger le compteur depuis la flash au démarrage
  s_detectionCountMemory = getDetectionCount();
  s_lastSavedCount = s_detectionCountMemory;
  Serial.printf("[PIR] Detection count loaded: %d\n", s_detectionCountMemory);
}

int pirGetCount() { 
  return s_detectionCountMemory;  // ✅ Retourner le compteur en mémoire
}

void pirResetMotion() { 
  inMotion = false; 
}

void pirLoop() {
  if (!getDetectionEnabled() || s_pirPin < 0) return;

  int current = digitalRead(s_pirPin);
  unsigned long now = millis();

  if (current != lastPirState) {
    // ✅ FIX: Gestion correcte de l'overflow millis()
    if ((now - lastStateChange) > DEBOUNCE_MS) {
      
      // Front montant + pas déjà en mouvement
      if (current == HIGH && !inMotion) {
        // ✅ FIX: Vérification du cooldown avec gestion overflow
        if ((now - lastValidDetection) > getCooldownMs()) {
          
          // ✅ OPTIMISATION : Incrémenter en mémoire
          s_detectionCountMemory++;
          int cnt = s_detectionCountMemory;
          
          lastValidDetection = now;
          inMotion = true;
          
          if (s_ledPin >= 0) {
            digitalWrite(s_ledPin, HIGH);
          }

          // Obtenir le timestamp Unix
          time_t tnow;
          time(&tnow);
          
          // Appeler le callback de publication si défini
          if (s_publishCallback) {
            s_publishCallback(cnt, tnow);
          }

          Serial.println("\n╔═══════════════════════════════════════╗");
          Serial.printf("║  🚨 MOTION #%-4d                      ║\n", cnt);
          Serial.println("╚═══════════════════════════════════════╝");
          
          // ✅ OPTIMISATION : Sauvegarder seulement toutes les N détections
          if (cnt == 1 || 
              cnt % SAVE_EVERY_N_DETECTIONS == 0 || 
              (cnt - s_lastSavedCount) >= SAVE_EVERY_N_DETECTIONS) {
            setDetectionCount(cnt);
            s_lastSavedCount = cnt;
            Serial.printf("[PIR] 💾 Count saved to flash: %d (every %d detections)\n", 
              cnt, SAVE_EVERY_N_DETECTIONS);
          } else {
            Serial.printf("[PIR] Count in memory: %d (will save at %d)\n", 
              cnt, ((cnt / SAVE_EVERY_N_DETECTIONS) + 1) * SAVE_EVERY_N_DETECTIONS);
          }
          
        } else {
          // Dans le cooldown - détection ignorée
          unsigned long timeLeft = getCooldownMs() - (now - lastValidDetection);
          Serial.printf("[PIR] ⏱️ Cooldown active (%lu ms restantes)\n", timeLeft);
        }
      }
      
      // Front descendant
      if (current == LOW && inMotion) {
        Serial.println("[PIR] Motion ended");
        inMotion = false;
        if (s_ledPin >= 0) {
          digitalWrite(s_ledPin, LOW);
        }
      }
      
      lastStateChange = now;
    }
  }
  lastPirState = current;
}