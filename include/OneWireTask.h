#pragma once

#include "OneWireManager.h"
#include "SystemTypes.h"
#include "Config.h"
#include <queue>

class OneWireTask {
public:
    static void init();
    static void start();
    
    // Public interface for task communication
    static void sendCommand(const TaskMessage& msg);
    static OneWireManager& getManager() { return manager; }

private:
    static void taskFunction(void* parameter);
    static void processCommand(const TaskMessage& msg);
    
    // Static members
    static OneWireManager manager;
    static QueueHandle_t commandQueue;
    static SemaphoreHandle_t dataMutex;
    
    // Constants
    static constexpr uint32_t TASK_INTERVAL = 100;    // Base task interval in ms
    static constexpr uint32_t READ_INTERVAL = 1000;   // Temperature read interval
    static constexpr uint32_t SCAN_INTERVAL = 30000;  // Device scan interval
    
    // Delete copy constructor and assignment operator
    OneWireTask(const OneWireTask&) = delete;
    OneWireTask& operator=(const OneWireTask&) = delete;
};