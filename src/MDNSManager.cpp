// src/MDNSManager.cpp
#include "MDNSManager.h"

bool MDNSManager::initialized = false;
uint32_t MDNSManager::lastUpdate = 0;

bool MDNSManager::init() {
    if (initialized) {
        return true;
    }

    // Wait for network to be ready
    if (!ETH.linkUp() || ETH.localIP().toString() == "0.0.0.0") {
        Logger::warning("Network not ready for mDNS initialization");
        return false;
    }

    Logger::info("Initializing mDNS...");
    
    if (!MDNS.begin(MDNS_HOSTNAME)) {
        Logger::error("Failed to start mDNS responder");
        return false;
    }

    // Add device identification
    MDNS.setInstanceName(DEVICE_NAME);
    Logger::info("Set mDNS instance name: " + String(DEVICE_NAME));

    // Add base HTTP service with extended information
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "manufacturer", DEVICE_MANUFACTURER);
    MDNS.addServiceTxt("http", "tcp", "model", DEVICE_MODEL);
    MDNS.addServiceTxt("http", "tcp", "version", DEVICE_VERSION);
    MDNS.addServiceTxt("http", "tcp", "firmware", FIRMWARE_VERSION);
    MDNS.addServiceTxt("http", "tcp", "type", "sensor-hub");
    MDNS.addServiceTxt("http", "tcp", "mac", ETH.macAddress());
    MDNS.addServiceTxt("http", "tcp", "ip", ETH.localIP().toString());
    Logger::info("Added HTTP service with device information");

    initialized = true;
    lastUpdate = millis();
    Logger::info("mDNS responder started successfully");
    return true;
}
void MDNSManager::update() {
    if (!initialized) {
        return;
    }

    uint32_t now = millis();
    if (now - lastUpdate >= UPDATE_INTERVAL) {
        // For ESP32, just ensure MDNS is running
        if (!MDNS.begin(MDNS_HOSTNAME)) {
            Logger::warning("mDNS responder restart failed");
            initialized = false;
        }
        lastUpdate = now;
    }
}

void MDNSManager::addService(const char* service, const char* proto, uint16_t port) {
    if (!initialized) {
        Logger::warning("Cannot add mDNS service - not initialized");
        return;
    }
    MDNS.addService(service, proto, port);
    Logger::info("Added mDNS service: " + String(service) + "." + String(proto));
}

void MDNSManager::removeService(const char* service, const char* proto) {
    if (!initialized) {
        return;
    }
    // ESP32 mDNS doesn't support direct service removal
    // Log the attempt but don't take action
    Logger::info("Note: Service removal requested but not supported on ESP32: " + 
                 String(service) + "." + String(proto));
}
