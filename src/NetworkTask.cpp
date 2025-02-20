// src/NetworkTask.cpp
#include "NetworkTask.h"
#include "OneWireTask.h"
#include "ControlTask.h"
#include "Logger.h"
#include "SystemHealth.h"
#include "NtpManager.h"
#include "MDNSManager.h"
#include <ESPmDNS.h>

// Static member initializations
MqttManager NetworkTask::mqttManager;
QueueHandle_t NetworkTask::publishQueue = nullptr;
QueueHandle_t NetworkTask::controlQueue = nullptr;

void NetworkTask::init() {
    Logger::info("Starting Network task initialization");
    
    publishQueue = xQueueCreate(20, sizeof(TaskMessage));
    controlQueue = xQueueCreate(10, sizeof(TaskMessage));
    
    if (!publishQueue || !controlQueue) {
        Logger::error("Failed to create queues");
        return;
    }
    Logger::info("Network queues created");

    // Try initial MDNS setup
    if (!MDNSManager::init()) {
        Logger::warning("Initial mDNS setup deferred - will retry in network task");
    }

    // Initialize NTP before MQTT
    NtpManager::init();
    Logger::info("NTP Manager initialized");
    
    Logger::info("Network initialization complete");
}

bool NetworkTask::enqueuePublication(const TaskMessage& msg) {
    if (!publishQueue) return false;
    return xQueueSend(publishQueue, &msg, 0) == pdTRUE;
}

void NetworkTask::start() {
    xTaskCreate(
        taskFunction,
        "NetworkTask",
        NETWORK_TASK_STACK_SIZE,
        nullptr,
        NETWORK_TASK_PRIORITY,
        nullptr
    );
}

void NetworkTask::taskFunction(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t connectionCheckInterval = pdMS_TO_TICKS(5000);
    const TickType_t publishInterval = pdMS_TO_TICKS(30000);  // 30 second data publish
    const uint32_t hadPublishInterval = 300000;  // 5 minutes HAD publish
    uint32_t lastPublishTime = 0;
    uint32_t lastHADPublishTime = 0;
    bool mqttInitialized = false;
    
    Logger::info("Network task starting");
    
    while (true) {
        // Handle mDNS
        if (!MDNSManager::isInitialized()) {
            MDNSManager::init();
        } else {
            MDNSManager::update();
        }

        // Initialize MQTT only after NTP sync
        if (!mqttInitialized && ETH.linkUp()) {
            if (NtpManager::waitForSync(5000)) {
                Logger::info("NTP synced - initializing MQTT");
                mqttManager.begin();
                mqttInitialized = true;
                Logger::info("MQTT Manager initialized");
            } else {
                Logger::warning("Waiting for NTP sync before MQTT init");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }
        
        // Maintain MQTT connection
        if (mqttInitialized) {
            mqttManager.maintainConnection();
            
            // Handle regular publications when connected
            if (mqttManager.isConnected()) {
                uint32_t now = millis();
                
                // Process any pending publish messages
                TaskMessage msg;
                while (xQueueReceive(publishQueue, &msg, 0) == pdTRUE) {
                    if (msg.type == MessageType::SENSOR_DATA) {
                        mqttManager.publishSensorData(msg.data.sensorData);
                    } else if (msg.type == MessageType::RELAY_STATE) {
                        mqttManager.publishRelayState(msg.data.relayState.id, 
                                                    msg.data.relayState.state);
                    }
                }
                
                // Regular sensor data publication
                if (now - lastPublishTime >= 30000) {
                    mqttManager.startBatchPublish();
                    
                    // Publish all sensor data
                    const auto& sensors = OneWireTask::getManager().getSensorList();
                    for (const auto& sensor : sensors) {
                        mqttManager.publishSensorData(sensor);
                    }
                    
                    // Publish relay states
                    for (uint8_t i = 0; i < 2; i++) {
                        bool state = ControlTask::getRelayState(i);
                        mqttManager.publishRelayState(i, state);
                    }
                    
                    mqttManager.endBatchPublish();
                    lastPublishTime = now;
                }
                
                // Periodic HAD metadata publication
                if (now - lastHADPublishTime >= hadPublishInterval) {
                    Logger::info("Publishing HAD metadata");
                    mqttManager.startBatchPublish();
                    
                    // Publish metadata for all sensors
                    const auto& sensors = OneWireTask::getManager().getSensorList();
                    for (const auto& sensor : sensors) {
                        mqttManager.publishSensorMetadata(sensor);
                    }
                    
                    // Publish relay metadata
                    mqttManager.publishRelayMetadata();
                    
                    mqttManager.endBatchPublish();
                    lastHADPublishTime = now;
                    Logger::info("HAD metadata publication complete");
                }
            }
        }

        vTaskDelayUntil(&lastWakeTime, connectionCheckInterval);
    }
}

