// src/ESP32PreferenceStorage.cpp
#include "ESP32PreferenceStorage.h"
#include <nvs_flash.h>

bool ESP32PreferenceStorage::begin(const char* name, bool readOnly) {
    Logger::info("Initializing preferences storage");
    
    // Initialize NVS flash storage
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Logger::info("NVS needs to be erased first");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Now initialize preferences
    if (!prefs.begin(name, readOnly)) {
        Logger::error("Failed to begin preferences");
        return false;
    }
    
    Logger::info("Preferences storage initialized successfully");
    return true;
}

bool ESP32PreferenceStorage::putString(const char* key, const char* value) {
    return prefs.putString(key, value);
}

String ESP32PreferenceStorage::getString(const char* key, const char* defaultValue) {
    return prefs.getString(key, defaultValue);
}

bool ESP32PreferenceStorage::putUInt(const char* key, uint32_t value) {
    return prefs.putUInt(key, value);
}

uint32_t ESP32PreferenceStorage::getUInt(const char* key, uint32_t defaultValue) {
    return prefs.getUInt(key, defaultValue);
}

bool ESP32PreferenceStorage::remove(const char* key) {
    return prefs.remove(key);
}