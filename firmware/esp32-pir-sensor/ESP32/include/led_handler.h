// include/led_handler.h
#pragma once
#include <Arduino.h>

/**
 * Initialise le module LED
 * @param ledPin NumÃ©ro du pin GPIO pour la LED
 */
void ledSetup(int ledPin);

/**
 * DÃ©marre un clignotement non-bloquant de la LED
 * @param count Nombre de clignotements Ã  effectuer
 * @param intervalMs DurÃ©e d'un cycle ON/OFF en millisecondes
 */
void startLedBlink(int count, unsigned long intervalMs);

/**
 * Met Ã  jour l'Ã©tat de la LED (Ã  appeler dans loop())
 * Cette fonction est non-bloquante
 */
void updateLedBlink();
