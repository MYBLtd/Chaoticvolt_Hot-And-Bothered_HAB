// src/MqttManager.cpp (Part 1)
#include "MqttManager.h"
#include "certificates.h"
#include "PreferencesManager.h"
#include <algorithm>
#include "OneWireTask.h"
#include <ArduinoJson.h>
#include <vector>

MqttManager::MqttManager() 
    : wifiClient()
    , mqttClient(wifiClient)
    , hasValidIP(false)
    , lastConnectAttempt(0)
    , lastSuccessfulConnect(0)
    , reconnectDelay(INITIAL_RETRY_DELAY)
    , connectAttempts(0)
    , inBatchPublish(false)
    , connectionState(ConnState::DISCONNECTED) {
    
    Logger::info("Initializing MqttManager");
    
    // Create mutex for thread safety
    mqttMutex = xSemaphoreCreateMutex();
    if (!mqttMutex) {
        Logger::error("Failed to create MQTT mutex");
        return;
    }
    
    // Load and verify root CA certificate
    const char* rootCA = getLetsEncryptRootCA();
    if (!rootCA) {
        Logger::error("Root CA Certificate unavailable!");
        return;
    }

    // Configure SSL client
    wifiClient.setHandshakeTimeout(30000);    // 30 second timeout
    
    // Load certificate with error checking
    try {
        wifiClient.setCACert(rootCA);
        Logger::info("CA certificate loaded successfully");
    } catch (const std::exception& e) {
        Logger::error("Exception during CA cert loading: " + String(e.what()));
        return;
    }

    // Configure MQTT client
    mqttClient.setBufferSize(2048);          // Increased from default
    mqttClient.setSocketTimeout(15);         // Socket timeout in seconds
    mqttClient.setKeepAlive(30);            // Keepalive in seconds
    
    // Additional SSL configuration
    wifiClient.setNoDelay(true);            // Disable Nagle algorithm
    wifiClient.setTimeout(10000);           // Overall timeout for operations
    
    Logger::info("MqttManager initialization complete");
}

MqttManager::~MqttManager() {
    disconnect();
    if (mqttMutex) {
        vSemaphoreDelete(mqttMutex);
    }
}

void MqttManager::begin() {
    Logger::info("Initializing MQTT Handler");
    loadConfiguration();
    
    if (!resolveServer()) {
        Logger::error("Failed to resolve MQTT server");
        return;
    }
    
    mqttClient.setServer(mqttServerIP, mqttPort);
}

void MqttManager::setServer(IPAddress ip) {
    mqttServerIP = ip;
    mqttClient.setServer(ip, mqttPort);
    hasValidIP = true;
}

void MqttManager::loadConfiguration() {
    char broker[MAX_MQTT_SERVER_LENGTH];
    char username[MAX_MQTT_CRED_LENGTH];
    char password[MAX_MQTT_CRED_LENGTH];
    unsigned short port;
    
    PreferencesManager::getMqttConfig(broker, port, username, password);
    
    mqttBroker = String(broker);
    mqttPort = port;
    mqttUsername = String(username);
    mqttPassword = String(password);
    
    if (mqttBroker.length() == 0 || mqttPort == 0) {
        Logger::error("Invalid MQTT configuration");
    }
}

bool MqttManager::resolveServer() {
    static unsigned long lastDnsResolve = 0;
    
    if (hasValidIP && millis() - lastDnsResolve < DNS_CACHE_TIME) {
        Logger::info("Using cached IP: " + mqttServerIP.toString());
        return true;
    }
    
    if (!ETH.linkUp()) {
        Logger::error("Ethernet link is DOWN - cannot resolve server");
        printDetailedNetworkDiagnostics();
        return false;
    }
    
    IPAddress resolved;
    bool dnsResult = WiFi.hostByName(mqttBroker.c_str(), resolved);
    
    if (dnsResult) {
        mqttServerIP = resolved;
        hasValidIP = true;
        lastDnsResolve = millis();
        Logger::info("DNS Resolution Successful: " + mqttServerIP.toString());
        return true;
    }
    
    Logger::error("DNS Resolution FAILED for broker: " + mqttBroker);
    printDetailedNetworkDiagnostics();
    return false;
}

