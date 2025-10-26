// include/ota_handler.h
#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>

/**
 * État de la mise à jour OTA
 */
enum OtaState {
  OTA_IDLE,
  OTA_DOWNLOADING,
  OTA_INSTALLING,
  OTA_SUCCESS,
  OTA_FAILED
};

/**
 * Initialise le module OTA
 */
void otaSetup();

/**
 * Démarre une mise à jour OTA depuis une URL
 * @param firmwareUrl URL du firmware (Azure Blob Storage avec SAS token)
 * @return true si le téléchargement a démarré, false sinon
 */
bool otaStartUpdate(const char* firmwareUrl);

/**
 * Retourne l'état actuel de l'OTA
 */
OtaState otaGetState();

/**
 * Retourne le pourcentage de progression (0-100)
 */
int otaGetProgress();

/**
 * Retourne le dernier message d'erreur
 */
String otaGetLastError();

#endif // OTA_HANDLER_H
