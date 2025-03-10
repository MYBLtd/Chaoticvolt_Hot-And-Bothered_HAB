// src/main.cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include <ETH.h>
#include <nvs_flash.h>
#include "Config.h"
#include "SystemTypes.h"
#include "OneWireTask.h"
#include "NetworkTask.h"
#include "ControlTask.h"
#include "Logger.h"
#include "SystemHealth.h"
#include <esp_task_wdt.h>
#include <esp_core_dump.h>
#include "PreferencesManager.h"
#include "SslTest.h"
#include "AuthManager.h"
#include "NtpManager.h"
#include "WebServer.h"

WebServer webServer(OneWireTask::getManager());


void prepareNetworkForSsl() {
    // Wait for stable network connection
    uint8_t timeout = 0;
    while (!ETH.linkUp() && timeout < 30) {  // 30 second timeout
        Logger::info("Waiting for stable network connection...");
        delay(1000);
        timeout++;
    }
    
    if (!ETH.linkUp()) {
        Logger::error("Network not ready for SSL test");
        return;
    }
    
    // Give DNS a moment to initialize
    delay(1000);
    
    IPAddress ip = ETH.localIP();
    Logger::info("Network ready for SSL test");
    Logger::info("IP: " + ip.toString());
    Logger::info("DNS: " + ETH.dnsIP().toString());
}

bool testSslStack() {
    Logger::info("Testing SSL stack before service initialization");
    
    if (!SslTest::runTests()) {
        Logger::error("SSL stack tests failed");
        return false;
    }
    
    Logger::info("SSL stack tests passed successfully");
    return true;
}

void handleCredentialReset() {
    Logger::info("Reset button held - waiting for confirmation");
    uint32_t buttonHoldStart = millis();
    bool resetTriggered = false;
    
    while (digitalRead(CREDENTIAL_RESET_PIN) == LOW) {
        if ((millis() - buttonHoldStart) > CREDENTIAL_RESET_TIME) {
            resetTriggered = true;
            break;
        }
        delay(100);
    }
    
    if (resetTriggered) {
        Logger::info("Reset confirmed - resetting credentials");
        AuthManager::reset();
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    pinMode(CREDENTIAL_RESET_PIN, INPUT_PULLUP);

    Logger::setLogLevel(Logger::Level::INFO);
    Logger::info("System starting...");
    
    // Initialize SPIFFS first
    if(!SPIFFS.begin(true)) {
        Logger::error("SPIFFS mount failed");
        return;
    }
    Logger::info("SPIFFS mounted successfully");

    // List all files in SPIFFS
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) {
        Logger::info("Found file: " + String(file.name()) + " (" + String(file.size()) + " bytes)");
        file = root.openNextFile();
    }
    
    // Initialize preferences before checking reset
    PreferencesManager::init();
    Logger::info("Preferences initialized");

    // Check for credential reset - must be held for 10 seconds
    if (digitalRead(CREDENTIAL_RESET_PIN) == LOW) {
        Logger::info("Reset button held - waiting for confirmation");
        uint32_t buttonHoldStart = millis();
        bool resetTriggered = false;
        
        while (digitalRead(CREDENTIAL_RESET_PIN) == LOW) {
            if ((millis() - buttonHoldStart) > CREDENTIAL_RESET_TIME) {
                resetTriggered = true;
                break;
            }
            delay(100);
        }
        
        if (resetTriggered) {
            Logger::info("Reset confirmed - resetting credentials");
            AuthManager::reset();
        }
    }

    // Initialize remaining components
    AuthManager::init();
    Logger::info("Auth Manager initialized");
    
    // Continue with rest of setup...
    Logger::info("Starting Ethernet initialization...");
    delay(100);
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, 
              ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
    delay(400);

    uint8_t timeout = 0;
    while (!ETH.linkUp() && timeout < 20) {
        Logger::info("Waiting for Ethernet (Heap: " + String(ESP.getFreeHeap()) + " bytes)");
        delay(1000);
        timeout++;
    }

    if (!ETH.linkUp()) {
        Logger::error("Ethernet connection failed!");
        return;
    }

    Logger::info("Ethernet connected!");
    Logger::info("IP address: " + ETH.localIP().toString());    
    Logger::info("Initializing system components...");

    esp_core_dump_init();
    Logger::info("Core dump initialized");

    // Initialize NTP early
    NtpManager::init();
    Logger::info("Waiting for initial NTP sync");
    
    // Wait for NTP with a longer timeout during boot
    if (!NtpManager::waitForSync(60000)) {  // 60 second timeout during boot
        Logger::error("Failed to get initial NTP sync - continuing with initialization");
    } else {
        Logger::info("Initial NTP sync complete");
    }

    Logger::info("Preparing network for SSL test");
    prepareNetworkForSsl();
    
    if (!testSslStack()) {
        Logger::error("SSL stack tests failed - halting initialization");
        return;
    }

    PreferencesManager::init();
    Logger::info("Preferences initialized");

    AuthManager::init();
    Logger::info("Auth Manager initialized");

    SystemHealth::init();
    Logger::info("System health initialized");

    ControlTask::init();
    ControlTask::start();
    Logger::info("Control task started");

    OneWireTask::init();
    OneWireTask::start();
    Logger::info("OneWire task started");

    NetworkTask::init();
    NetworkTask::start();
    
    Logger::info("Network task started");

    esp_task_wdt_init(WATCHDOG_TIMEOUT / 3000, true);
    esp_task_wdt_add(nullptr);

    AuthManager::init();
    webServer.begin();
    Logger::info("Web server started");
    
    Logger::info("Setup complete - system running");
}

void loop() {
    esp_task_wdt_reset();
    SystemHealth::update();
    vTaskDelay(pdMS_TO_TICKS(1000));
}