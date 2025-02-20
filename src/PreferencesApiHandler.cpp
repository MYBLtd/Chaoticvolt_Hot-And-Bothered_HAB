// src/PreferencesApiHandler.cpp
#include "PreferencesApiHandler.h"
#include <Arduino.h>
#include "PreferencesManager.h"

String PreferencesApiHandler::handleGet() {
    Logger::debug("Building preferences JSON response");
    
    DynamicJsonDocument doc(16384);
    JsonObject root = doc.to<JsonObject>();
    
    // Add MQTT settings
    JsonObject mqtt = root.createNestedObject("mqtt");
    
    // Get MQTT configuration using the correct method
    char server[MAX_MQTT_SERVER_LENGTH] = {0};
    char username[MAX_MQTT_CRED_LENGTH] = {0};
    char password[MAX_MQTT_CRED_LENGTH] = {0};
    unsigned short port = 0;
    
    PreferencesManager::getMqttConfig(server, port, username, password);
    
    // Only add values that are properly initialized
    if (strlen(server) > 0) {
        mqtt["broker"] = server;
    }
    if (port > 0) {
        mqtt["port"] = port;
    }
    if (strlen(username) > 0) {
        mqtt["username"] = username;
    }
    
    // Add sensor mappings
    JsonObject sensors = root.createNestedObject("sensors");
    const auto& sensorList = oneWireManager.getSensorList();  // Use oneWireManager instead of owManager
    
    for (const auto& sensor : sensorList) {
        String addr = PreferencesManager::addressToString(sensor.address);  // Use PreferencesManager's method
        String name = PreferencesManager::getSensorName(sensor.address);
        if (name.length() > 0) {
            sensors[addr] = name;
        }
    }
    
    String output;
    serializeJson(doc, output);
    Logger::debug("Generated preferences JSON: " + output);
    
    return output;
}

void PreferencesApiHandler::addMqttConfigToJson(JsonObject& root) {
    JsonObject mqtt = root.createNestedObject("mqtt");
    
    // Get current MQTT configuration
    char server[MAX_MQTT_SERVER_LENGTH] = {0};
    char username[MAX_MQTT_CRED_LENGTH] = {0};
    char password[MAX_MQTT_CRED_LENGTH] = {0};
    unsigned short port = 0;
    
    PreferencesManager::getMqttConfig(server, port, username, password);
    
    // Only add values that are properly initialized
    if (strlen(server) > 0) {
        mqtt["broker"] = server;
    }
    if (port > 0) {
        mqtt["port"] = port;
    }
    if (strlen(username) > 0) {
        mqtt["username"] = username;
    }
    // Never include password in response
}

void PreferencesApiHandler::addScanningConfigToJson(JsonObject& root) {
    JsonObject scanning = root.createNestedObject("scanning");
    scanning["autoScanEnabled"] = PreferencesManager::getAutoScanEnabled();
    scanning["scanInterval"] = PreferencesManager::getScanInterval();
}

void PreferencesApiHandler::addDisplayConfigToJson(JsonObject& root) {
    JsonObject display = root.createNestedObject("display");
    
    // Get currently selected sensor for display
    uint8_t displaySensorAddr[8];
    PreferencesManager::getDisplaySensor(displaySensorAddr);
    String sensorAddr = PreferencesManager::addressToString(displaySensorAddr);
    
    // Only add non-empty sensor address
    if (sensorAddr != "0000000000000000") {
        display["selectedSensor"] = sensorAddr;
    }
    
    // Add display settings
    display["brightnessLevel"] = 7;  // Default brightness
    display["displayTimeout"] = 30;   // Default timeout in seconds
}

void PreferencesApiHandler::addSensorNamesToJson(JsonObject& root) {
    JsonArray sensors = root.createNestedArray("sensors");
    
    // Add size check
    if (!sensors.isNull()) {
        const auto& sensorList = oneWireManager.getSensorList();
        
        for (const auto& sensor : sensorList) {
            // Check available heap before allocation
            if (ESP.getFreeHeap() < 1024) {  // Minimum safe threshold
                Logger::error("Insufficient heap for sensor JSON");
                break;
            }
            
            JsonObject sensorObj = sensors.createNestedObject();
            if (sensorObj.isNull()) {
                Logger::error("Failed to create sensor object");
                continue;
            }
            
            String addr = PreferencesManager::addressToString(sensor.address);
            // Only attempt to get name if address is valid
            String name = addr.length() > 0 ? 
                         PreferencesManager::getSensorName(sensor.address) : "";
            
            sensorObj["address"] = addr;
            if (name.length() > 0) {
                sensorObj["name"] = name;
            }
            sensorObj["temperature"] = sensor.temperature;
            sensorObj["valid"] = sensor.valid;
        }
    }
}

