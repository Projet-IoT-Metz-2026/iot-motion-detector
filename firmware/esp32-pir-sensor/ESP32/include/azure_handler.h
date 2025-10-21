// include/azure_handler.h
#pragma once
#include <Arduino.h>

/**
 * DÃ©finit la version du firmware pour le reporting
 */
void azureSetFirmwareVersion(const char* version);

/**
 * Initialise la connexion Azure IoT Hub (WiFi, MQTT, NTP)
 */
void azureInit();

/**
 * Boucle principale Azure (state machines WiFi/MQTT, buffer, twin, status)
 * Ã€ appeler dans loop()
 */
void azureLoop();

/**
 * Publie une dÃ©tection de mouvement vers Azure IoT Hub
 * @param count NumÃ©ro de la dÃ©tection
 * @param timestamp Timestamp Unix
 */
void azurePublishMotion(int count, time_t timestamp);

/**
 * Publie le statut systÃ¨me vers Azure IoT Hub
 */
void publishStatus();