bool MqttManager::maintainConnection() {
    static unsigned long lastReport = 0;
    static int lastState = -99;
    unsigned long now = millis();
    
    // Log connection status every 10 seconds or on state change
    if (now - lastReport > 10000) {
        int currentState = mqttClient.state();
        if (currentState != lastState) {
            Logger::info("MQTT State changed: " + String(lastState) + " -> " + String(currentState));
            lastState = currentState;
        }
        lastReport = now;
    }
    
    // Check network connectivity
    if (!ETH.linkUp()) {
        if (connectionState == ConnState::CONNECTED) {
            disconnect();
        }
        return false;
    }
    
    // Handle current connection state
    if (connectionState == ConnState::CONNECTED) {
        if (!mqttClient.connected()) {
            Logger::warning("Lost MQTT connection");
            disconnect();
        } else {
            mqttClient.loop();
            return true;
        }
    }
    
    if (connectionState == ConnState::CONNECTING) {
        return false;
    }
    
    // Handle reconnection timing
    if (now - lastConnectAttempt < reconnectDelay) {
        return false;
    }
    
    return connect();
}

String MqttManager::createBaseTopic() const {
    return String(SYSTEM_NAME) +  + "/" + String(DEVICE_ID);
}

String MqttManager::createStatusTopic() const {
    return createBaseTopic() + "/status/connection";
}

String MqttManager::createSensorTopic(const uint8_t* address) const {
    return createBaseTopic() + "/temperature/" + 
           String(DEVICE_ID) + "_" + addressToString(address);
}

String MqttManager::createSwitchTopic(unsigned char relayId) const {
    return createBaseTopic() + "/controls/switch" + String(relayId);
}

String MqttManager::createHADiscoveryBaseTopic() const {
    return String(SYSTEM_NAME) +  + "/" + String(DEVICE_ID);  // This means adjusting the prefix path in HA
                                                             //Home Assistant uses homeassistant//<object_id>/config for discovery; 
} //in HA: update the discovery_prefix parameter to point to your discovery path, e.g., {system_name}/devices/{device_id}/$system/discovery/

String MqttManager::createHADiscoveryTopic(const uint8_t* address) const {
    return createHADiscoveryBaseTopic() + "/sensors/" + 
           String(DEVICE_ID) + "_" + addressToString(address) + "/config";
}

String MqttManager::createHADiscoveryTopicForRelay(unsigned char relayId) const {
    return createHADiscoveryBaseTopic() + "/switch/" + 
           String(DEVICE_ID) + "_relay" + String(relayId) + "/config";
}

String MqttManager::createTBTelemetryTopic() const {
    return String(SYSTEM_NAME)  + "/" + "v1/devices/me/telemetry";
}

String MqttManager::createTBAttributesTopic() const {
    return String(SYSTEM_NAME)  + "/" + "v1/devices/me/attributes";
}

String MqttManager::getClientId() const {
    return String(SYSTEM_NAME) + String(DEVICE_ID) + "-" + ETH.macAddress();
}

String MqttManager::addressToString(const uint8_t* address) const {
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X%02X%02X",
             address[0], address[1], address[2], address[3],
             address[4], address[5], address[6], address[7]);
    return String(buffer);
}

void MqttManager::publishTelemetryBatch(const std::vector<TemperatureSensor>& sensors) {
    if (!isConnected()) return;

    DynamicJsonDocument doc(2048);

    for (const auto& sensor : sensors) {
        if (sensor.valid) {
            doc[addressToString(sensor.address)] = sensor.temperature;
        }
    }

    String payload;
    serializeJson(doc, payload);
    publish(createTBTelemetryTopic().c_str(), payload.c_str(), false);
}

