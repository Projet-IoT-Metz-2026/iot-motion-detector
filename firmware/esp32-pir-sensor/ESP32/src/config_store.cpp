// src/config_store.cpp
#include "config_store.h"
#include <LittleFS.h>

// ============================================
// VARIABLES GLOBALES
// ============================================

static bool detectionEnabled = true;
static int cooldownMs = 5000;

// ============================================
// FONCTIONS PRIVÉES
// ============================================

/**
 * Sauvegarde la configuration dans LittleFS
 */
static void saveConfig() {
  if (!LittleFS.begin(true)) {
    Serial.println("[Config] ⚠️ Impossible d'initialiser LittleFS");
    return;
  }
  
  File f = LittleFS.open("/config.txt", "w");
  if (!f) {
    Serial.println("[Config] ⚠️ Impossible d'ouvrir le fichier de config");
    return;
  }
  
  f.printf("detectionEnabled=%d\n", detectionEnabled ? 1 : 0);
  f.printf("cooldownMs=%d\n", cooldownMs);
  f.close();
  
  Serial.println("[Config] 💾 Configuration sauvegardée");
}

/**
 * Charge la configuration depuis LittleFS
 */
static void loadConfig() {
  if (!LittleFS.begin(true)) {
    Serial.println("[Config] ⚠️ LittleFS non disponible, config par défaut");
    return;
  }
  
  if (!LittleFS.exists("/config.txt")) {
    Serial.println("[Config] ℹ️  Pas de config sauvegardée, valeurs par défaut");
    return;
  }
  
  File f = LittleFS.open("/config.txt", "r");
  if (!f) {
    Serial.println("[Config] ⚠️ Impossible de lire le fichier de config");
    return;
  }
  
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    
    if (line.startsWith("detectionEnabled=")) {
      int value = line.substring(17).toInt();
      detectionEnabled = (value == 1);
      Serial.printf("[Config] Chargé: detectionEnabled=%s\n", detectionEnabled ? "true" : "false");
    }
    else if (line.startsWith("cooldownMs=")) {
      cooldownMs = line.substring(11).toInt();
      Serial.printf("[Config] Chargé: cooldownMs=%d\n", cooldownMs);
    }
  }
  
  f.close();
  Serial.println("[Config] ✅ Configuration chargée");
}

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void configInit() {
  Serial.println("[Config] Initialisation...");
  loadConfig();
}

void configSetDetectionEnabled(bool enabled) {
  detectionEnabled = enabled;
  saveConfig();
  Serial.printf("[Config] detectionEnabled → %s\n", enabled ? "true" : "false");
}

void configSetCooldown(int cooldown) {
  cooldownMs = cooldown;
  saveConfig();
  Serial.printf("[Config] cooldownMs → %d\n", cooldownMs);
}

bool configGetDetectionEnabled() {
  return detectionEnabled;
}

int configGetCooldown() {
  return cooldownMs;
}