// ============================================
// CONFIG STORE - EEPROM Persistence
// ============================================

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include "Particle.h"
#include "config.h"

struct DeviceConfig {
    uint16_t magic;
    uint16_t version;
    bool detectionEnabled;
    uint32_t cooldownMs;
    uint32_t detectionCount;
    char iotHubHostname[256];
    char deviceId[128];
};

void configStoreInit();
void configLoad();
void configSave();

// Getters
bool configGetDetectionEnabled();
uint32_t configGetCooldown();
uint32_t configGetDetectionCount();
const char* configGetIotHubHostname();
const char* configGetDeviceId();

// Setters
void configSetDetectionEnabled(bool enabled);
void configSetCooldown(uint32_t cooldownMs);
void configIncrementDetectionCount();
void configSetIotHubHostname(const char* hostname);
void configSetDeviceId(const char* deviceId);

#endif // CONFIG_STORE_H
