#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ETH.h>
#include <vector>
#include "ControlTask.h" 
#include "SystemTypes.h"
#include "Config.h"
#include "Logger.h"

class MqttManager {
public:
    MqttManager();
    ~MqttManager();
    
    void begin();
    void setServer(IPAddress ip);
    bool connect();
    void disconnect();
    bool isConnected() { return mqttClient.connected(); }
    bool maintainConnection();
    
    // Publication methods
    bool publish(const char* topic, const char* payload, bool retained = false);
    void publishSensorData(const TemperatureSensor& sensor);
    bool publishRelayState(unsigned char relayId, bool state);
    void publishDisplaySensor(float temperature);
    void publishTelemetryBatch(const std::vector<TemperatureSensor>& sensors);
    
    // Home Assistant Discovery methods
    void publishSensorMetadata(const TemperatureSensor& sensor);
    void publishRelayMetadata();
    
    // Batch operations
    void startBatchPublish();
    void endBatchPublish();
    
    // State management
    void resetConnectionState();

private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;
    IPAddress mqttServerIP;
    String mqttBroker;
    String mqttUsername;
    String mqttPassword;
    unsigned short mqttPort;
    bool hasValidIP;
    unsigned long lastConnectAttempt;
    unsigned long lastSuccessfulConnect;
    uint32_t reconnectDelay;
    uint8_t connectAttempts;
    bool inBatchPublish;
    SemaphoreHandle_t mqttMutex;
    
    enum class ConnState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };
    ConnState connectionState;
    
    // Configuration and setup
    void loadConfiguration();
    bool resolveServer();
    uint32_t calculateBackoff();
    bool acquireMutex(const char* caller);
    void releaseMutex();
    void printDetailedNetworkDiagnostics();
    String getConnectionStateString(ConnState state);
    String getClientId() const;
    
    // Connection handling
    void handleConnectionError();
    
    // Topic creation methods
    String createBaseTopic() const;
    String createStatusTopic() const;
    String createSensorTopic(const uint8_t* address) const;
    String createSwitchTopic(unsigned char relayId) const;
    String createHADiscoveryBaseTopic() const;
    String createHADiscoveryTopic(const uint8_t* address) const;
    String createHADiscoveryTopicForRelay(unsigned char relayId) const;
    String createTBTelemetryTopic() const;
    String createTBAttributesTopic() const;
    
    // Payload creation
    String createHADevicePayload() const;
    void publishDeviceAttributes();
    
    // Utility methods
    String addressToString(const uint8_t* address) const;

    // Control logic
    void onMqttMessage(char* topic, byte* payload, unsigned int length);
};