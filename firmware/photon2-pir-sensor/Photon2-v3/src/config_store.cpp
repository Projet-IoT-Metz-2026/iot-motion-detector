// ============================================
// CONFIG STORE - EEPROM Persistence
// ============================================

#include "config_store.h"

static DeviceConfig cfg;

void configStoreInit() {
    EEPROM.get(EEPROM_ADDR, cfg);

    if (cfg.magic != EEPROM_MAGIC || cfg.version != EEPROM_VERSION) {
        Log.info("Config: Initializing with defaults");
        cfg.magic = EEPROM_MAGIC;
        cfg.version = EEPROM_VERSION;
        cfg.detectionEnabled = true;
        cfg.cooldownMs = PIR_DEFAULT_COOLDOWN_MS;
        cfg.detectionCount = 0;
        memset(cfg.iotHubHostname, 0, sizeof(cfg.iotHubHostname));
        memset(cfg.deviceId, 0, sizeof(cfg.deviceId));
        configSave();
    } else {
        Log.info("Config: Loaded from EEPROM");
    }
}

void configLoad() {
    EEPROM.get(EEPROM_ADDR, cfg);
}

void configSave() {
    EEPROM.put(EEPROM_ADDR, cfg);
}

bool configGetDetectionEnabled() {
    return cfg.detectionEnabled;
}

uint32_t configGetCooldown() {
    return cfg.cooldownMs;
}

uint32_t configGetDetectionCount() {
    return cfg.detectionCount;
}

const char* configGetIotHubHostname() {
    return cfg.iotHubHostname;
}

const char* configGetDeviceId() {
    return cfg.deviceId;
}

void configSetDetectionEnabled(bool enabled) {
    cfg.detectionEnabled = enabled;
    configSave();
}

void configSetCooldown(uint32_t cooldownMs) {
    if (cooldownMs >= PIR_MIN_COOLDOWN_MS && cooldownMs <= PIR_MAX_COOLDOWN_MS) {
        cfg.cooldownMs = cooldownMs;
        configSave();
    }
}

void configIncrementDetectionCount() {
    cfg.detectionCount++;
    configSave();
}

void configSetIotHubHostname(const char* hostname) {
    if (hostname) {
        strncpy(cfg.iotHubHostname, hostname, sizeof(cfg.iotHubHostname) - 1);
        cfg.iotHubHostname[sizeof(cfg.iotHubHostname) - 1] = '\0';
        configSave();
    }
}

void configSetDeviceId(const char* deviceId) {
    if (deviceId) {
        strncpy(cfg.deviceId, deviceId, sizeof(cfg.deviceId) - 1);
        cfg.deviceId[sizeof(cfg.deviceId) - 1] = '\0';
        configSave();
    }
}
