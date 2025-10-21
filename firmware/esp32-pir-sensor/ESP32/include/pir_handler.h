// include/pir_handler.h
#pragma once
#include <Arduino.h>

/**
 * Initialise le capteur PIR
 * @param pirPin GPIO du capteur PIR
 * @param ledPin GPIO de la LED de statut
 */
void pirSetup(int pirPin, int ledPin);

/**
 * Boucle principale du gestionnaire PIR (Ã  appeler dans loop())
 * GÃ¨re le debouncing, cooldown, et dÃ©clenche le callback de publication
 */
void pirLoop();

/**
 * Retourne le nombre total de dÃ©tections
 */
int pirGetCount();

/**
 * RÃ©initialise l'Ã©tat de mouvement en cours
 */
void pirResetMotion();

/**
 * Callback de publication appelÃ©e lors d'une dÃ©tection
 * @param count NumÃ©ro de la dÃ©tection
 * @param timestamp Timestamp Unix de la dÃ©tection
 */
typedef void (*PublishMotionFn)(int count, time_t timestamp);

/**
 * Enregistre le callback qui sera appelÃ© lors d'une dÃ©tection
 */
void pirSetPublishCallback(PublishMotionFn cb);
