// include/config_store.h
#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

// ============================================
// Configuration Store (LittleFS persistence)
// ============================================

/**
 * Initialise le module de configuration
 * Charge les valeurs depuis LittleFS
 */
void configInit();

/**
 * Active/désactive la détection PIR
 * @param enabled true pour activer, false pour désactiver
 */
void configSetDetectionEnabled(bool enabled);

/**
 * Définit le cooldown entre détections (millisecondes)
 * @param cooldownMs Délai minimum entre 2 détections
 */
void configSetCooldown(int cooldownMs);

/**
 * Récupère l'état de la détection
 * @return true si activé, false sinon
 */
bool configGetDetectionEnabled();

/**
 * Récupère le cooldown actuel (millisecondes)
 * @return Délai en ms
 */
int configGetCooldown();

#endif