// src/ota_handler.cpp
#include "ota_handler.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

// ============================================
// VARIABLES GLOBALES
// ============================================

static OtaState otaState = OTA_IDLE;
static int otaProgress = 0;
static String lastError = "";

// ============================================
// FONCTIONS PRIVÉES
// ============================================

/**
 * Callback de progression du téléchargement
 */
void otaProgressCallback(size_t progress, size_t total) {
  if (total > 0) {
    otaProgress = (progress * 100) / total;
    Serial.printf("[OTA] Progression: %d%% (%d/%d octets)\n", otaProgress, progress, total);
  }
}

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

void otaSetup() {
  Serial.println("[OTA] Module OTA initialisé");

  // Configurer les callbacks de l'Update
  Update.onProgress(otaProgressCallback);
}

bool otaStartUpdate(const char* firmwareUrl) {
  if (otaState != OTA_IDLE) {
    lastError = "Une mise à jour est déjà en cours";
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    return false;
  }

  Serial.printf("[OTA] Démarrage de la mise à jour depuis: %s\n", firmwareUrl);
  otaState = OTA_DOWNLOADING;
  otaProgress = 0;
  lastError = "";

  WiFiClientSecure client;
  client.setInsecure(); // Pour Azure Blob Storage (certificat auto-signé possible)

  HTTPClient http;
  http.begin(client, firmwareUrl);
  http.setTimeout(30000); // 30 secondes timeout

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    lastError = "Échec du téléchargement HTTP: " + String(httpCode);
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    lastError = "Taille du firmware invalide";
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  Serial.printf("[OTA] Taille du firmware: %d octets\n", contentLength);

  // Démarrer la mise à jour
  otaState = OTA_INSTALLING;
  bool canBegin = Update.begin(contentLength);

  if (!canBegin) {
    lastError = "Impossible de démarrer la mise à jour (espace insuffisant?)";
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  // Écrire le firmware
  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written != contentLength) {
    lastError = "Échec de l'écriture du firmware: " + String(written) + "/" + String(contentLength);
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  // Finaliser la mise à jour
  if (!Update.end()) {
    lastError = "Échec de finalisation: " + String(Update.getError());
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  // Vérifier l'intégrité
  if (!Update.isFinished()) {
    lastError = "Mise à jour incomplète";
    Serial.printf("[OTA] ❌ %s\n", lastError.c_str());
    otaState = OTA_FAILED;
    http.end();
    return false;
  }

  Serial.println("[OTA] ✅ Mise à jour réussie ! Redémarrage dans 3 secondes...");
  otaState = OTA_SUCCESS;
  otaProgress = 100;
  http.end();

  delay(3000);
  ESP.restart();

  return true;
}

OtaState otaGetState() {
  return otaState;
}

int otaGetProgress() {
  return otaProgress;
}

String otaGetLastError() {
  return lastError;
}
