// ============================================
// CONFIGURATION - Photon2 PIR Sensor v3.0.0
// ============================================

#ifndef CONFIG_H
#define CONFIG_H

#include "Particle.h"

// ============================================
// HARDWARE CONFIGURATION
// ============================================

#define PIR_PIN D2
#define LED_PIN D7

// ============================================
// PIR SENSOR SETTINGS
// ============================================

#define PIR_DEBOUNCE_MS 500
#define PIR_DEFAULT_COOLDOWN_MS 3000
#define PIR_MIN_COOLDOWN_MS 1000
#define PIR_MAX_COOLDOWN_MS 300000  // 5 minutes max

// ============================================
// AZURE MQTT SETTINGS
// ============================================

#define MQTT_PORT 8883
#define MQTT_KEEPALIVE 60
#define MQTT_CLEAN_SESSION true
#define MQTT_QOS 1

// MQTT Retry with Exponential Backoff
#define MQTT_RECONNECT_BASE_DELAY 1000   // 1 second
#define MQTT_RECONNECT_MAX_DELAY 30000   // 30 seconds max

// ============================================
// MESSAGE QUEUE SETTINGS
// ============================================

#define MESSAGE_QUEUE_MAX_SIZE 50
#define MESSAGE_MAX_LENGTH 512
#define QUEUE_FLUSH_INTERVAL_MS 5000

// ============================================
// EEPROM CONFIGURATION
// ============================================

#define EEPROM_MAGIC 0x42A5
#define EEPROM_VERSION 3
#define EEPROM_ADDR 0

// ============================================
// TELEMETRY SETTINGS
// ============================================

#define TELEMETRY_BUFFER_SIZE 512

// ============================================
// WATCHDOG TIMEOUT
// ============================================

#define WATCHDOG_TIMEOUT_MS 30000

#endif // CONFIG_H
