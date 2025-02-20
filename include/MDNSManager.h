// include/MDNSManager.h
#pragma once

#include <Arduino.h>
#include <ETH.h>
#include <ESPmDNS.h>
#include "Logger.h"
#include "Config.h"

class MDNSManager {
public:
    static bool init();
    static void update();
    static bool isInitialized() { return initialized; }
    static void addService(const char* service, const char* proto, uint16_t port);
    static void removeService(const char* service, const char* proto);

private:
    static bool initialized;
    static constexpr uint32_t UPDATE_INTERVAL = 10000; // 10 second interval
    static uint32_t lastUpdate;
};
