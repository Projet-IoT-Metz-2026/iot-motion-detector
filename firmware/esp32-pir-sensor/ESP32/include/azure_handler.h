// include/azure_handler.h
#ifndef AZURE_HANDLER_H
#define AZURE_HANDLER_H

#include <Arduino.h>

// ============================================
// Azure IoT Hub Handler (avec support DPS)
// ============================================

/**
 * Définit le hostname du IoT Hub (utilisé après provisioning DPS)
 * DOIT être appelé AVANT azureInit()
 * @param hostname Hostname du Hub (ex: "iot-xxx.azure-devices.net")
 */
void azureSetIotHub(const char* hostname);

/**
 * Définit la version du firmware (pour le Device Twin)
 * @param version Version du firmware
 */
void azureSetFirmwareVersion(const char* version);

/**
 * Initialise le module Azure (WiFi, MQTT, Device Twin)
 * Appeler APRÈS azureSetIotHub()
 */
void azureInit();

/**
 * Boucle principale Azure (gère WiFi, MQTT, buffer, status)
 * À appeler dans loop()
 */
void azureLoop();

/**
 * Publie un message de télémétrie
 * @param jsonPayload Payload JSON à envoyer
 * @return true si publié, false sinon
 */
bool azurePublishTelemetry(const char* jsonPayload);

/**
 * Enregistre un callback pour les détections PIR
 * @param callback Fonction appelée lors d'une détection
 */
void azureSetPirCallback(void (*callback)());

#endif