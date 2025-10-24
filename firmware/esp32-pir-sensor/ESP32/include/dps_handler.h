// include/dps_handler.h
#ifndef DPS_HANDLER_H
#define DPS_HANDLER_H

#include <Arduino.h>

// ============================================
// DPS (Device Provisioning Service) Handler
// ============================================
// Gère le workflow de provisioning :
// 1. Connexion à global.azure-devices-provisioning.net
// 2. Envoi de la requête de provisioning
// 3. Récupération de l'IoT Hub assigné
// 4. Stockage du Hub pour connexion ultérieure

// ============================================
// STRUCTURES
// ============================================

struct DpsAssignment {
  char iotHubHostname[256];  // Ex: "iot-iotdetector-dev.azure-devices.net"
  char deviceId[64];         // Ex: "esp32-pir-01"
  bool success;
};

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

/**
 * Initialise le module DPS
 * Charge les credentials depuis secrets.h
 */
void dpsInit();

/**
 * Lance le workflow de provisioning
 * @return DpsAssignment avec le Hub assigné
 */
DpsAssignment dpsProvision();

/**
 * Vérifie si un Hub a déjà été assigné (persisté)
 * @return true si un Hub est en cache
 */
bool dpsHasCachedHub();

/**
 * Récupère le Hub en cache (si disponible)
 * @return Hostname du Hub ou chaîne vide
 */
String dpsGetCachedHub();

/**
 * Sauvegarde le Hub assigné en mémoire persistante
 * @param hostname Hostname du Hub
 */
void dpsSaveHub(const char* hostname);

/**
 * Efface le cache du Hub (force re-provisioning)
 */
void dpsClearCache();

#endif