bool PreferencesApiHandler::handlePost(const String& jsonData) {
    DynamicJsonDocument doc(16384);  // Increased size for safety
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        Logger::error("JSON parsing failed: " + String(error.c_str()));
        return false;
    }
    
    Logger::info("Processing preferences update request");
    String debug;
    serializeJson(doc, debug);
    Logger::debug("Received data: " + debug);
    
    bool success = true;
    bool mqttUpdated = false;
    bool scanningUpdated = false;
    bool sensorsUpdated = false;
    bool relaysUpdated = false;
    bool displayUpdated = false;
    
    // Process MQTT settings
    if (doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        if (validateMqttConfig(mqtt)) {
            if (updateMqttConfig(mqtt)) {
                Logger::info("MQTT configuration saved");
                mqttUpdated = true;
            } else {
                Logger::error("Failed to save MQTT configuration");
                success = false;
            }
        } else {
            Logger::error("Invalid MQTT configuration");
            success = false;
        }
    }
    
    // Process sensor names
    if (doc.containsKey("sensors")) {
        JsonVariant sensors = doc["sensors"];
        Logger::debug("Processing sensors update");
        if (updateSensorNames(sensors)) {
            Logger::info("Sensor names updated successfully");
            sensorsUpdated = true;
        } else {
            Logger::error("Failed to update sensor names");
            success = false;
        }
    }
    
    // Process relay names
    if (doc.containsKey("relays")) {
        JsonArray relays = doc["relays"];
        Logger::debug("Processing relay names update");
        if (updateRelayNames(relays)) {
            Logger::info("Relay names updated successfully");
            relaysUpdated = true;
        } else {
            Logger::error("Failed to update relay names");
            success = false;
        }
    }
    
    // Process scanning settings
    if (doc.containsKey("scanning")) {
        JsonObject scanning = doc["scanning"];
        if (validateScanningConfig(scanning)) {
            if (updateScanningConfig(scanning)) {
                Logger::info("Scanning configuration saved");
                scanningUpdated = true;
            } else {
                Logger::error("Failed to save scanning configuration");
                success = false;
            }
        } else {
            Logger::error("Invalid scanning configuration");
            success = false;
        }
    }
    
    // Process display settings
    if (doc.containsKey("display")) {
        JsonObject display = doc["display"];
        if (validateDisplayConfig(display)) {
            if (updateDisplayConfig(display)) {
                Logger::info("Display configuration saved");
                displayUpdated = true;
            } else {
                Logger::error("Failed to save display configuration");
                success = false;
            }
        } else {
            Logger::error("Invalid display configuration");
            success = false;
        }
    }
    
    // Log summary
    String updateSummary = "Updates completed - ";
    updateSummary += mqttUpdated ? "MQTT:✓ " : "MQTT:✗ ";
    updateSummary += sensorsUpdated ? "Sensors:✓ " : "Sensors:✗ ";
    updateSummary += relaysUpdated ? "Relays:✓ " : "Relays:✗ ";
    updateSummary += scanningUpdated ? "Scanning:✓ " : "Scanning:✗ ";
    updateSummary += displayUpdated ? "Display:✓ " : "Display:✗ ";
    Logger::info(updateSummary);
    
    return success;
}

bool PreferencesApiHandler::validateMqttConfig(JsonObject& mqtt) {
    if (!mqtt.containsKey("broker") || !mqtt.containsKey("port")) {
        Logger::error("Missing required MQTT fields");
        return false;
    }
    
    const char* broker = mqtt["broker"];
    uint16_t port = mqtt["port"];
    
    if (!broker || strlen(broker) == 0 || strlen(broker) >= MAX_MQTT_SERVER_LENGTH) {
        Logger::error("Invalid MQTT broker address");
        return false;
    }
    
    if (!validateHostname(broker)) {
        Logger::error("Invalid MQTT broker hostname format");
        return false;
    }
    
    if (port < 1 || port > 65535) {
        Logger::error("Invalid MQTT port number");
        return false;
    }
    
    // Optional credential validation
    if (mqtt.containsKey("username")) {
        const char* username = mqtt["username"];
        if (strlen(username) >= MAX_MQTT_CRED_LENGTH) {
            Logger::error("MQTT username too long");
            return false;
        }
    }
    
    return true;
}

bool PreferencesApiHandler::validateScanningConfig(JsonObject& scanning) {
    bool isValid = true;
    
    if (scanning.containsKey("scanInterval")) {
        uint32_t interval = scanning["scanInterval"];
        if (interval < MIN_SCAN_INTERVAL || interval > MAX_SCAN_INTERVAL) {
            Logger::error("Invalid scan interval");
            isValid = false;
        }
    }
    
    return isValid;
}

