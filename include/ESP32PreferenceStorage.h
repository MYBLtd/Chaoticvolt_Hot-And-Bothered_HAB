// include/ESP32PreferenceStorage.h
#pragma once

#include "PreferenceStorage.h"
#include <Preferences.h>
#include "Logger.h"

class ESP32PreferenceStorage : public PreferenceStorage {
public:
    ESP32PreferenceStorage() = default;
    ~ESP32PreferenceStorage() override {
        prefs.end();  // Simply call end() directly
    }

    bool begin(const char* name, bool readOnly) override;
    bool putString(const char* key, const char* value) override;
    String getString(const char* key, const char* defaultValue) override;
    bool putUInt(const char* key, uint32_t value) override;
    uint32_t getUInt(const char* key, uint32_t defaultValue) override;
    bool remove(const char* key) override;

private:
    Preferences prefs;
};