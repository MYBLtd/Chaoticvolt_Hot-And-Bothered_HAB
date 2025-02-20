// include/SystemTypes.h
#pragma once

#include <cstdint>
#include "OneWireManager.h"  // This should contain TemperatureSensor definition

enum class MessageType {
    RELAY_CHANGE_REQUEST,
    SENSOR_SCAN_REQUEST,
    TEMPERATURE_READ_REQUEST,
    SENSOR_DATA,
    RELAY_STATE
};

struct RelayChangeRequest {
    uint8_t relayId;
    bool state;
};

struct RelayStateData {  // Renamed to avoid confusion
    uint8_t id;
    bool state;
};

// Temperature sensor data structure
struct TemperatureSensor {
    uint8_t address[8];                              // Sensor's unique address
    char friendlyName[MAX_FRIENDLY_NAME_LENGTH];     // Human-readable name
    float temperature;                               // Current temperature reading
    float lastValidReading;                         // Last known good reading
    uint32_t lastReadTime;                          // Timestamp of last reading
    uint8_t consecutiveErrors;                      // Error tracking
    bool isActive;                                  // Whether sensor is currently responding
    bool valid;                                     // Whether current reading is valid
};

union MessageData {
    RelayChangeRequest relayChange;
    RelayStateData relayState;    // Updated name
    TemperatureSensor sensorData; // Explicit name for sensor data
};

struct TaskMessage {
    MessageType type;
    MessageData data;
};

// Data structure for MQTT publish messages
struct MqttPublishData {
    char topic[128];
    char payload[256];
    bool retained;
};

// Home Assistant Discovery data structure
struct HadDiscoveryData {
    char deviceId[32];
    char friendlyName[MAX_FRIENDLY_NAME_LENGTH];
    char topic[128];
    char deviceClass[32];
    char valueTemplate[128];
    bool isSwitch;
};

// Device status enumeration
enum class DeviceStatus : uint8_t {
    OK = 0,
    ERROR = 1,
    DISCONNECTED = 2,
    INITIALIZING = 3
};

// Sensor types enumeration
enum class SensorType : uint8_t {
    TEMPERATURE = 0,
    HUMIDITY = 1,
    PRESSURE = 2,
    UNKNOWN = 255
};

// Display mode enumeration
enum class DisplayMode : uint8_t {
    NORMAL = 0,
    ERROR = 1,
    TEST = 2,
    OFF = 3
};

// Temperature scale enumeration
enum class TemperatureScale : uint8_t {
    CELSIUS = 0,
    FAHRENHEIT = 1,
    KELVIN = 2
};



// Sensor data structure
struct SensorData {
    float value;
    SensorType type;
    uint32_t timestamp;
    DeviceStatus status;
    char friendlyName[MAX_FRIENDLY_NAME_LENGTH];
};

// System status structure
struct SystemStatus {
    DeviceStatus deviceStatus;
    bool networkConnected;
    bool mqttConnected;
    uint32_t uptime;
    uint32_t lastError;
    DisplayMode displayMode;
};