bool PreferencesApiHandler::validateDisplayConfig(JsonObject& display) {
    bool isValid = true;
    
    if (display.containsKey("selectedSensor")) {
        const char* sensorAddr = display["selectedSensor"];
        if (!sensorAddr || strlen(sensorAddr) != 16) {
            Logger::error("Invalid sensor address format");
            isValid = false;
        }
    }
    
    if (display.containsKey("brightnessLevel")) {
        int brightness = display["brightnessLevel"] | -1;
        if (brightness < 1 || brightness > 15) {
            Logger::error("Invalid brightness level (must be 1-15)");
            isValid = false;
        }
    }
    
    if (display.containsKey("displayTimeout")) {
        int timeout = display["displayTimeout"] | -1;
        if (timeout < 0 || timeout > 3600) {
            Logger::error("Invalid display timeout (must be 0-3600)");
            isValid = false;
        }
    }
    
    return isValid;
}

bool PreferencesApiHandler::updateSensorNames(JsonVariant sensors) {
    if (!sensors.is<JsonObject>()) {
        Logger::error("Invalid sensors data format - expected object");
        return false;
    }
    
    JsonObject sensorObj = sensors.as<JsonObject>();
    Logger::debug("Processing " + String(sensorObj.size()) + " sensor names");
    
    bool success = true;
    for (JsonPair kvp : sensorObj) {
        const char* address = kvp.key().c_str();
        const char* name = kvp.value().as<const char*>();
        
        Logger::debug("Setting name for sensor " + String(address) + " -> " + String(name));
        
        if (strlen(address) != 16) {
            Logger::error("Invalid sensor address length: " + String(address));
            success = false;
            continue;
        }
        
        uint8_t addr[8];
        PreferencesManager::stringToAddress(String(address), addr);
        
        if (!PreferencesManager::setSensorName(addr, name)) {
            Logger::error("Failed to save name for sensor: " + String(address));
            success = false;
        } else {
            Logger::info("Successfully saved name for sensor: " + String(address));
        }
    }
    
    return success;
}



bool PreferencesApiHandler::updateMqttConfig(JsonObject& mqtt) {
    const char* broker = mqtt["broker"];
    uint16_t port = mqtt["port"];
    const char* username = mqtt["username"] | "";
    const char* password = mqtt["password"] | "";
    
    return PreferencesManager::setMqttConfig(broker, port, username, password);
}

bool PreferencesApiHandler::updateScanningConfig(JsonObject& scanning) {
    if (scanning.containsKey("autoScanEnabled")) {
        PreferencesManager::setAutoScanEnabled(scanning["autoScanEnabled"]);
    }
    
    if (scanning.containsKey("scanInterval")) {
        uint32_t interval = scanning["scanInterval"];
        if (interval >= MIN_SCAN_INTERVAL && interval <= MAX_SCAN_INTERVAL) {
            PreferencesManager::setScanInterval(interval);
        }
    }
    
    return true;
}

bool PreferencesApiHandler::updateDisplayConfig(JsonObject& display) {
    bool success = true;
    
    if (display.containsKey("selectedSensor")) {
        Logger::debug("Display sensor selection update requested");
        const char* sensorAddr = display["selectedSensor"];
        Logger::debug("Selected sensor address: " + String(sensorAddr));
        
        uint8_t address[8];
        PreferencesManager::stringToAddress(sensorAddr, address);
        
        // Log the converted address
        String addrStr;
        for (int i = 0; i < 8; i++) {
            if (i > 0) addrStr += ":";
            addrStr += String(address[i], HEX);
        }
        Logger::debug("Converting address string to bytes: " + addrStr);
        
        success = PreferencesManager::setDisplaySensor(address);
        Logger::debug("Display sensor update " + String(success ? "succeeded" : "failed"));
    }
    
    return success;
}

bool PreferencesApiHandler::validateHostname(const char* hostname) {
    if (!hostname || strlen(hostname) == 0) {
        return false;
    }

    for (size_t i = 0; hostname[i]; i++) {
        char c = hostname[i];
        if (!isalnum(c) && c != '.' && c != '-' && c != ':') {
            return false;
        }
    }

    if (strstr(hostname, "..") || strstr(hostname, "--")) {
        return false;
    }

    size_t len = strlen(hostname);
    if (hostname[0] == '.' || hostname[len-1] == '.' ||
        hostname[0] == '-' || hostname[len-1] == '-') {
        return false;
    }

    return true;
}

bool PreferencesApiHandler::updateRelayNames(JsonArray& relays) {
    Logger::debug("Processing " + String(relays.size()) + " relay names");
    bool success = true;

    for (JsonObject relay : relays) {
        if (!relay.containsKey("relay_id") || !relay.containsKey("name")) {
            Logger::error("Invalid relay data format");
            continue;
        }

        int relayId = relay["relay_id"];
        const char* name = relay["name"];

        Logger::debug("Setting name for relay " + String(relayId) + " -> " + String(name));

        if (!PreferencesManager::setRelayName(relayId, name)) {
            Logger::error("Failed to save name for relay " + String(relayId));
            success = false;
        } else {
            Logger::info("Successfully saved name for relay " + String(relayId));
        }
    }

    return success;
}