bool MqttManager::connect() {
    if (mqttBroker.isEmpty() || mqttPort == 0) {
        Logger::error("Invalid MQTT configuration");
        return false;
    }

    Logger::info("MQTT Connection Attempt:"
        "\n  - Broker: " + mqttBroker +
        "\n  - Port: " + String(mqttPort)
    );

    disconnect();
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);

    String clientId = getClientId();
    String statusTopic = createStatusTopic();

    connectionState = ConnState::CONNECTING;
    lastConnectAttempt = millis();

    // Set up message callback before connecting
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->onMqttMessage(topic, payload, length);
    });

    bool result = mqttClient.connect(
        clientId.c_str(), 
        mqttUsername.length() > 0 ? mqttUsername.c_str() : nullptr, 
        mqttPassword.length() > 0 ? mqttPassword.c_str() : nullptr,
        statusTopic.c_str(), 
        1,    // QoS for will message
        true, // Retain will message
        "offline"
    );

    if (result) {
        Logger::info("MQTT Connection Successful");
        connectionState = ConnState::CONNECTED;
        lastSuccessfulConnect = millis();
        connectAttempts = 0;
        reconnectDelay = INITIAL_RETRY_DELAY;
        
        // Publish online status
        mqttClient.publish(statusTopic.c_str(), "online", true);
        
        // Subscribe to relay control topics
        bool subscribeSuccess = true;
        for (int i = 0; i < 2; i++) {
            String controlTopic = createSwitchTopic(i) + "/set";
            Logger::debug("Subscribing to relay control topic: " + controlTopic);
            
            if (!mqttClient.subscribe(controlTopic.c_str(), 1)) {  // QoS 1 for reliability
                Logger::error("Failed to subscribe to: " + controlTopic);
                subscribeSuccess = false;
            }
        }

        if (!subscribeSuccess) {
            Logger::warning("Some MQTT subscriptions failed - system will retry on next connection");
        }
        
        // Force immediate metadata publication on connection
        const auto& sensors = OneWireTask::getManager().getSensorList();
        startBatchPublish();
        for (const auto& sensor : sensors) {
            publishSensorMetadata(sensor);
        }
        publishRelayMetadata();
        publishDeviceAttributes();
        endBatchPublish();
        
        return true;
    } else {
        handleConnectionError();
        return false;
    }
}

void MqttManager::handleConnectionError() {
    int state = mqttClient.state();
    Logger::error("MQTT Connection Failed:"
        "\n  - MQTT State: " + String(state) +
        "\n  - Socket Connected: " + String(wifiClient.connected() ? "YES" : "NO") +
        "\n  - Connection Attempts: " + String(connectAttempts + 1)
    );

    switch(state) {
        case -4: 
            Logger::error("Connection Timeout - Check network latency and server responsiveness");
            break;
        case -3: 
            Logger::error("Connection Lost - Possible network instability or server issues");
            break;
        case -2: 
            Logger::error("Connection Failed - Possible SSL/TLS negotiation failure");
            break;
        case -1: 
            Logger::error("Disconnected - Check credentials and server configuration");
            break;
        default: 
            Logger::error("Unknown Connection Failure - State: " + String(state));
            break;
    }

    connectionState = ConnState::ERROR;
    connectAttempts++;
    reconnectDelay = calculateBackoff();
}

void MqttManager::publishDeviceAttributes() {
if (!isConnected()) return;

StaticJsonDocument<512> doc;

doc["manufacturer"] = DEVICE_MANUFACTURER;
doc["model"] = DEVICE_MODEL;
doc["firmware_version"] = FIRMWARE_VERSION;
doc["device_id"] = DEVICE_ID;
doc["system_name"] = SYSTEM_NAME;

String payload;
serializeJson(doc, payload);
publish(createTBAttributesTopic().c_str(), payload.c_str(), true);
}

bool MqttManager::publishRelayState(unsigned char relayId, bool state) {
if (!isConnected()) return false;

// Home Assistant state
String haTopic = createSwitchTopic(relayId) + "/state";
const char* haPayload = state ? "ON" : "OFF";

// ThingsBoard state
StaticJsonDocument<128> tbDoc;
tbDoc["relay_" + String(relayId)] = state;
String tbPayload;
serializeJson(tbDoc, tbPayload);

bool success = true;
success &= publish(haTopic.c_str(), haPayload, true);
success &= publish(createTBTelemetryTopic().c_str(), tbPayload.c_str(), false);

return success;
}

String MqttManager::createHADevicePayload() const {
StaticJsonDocument<512> doc;

JsonObject device = doc.createNestedObject("device");
device["identifiers"].add(String(SYSTEM_NAME) + "_" + DEVICE_ID);
device["name"] = String(SYSTEM_NAME) + " " + DEVICE_ID;
device["manufacturer"] = DEVICE_MANUFACTURER;
device["model"] = DEVICE_MODEL;
device["sw_version"] = FIRMWARE_VERSION;

String payload;
serializeJson(doc, payload);
return payload;
}

