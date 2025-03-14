// src/ControlTask.cpp
#include "ControlTask.h"
#include "PreferencesManager.h"
#include "OneWireTask.h"
#include "NetworkTask.h"
#include <cstring>
#include <cstddef>
#include <Arduino.h>

// Static member initializations
DisplayManager ControlTask::display(DISPLAY_CLK, DISPLAY_DIO);
RelayState ControlTask::relayStates[2] = {{false, false, 0}, {false, false, 0}};
QueueHandle_t ControlTask::controlQueue = nullptr;
SemaphoreHandle_t ControlTask::stateMutex = nullptr;

void ControlTask::init() {
    Logger::info("Starting ControlTask initialization");
    
    // Create control queue
    controlQueue = xQueueCreate(10, sizeof(TaskMessage));
    if (!controlQueue) {
        Logger::error("Failed to create control queue");
        return;
    }
    Logger::info("Control queue created");
    
    // Create mutex
    stateMutex = xSemaphoreCreateMutex();
    if (!stateMutex) {
        Logger::error("Failed to create state mutex");
        return;
    }
    Logger::info("State mutex created");
    
    // Configure relay pins
    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);
    digitalWrite(RELAY_1_PIN, LOW);
    digitalWrite(RELAY_2_PIN, LOW);
    Logger::info("Relay pins configured");

    Logger::info("Initializing display on CLK=" + String(DISPLAY_CLK) + 
                 " DIO=" + String(DISPLAY_DIO));
                 
    display.init();  // Initialize the display
    
    Logger::info("ControlTask initialization complete");
}

void ControlTask::start() {
    Logger::info("Starting ControlTask creation");
    
    TaskHandle_t taskHandle;
    BaseType_t result = xTaskCreate(
        taskFunction,
        "ControlTask",
        CONTROL_TASK_STACK_SIZE,
        nullptr,
        CONTROL_TASK_PRIORITY,
        &taskHandle
    );
    
    if (result != pdPASS) {
        Logger::error("Failed to create ControlTask - error code: " + String(result));
        return;
    }
    
    if (taskHandle == nullptr) {
        Logger::error("Task handle is null after creation");
        return;
    }
    
    Logger::info("ControlTask successfully created on core " + 
                 String(xPortGetCoreID()) + 
                 " with priority " + 
                 String(uxTaskPriorityGet(taskHandle)));
}


void ControlTask::taskFunction(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint8_t displaySensorAddr[8] = {0};
    uint8_t currentSensorAddr[8] = {0};  // Keep track of current selection
    float lastPublishedTemp = -999.0f;  // Track last published temperature
    uint32_t lastPublishAttempt = 0;
    const uint32_t PUBLISH_RETRY_INTERVAL = 5000; // 5 seconds between retries
    
    Logger::info("Control task starting");
    
    while (true) {
        // Handle relay control messages
        TaskMessage msg;
        while (xQueueReceive(controlQueue, &msg, 0) == pdTRUE) {
            if (msg.type == MessageType::RELAY_CHANGE_REQUEST) {
                uint8_t relayId = msg.data.relayChange.relayId;
                bool newState = msg.data.relayChange.state;
                
                if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    relayStates[relayId].requested = newState;
                    xSemaphoreGive(stateMutex);
                }
            }
        }
        
        // Update physical relay states if needed
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < 2; i++) {
                if (relayStates[i].requested != relayStates[i].actual) {
                    digitalWrite(i == 0 ? RELAY_1_PIN : RELAY_2_PIN, 
                               relayStates[i].requested ? HIGH : LOW);
                    
                    relayStates[i].actual = relayStates[i].requested;
                    relayStates[i].lastChangeTime = millis();
                    
                    Logger::info("Relay " + String(i) + " state changed to " + 
                               String(relayStates[i].actual ? "ON" : "OFF"));
                }
            }
            xSemaphoreGive(stateMutex);
        }
        
        // Get current preferences
        PreferencesManager::getDisplaySensor(displaySensorAddr);
        
        // Check if sensor selection changed
        if (memcmp(currentSensorAddr, displaySensorAddr, 8) != 0) {
            Logger::info("Display sensor selection changed");
            String newAddr;
            for (int i = 0; i < 8; i++) {
                if (i > 0) newAddr += ":";
                newAddr += String(displaySensorAddr[i], HEX);
            }
            Logger::info("New sensor address: " + newAddr);
            
            // Update current selection
            memcpy(currentSensorAddr, displaySensorAddr, 8);
            
            // Show brief message indicating change
            display.showMessage("CHG");
            delay(500);
            
            // Reset last published temperature to force new publish
            lastPublishedTemp = -999.0f;
        }
        
        // Get sensor list
        const auto& sensors = OneWireTask::getManager().getSensorList();

        // Find the selected sensor
        bool sensorFound = false;
        for (const auto& sensor : sensors) {
            if (memcmp(sensor.address, currentSensorAddr, 8) == 0) {
                sensorFound = true;
                if (sensor.valid) {
                    float currentTemp = sensor.temperature;
                    display.setTemperature(currentTemp);
                } else {
                    display.showMessage("ERR");
                    Logger::warning("Selected sensor reading invalid");
                    
                break;
            }
        }
        
        if (!sensorFound) {
            bool isEmpty = true;
            for (int i = 0; i < 8; i++) {
                if (currentSensorAddr[i] != 0) {
                    isEmpty = false;
                    break;
                }
            }
            
            if (isEmpty && !sensors.empty()) {
                // Auto-select first sensor if none configured
                PreferencesManager::setDisplaySensor(sensors[0].address);
                memcpy(currentSensorAddr, sensors[0].address, 8);
                display.showMessage("AUTO");
                delay(500);
            } else {
                display.showMessage("LOST");
                }
            }
        } 
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL));
    }
}

void ControlTask::updateRelayRequest(uint8_t relayId, bool state) {
    if (relayId >= 2) {
        Logger::error("Invalid relay ID: " + String(relayId));
        return;
    }
    
    if (!controlQueue) {
        Logger::error("Control queue not initialized");
        return;
    }
    
    TaskMessage msg;
    msg.type = MessageType::RELAY_CHANGE_REQUEST;
    msg.data.relayChange.relayId = relayId;
    msg.data.relayChange.state = state;
    
    // Send message to control queue with timeout
    if (xQueueSend(controlQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        Logger::error("Failed to send relay control message to queue");
    } else {
        Logger::info("Relay " + String(relayId) + " state change requested to " + 
                    String(state ? "ON" : "OFF"));
    }
}

bool ControlTask::getRelayState(uint8_t relayId) {
    if (relayId >= 2) {
        Logger::error("Invalid relay ID in getRelayState: " + String(relayId));
        return false;
    }
    
    if (!stateMutex) {
        Logger::error("State mutex not initialized in getRelayState");
        return false;
    }
    
    bool state = false;
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = relayStates[relayId].actual;
        xSemaphoreGive(stateMutex);
    } else {
        Logger::error("Failed to acquire mutex in getRelayState");
    }
    
    return state;
}

