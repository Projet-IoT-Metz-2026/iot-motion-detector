// src/config_store.cpp
#include "config_store.h"
#include <Preferences.h>

static Preferences prefs;

static bool          s_detectionEnabled = true;
static unsigned long s_cooldownMs       = 3000;
static int           s_detectionCount   = 0;

void configInit() {
  prefs.begin("pir-config", true); // Read-only mode
  s_detectionEnabled = prefs.getBool("detEnabled", true);
  s_cooldownMs       = prefs.getULong("cooldown", 3000UL);
  s_detectionCount   = prefs.getInt("detCount", 0);
  prefs.end();

  // Validation des valeurs chargées
  if (s_cooldownMs < COOLDOWN_MIN_MS || s_cooldownMs > COOLDOWN_MAX_MS) {
    Serial.printf("[CONFIG] ⚠️ Invalid cooldown %lu, resetting to 3000ms\n", s_cooldownMs);
    s_cooldownMs = 3000;
  }

  Serial.printf("[CONFIG] ✅ Loaded: detection=%s, cooldown=%lu ms, count=%d\n",
    s_detectionEnabled ? "ON" : "OFF", s_cooldownMs, s_detectionCount);
}

void saveConfiguration() {
  prefs.begin("pir-config", false); // Read-write mode
  prefs.putBool("detEnabled", s_detectionEnabled);
  prefs.putULong("cooldown", s_cooldownMs);
  prefs.putInt("detCount", s_detectionCount);
  prefs.end();
  Serial.println("[CONFIG] 💾 Saved to flash");
}

bool getDetectionEnabled() { 
  return s_detectionEnabled; 
}

void setDetectionEnabled(bool v) {
  if (s_detectionEnabled == v) return;
  s_detectionEnabled = v;
  saveConfiguration();
  Serial.printf("[CONFIG] Detection %s\n", v ? "enabled" : "disabled");
}

unsigned long getCooldownMs() { 
  return s_cooldownMs; 
}

bool setCooldownMs(unsigned long v) {
  if (v < COOLDOWN_MIN_MS || v > COOLDOWN_MAX_MS) {
    Serial.printf("[CONFIG] ⚠️ Cooldown %lu out of range [%lu-%lu]\n", 
      v, COOLDOWN_MIN_MS, COOLDOWN_MAX_MS);
    return false;
  }
  if (s_cooldownMs == v) return true;
  s_cooldownMs = v;
  saveConfiguration();
  Serial.printf("[CONFIG] Cooldown set to %lu ms\n", v);
  return true;
}

int getDetectionCount() { 
  return s_detectionCount; 
}

void setDetectionCount(int v) {
  if (v == s_detectionCount) return;
  s_detectionCount = v;
  saveConfiguration();
}