void MqttManager::publishSensorMetadata(const TemperatureSensor& sensor) {
    if (!isConnected()) return;
    
    String sensorId = addressToString(sensor.address);
    String configTopic = createHADiscoveryTopic(sensor.address);
    String stateTopic = createSensorTopic(sensor.address) + "/temperature";
    String availabilityTopic = createStatusTopic();
    
    String payload = "{"
        "\"device\": {"
            "\"identifiers\": [\"" + String(SYSTEM_NAME) + "_" + DEVICE_ID + "\"],"
            "\"name\": \"" + String(SYSTEM_NAME) + " " + DEVICE_ID + "\","
            "\"manufacturer\": \"" + String(DEVICE_MANUFACTURER) + "\","
            "\"model\": \"" + String(DEVICE_MODEL) + "\","
            "\"sw_version\": \"" + String(FIRMWARE_VERSION) + "\","
            "\"configuration_url\": \"http://" + ETH.localIP().toString() + "\""
        "},"
        "\"name\": \"Temperature Sensor " + sensorId + "\","
        "\"unique_id\": \"" + String(SYSTEM_NAME) + "_" + DEVICE_ID + "_temp_" + sensorId + "\","
        "\"device_class\": \"temperature\","
        "\"state_topic\": \"" + stateTopic + "\","
        "\"unit_of_measurement\": \"°C\","
        "\"value_template\": \"{{ value | float }}\","
        "\"expire_after\": 600,"
        "\"availability_topic\": \"" + availabilityTopic + "\","
        "\"payload_available\": \"online\","
        "\"payload_not_available\": \"offline\""
    "}";

    publish(configTopic.c_str(), payload.c_str(), true);
}

void MqttManager::publishRelayMetadata() {
    if (!isConnected()) return;
    
    for (int i = 0; i < 2; i++) {
        String configTopic = createHADiscoveryTopicForRelay(i);
        String switchTopic = createSwitchTopic(i);
        String availabilityTopic = createStatusTopic();
        
        String payload = "{"
            "\"device\": {"
                "\"identifiers\": [\"" + String(SYSTEM_NAME) + "_" + DEVICE_ID + "\"],"
                "\"name\": \"" + String(SYSTEM_NAME) + " " + DEVICE_ID + "\","
                "\"manufacturer\": \"" + String(DEVICE_MANUFACTURER) + "\","
                "\"model\": \"" + String(DEVICE_MODEL) + "\","
                "\"sw_version\": \"" + String(FIRMWARE_VERSION) + "\","
                "\"configuration_url\": \"http://" + ETH.localIP().toString() + "\""
            "},"
            "\"name\": \"Relay " + String(i + 1) + "\","
            "\"unique_id\": \"" + String(SYSTEM_NAME) + "_" + DEVICE_ID + "_relay" + String(i) + "\","
            "\"entity_category\": \"config\","
            "\"command_topic\": \"" + switchTopic + "/set\","
            "\"state_topic\": \"" + switchTopic + "/state\","
            "\"availability_topic\": \"" + availabilityTopic + "\","
            "\"payload_available\": \"online\","
            "\"payload_not_available\": \"offline\","
            "\"payload_on\": \"ON\","
            "\"payload_off\": \"OFF\","
            "\"state_on\": \"ON\","
            "\"state_off\": \"OFF\""
        "}";
        
        publish(configTopic.c_str(), payload.c_str(), true);
    }
}

void MqttManager::startBatchPublish() {
if (acquireMutex("startBatch")) {
    inBatchPublish = true;
}
}

void MqttManager::endBatchPublish() {
if (inBatchPublish) {
    inBatchPublish = false;
    releaseMutex();
}
}

bool MqttManager::acquireMutex(const char* caller) {
static const unsigned long MUTEX_TIMEOUT = 2000;

if (!mqttMutex) {
    Logger::error("MUTEX NOT INITIALIZED - Called from: " + String(caller));
    return false;
}

if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT)) != pdTRUE) {
    Logger::error("MUTEX ACQUISITION FAILED - Caller: " + String(caller));
    return false;
}

