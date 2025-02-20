// include/Config.h
#pragma once
#include <Arduino.h>
#include <stdint.h>

#define DEVICE_NAME "Chaoticvolt_SensorHUB"
#define FIRMWARE_VERSION "1.2.7"
#define DEVICE_MANUFACTURER "Chaoticvolt"
#define DEVICE_MODEL "SensorHUB"
#define DEVICE_VERSION "2.0.0"
// Network Configuration
#define MDNS_HOSTNAME "sensorhub"
//#define MDNS_SERVER_NAME "Temperature Monitor"

// MQTT Broker Configuration
#define SYSTEM_NAME "chaoticvolt"
#define DEVICE_ID "sensorhub1"

// MQTT Topic Structure
#define MQTT_BASE_TOPIC SYSTEM_NAME "/" DEVICE_ID           // chaoticvolt/sensorhub1
#define MQTT_SENSOR_BASE "sensor"                          // For temperature sensors
#define MQTT_SWITCH_BASE "switch"                          // For relay control
#define MQTT_STATE_TOPIC "status"                         // For availability
#define MQTT_AVAILABILITY_TOPIC "availability"            // For detailed status
#define MQTT_SET_TOPIC "set"                             // For commands
#define MQTT_HA_DISCOVERY_PREFIX "homeassistant"         // Home Assistant discovery prefix

// Connection timing
#define INITIAL_RETRY_DELAY 5000
#define MAX_RETRY_DELAY 300000
#define DNS_CACHE_TIME 300000  // 5 minutes

// Task configurations
#define NETWORK_TASK_STACK_SIZE 8192
#define NETWORK_TASK_PRIORITY 2
#define ONEWIRE_TASK_STACK_SIZE 4096
#define ONEWIRE_TASK_PRIORITY 2
#define CONTROL_TASK_STACK_SIZE 4096
#define CONTROL_TASK_PRIORITY 2

// Pin Configuration
constexpr uint8_t ONE_WIRE_BUS = 4;

// System Configuration
#define CREDENTIAL_RESET_PIN 15  // GPIO15 from UEXT
#define CREDENTIAL_RESET_TIME 10000  // 10 seconds hold time
constexpr size_t MAX_ONEWIRE_SENSORS = 16;
constexpr uint32_t WATCHDOG_TIMEOUT = 30000;  // 30 seconds
constexpr uint8_t MAX_RETRIES = 3;            // Maximum number of retry attempts

// Timing Intervals (ms)
constexpr uint32_t SCAN_INTERVAL = 30000;           // Scan for new sensors every 30 seconds
constexpr uint32_t READ_INTERVAL = 10000;           // Read temperatures every 10 seconds
constexpr uint32_t WEB_UPDATE_INTERVAL = 2000;      // Update web interface every 2 seconds
constexpr uint32_t MQTT_PUBLISH_INTERVAL = 5000;    // Update web interface every 2 seconds
constexpr uint32_t TASK_INTERVAL = 1000;            // Task loop interval 1 second
constexpr uint32_t DISPLAY_UPDATE_INTERVAL = 1000;

// System Requirements
constexpr size_t MINIMUM_REQUIRED_HEAP = 32768;

// MQTT Configuration
#ifndef MQTT_MAX_PACKET_SIZE
constexpr size_t MQTT_MAX_PACKET_SIZE = 512;
#endif

// System Configuration
#define MAX_FRIENDLY_NAME_LENGTH 32

// display pins
// Safe configuration avoiding all used peripherals
#define DISPLAY_DIO  14   // HS2_CLK - safe if not using SD
#define DISPLAY_CLK  17   // CS - safe if not using SPI

// pins for the relays
#define RELAY_1_PIN 32
#define RELAY_2_PIN 33

#define ETH_TYPE               ETH_PHY_LAN8720
#define ETH_ADDR               1
#define ETH_POWER_PIN          16
#define ETH_MDC_PIN           23
#define ETH_MDIO_PIN          18
#define ETH_PHY_ADDR           0
#define ETH_CLK_MODE          ETH_CLOCK_GPIO0_IN