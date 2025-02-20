#include "OneWireTask.h"
#include "Logger.h"
#include "esp_task_wdt.h"
#include "NetworkTask.h"
#include "ControlTask.h"

// Static member initialization
OneWireManager OneWireTask::manager(ONE_WIRE_BUS);
QueueHandle_t OneWireTask::commandQueue = nullptr;
SemaphoreHandle_t OneWireTask::dataMutex = nullptr;

void OneWireTask::init() {
    Logger::info("Initializing OneWire task");
    
    // Initialize watchdog
    ESP_ERROR_CHECK(esp_task_wdt_init(CONFIG_ESP_TASK_WDT_TIMEOUT_S, true));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    
    // Create command queue and mutex
    commandQueue = xQueueCreate(10, sizeof(TaskMessage));
    dataMutex = xSemaphoreCreateMutex();
    
    if (!commandQueue || !dataMutex) {
        Logger::error("Failed to create OneWire task queues or mutex");
        return;
    }
    
    Logger::info("OneWire task initialized successfully");
}

void OneWireTask::start() {
    Logger::info("Starting OneWire task");
    
    xTaskCreate(
        taskFunction,
        "OneWireTask",
        ONEWIRE_TASK_STACK_SIZE,
        nullptr,
        ONEWIRE_TASK_PRIORITY,
        nullptr
    );
}

void OneWireTask::taskFunction(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t lastScanTime = 0;
    uint32_t lastReadTime = 0;
    bool conversionStarted = false;
    
    // Initial scan
    Logger::info("Performing initial OneWire bus scan");
    if (manager.scanDevices()) {
        lastScanTime = millis();
        Logger::info("Initial scan completed successfully");
    }
    
    while (true) {
        esp_task_wdt_reset();
        
        // Process commands
        TaskMessage msg;
        while (xQueueReceive(commandQueue, &msg, 0) == pdTRUE) {
            processCommand(msg);
        }
        
        uint32_t currentTime = millis();
        
        // Periodic scan
        if (currentTime - lastScanTime >= SCAN_INTERVAL) {
            if (!manager.isBusBusy() && !conversionStarted) {
                if (manager.scanDevices()) {
                    lastScanTime = currentTime;
                }
            }
        }
        
        // Temperature reading state machine
        if (!conversionStarted) {
            if (currentTime - lastReadTime >= READ_INTERVAL) {
                if (!manager.isBusBusy()) {
                    manager.startTemperatureConversion();
                    conversionStarted = true;
                }
            }
        } else {
            if (manager.checkAndCollectTemperatures()) {
                lastReadTime = currentTime;
                conversionStarted = false;
                
                // Trigger publication of new temperature data
                const auto& sensors = manager.getSensorList();
                for (const auto& sensor : sensors) {
                    if (sensor.valid) {
                        TaskMessage pubMsg;
                        pubMsg.type = MessageType::SENSOR_DATA;
                        pubMsg.data.sensorData = sensor;  // Updated field name
                        
                        NetworkTask::enqueuePublication(pubMsg);
                    }
                }
            }
        }
        
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(TASK_INTERVAL));
    }
}

void OneWireTask::processCommand(const TaskMessage& msg) {
    switch (msg.type) {
        case MessageType::SENSOR_SCAN_REQUEST:
            Logger::info("Processing scan request");
            if (!manager.isBusBusy()) {
                manager.scanDevices();
            } else {
                Logger::warning("Scan request ignored - bus busy");
            }
            break;
            
        case MessageType::TEMPERATURE_READ_REQUEST:
            Logger::info("Processing temperature read request");
            if (!manager.isBusBusy() && !manager.isConversionInProgress()) {
                manager.startTemperatureConversion();
            } else {
                Logger::warning("Read request ignored - operation in progress");
            }
            break;
            
        default:
            Logger::warning("Unknown command received");
            break;
    }
}

void OneWireTask::sendCommand(const TaskMessage& msg) {
    if (commandQueue) {
        if (xQueueSend(commandQueue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
            Logger::error("Failed to send command to OneWire task");
        }
    } else {
        Logger::error("Command queue not initialized");
    }
}