return true;
}

void MqttManager::releaseMutex() {
    if (mqttMutex) {
        xSemaphoreGive(mqttMutex);
    } else {
        Logger::error("Attempt to release null mutex");
    }
}

void MqttManager::resetConnectionState() {
        connectionState = ConnState::DISCONNECTED;
        hasValidIP = false;
        lastConnectAttempt = 0;
        lastSuccessfulConnect = 0;
        reconnectDelay = INITIAL_RETRY_DELAY;
        connectAttempts = 0;
    }

uint32_t MqttManager::calculateBackoff() {
    uint32_t delay = INITIAL_RETRY_DELAY * (1 << connectAttempts);
    return std::min<uint32_t>(delay, MAX_RETRY_DELAY);
}

String MqttManager::getConnectionStateString(ConnState state) {
    switch(state) {
    case ConnState::DISCONNECTED: return "DISCONNECTED";
    case ConnState::CONNECTING: return "CONNECTING";
    case ConnState::CONNECTED: return "CONNECTED";
    case ConnState::ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

void MqttManager::printDetailedNetworkDiagnostics() {
Logger::info("Detailed Network Diagnostics:"
    "\n  - Ethernet Link: " + String(ETH.linkUp() ? "UP" : "DOWN") +
    "\n  - Local IP: " + ETH.localIP().toString() +
    "\n  - Gateway IP: " + ETH.gatewayIP().toString() +
    "\n  - DNS: " + ETH.dnsIP(0).toString() +
    "\n  - MQTT Broker: " + mqttBroker +
    "\n  - MQTT IP: " + mqttServerIP.toString() +
    "\n  - State: " + getConnectionStateString(connectionState) +
    "\n  - Heap: " + String(ESP.getFreeHeap()) + " bytes"
);
}

bool MqttManager::publish(const char* topic, const char* payload, bool retained) {
    if (!isConnected()) return false;
    
    static unsigned long lastPublish = 0;
    static size_t totalBytes = 0;
    unsigned long now = millis();
    
    if (now - lastPublish > 1000) {
        totalBytes = 0;
    }
    
    size_t messageSize = strlen(topic) + strlen(payload) + 2;
    if (totalBytes + messageSize > 2048) {
        delay(50);
        totalBytes = 0;
    }
    
    if (!inBatchPublish) {
        if (!acquireMutex("publish")) return false;
    }
    
    bool success = mqttClient.publish(topic, payload, retained);
    lastPublish = millis();
    totalBytes += messageSize;
    
    if (!inBatchPublish) {
        releaseMutex();
    }
    
    return success;
}

void MqttManager::disconnect() {
    if (mqttClient.connected()) {
        String statusTopic = createStatusTopic();
        mqttClient.publish(statusTopic.c_str(), "offline", true);
        mqttClient.disconnect();
    }
    
    wifiClient.stop();
    
    connectionState = ConnState::DISCONNECTED;
    hasValidIP = false;
}

void MqttManager::publishSensorData(const TemperatureSensor& sensor) {
    if (!isConnected()) return;
    
    if (sensor.valid) {
        // Home Assistant format
        char tempStr[10];
        snprintf(tempStr, sizeof(tempStr), "%.1f", sensor.temperature);
        String haTopic = createSensorTopic(sensor.address) + "/temperature";
        if (publish(haTopic.c_str(), tempStr, true)) {
            Logger::debug("Published sensor: " + String(tempStr));
        }
        
        // ThingsBoard format
        DynamicJsonDocument doc(128);
        doc[addressToString(sensor.address)] = sensor.temperature;
        String tbPayload;
        serializeJson(doc, tbPayload);
        publish(createTBTelemetryTopic().c_str(), tbPayload.c_str(), false);
    }
}

void MqttManager::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = String((char*)payload, length);
    
    // Check if it's a relay control topic
    for (int i = 0; i < 2; i++) {
        String controlTopic = createSwitchTopic(i) + "/set";
        if (topicStr == controlTopic) {
            bool state = (payloadStr == "ON");
            ControlTask::updateRelayRequest(i, state);
            return;
        }
    }
}