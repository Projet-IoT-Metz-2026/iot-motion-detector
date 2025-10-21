// include/config_store.h
#pragma once
#include <Arduino.h>

// Limites de cooldown (ms)
static const unsigned long COOLDOWN_MIN_MS = 1000;
static const unsigned long COOLDOWN_MAX_MS = 60000;

/**
 * Initialise le module de configuration et charge depuis la flash (Preferences)
 */
void configInit();

/**
 * Sauvegarde la configuration courante dans la flash
 */
void saveConfiguration();

/**
 * Retourne l'Ã©tat de la dÃ©tection (activÃ©e/dÃ©sactivÃ©e)
 */
bool getDetectionEnabled();

/**
 * Active/dÃ©sactive la dÃ©tection et persiste automatiquement
 * @param v true pour activer, false pour dÃ©sactiver
 */
void setDetectionEnabled(bool v);

/**
 * Retourne le cooldown actuel en millisecondes
 */
unsigned long getCooldownMs();

/**
 * Modifie le cooldown (entre COOLDOWN_MIN_MS et COOLDOWN_MAX_MS)
 * @param v Nouveau cooldown en ms
 * @return true si la valeur est valide et a Ã©tÃ© appliquÃ©e
 */
bool setCooldownMs(unsigned long v);

/**
 * Retourne le nombre total de dÃ©tections depuis le dÃ©marrage
 */
int getDetectionCount();

/**
 * Modifie le compteur de dÃ©tections et persiste
 */
void setDetectionCount(int v);
