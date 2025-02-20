// NetworkTask.h
#pragma once

#include <Arduino.h>
#include "MqttManager.h"

class NetworkTask {
public:
    static void init();
    static void start();
    static bool enqueuePublication(const TaskMessage& msg);
    
private:
    static void taskFunction(void* parameter);
    static MqttManager mqttManager;
    static QueueHandle_t publishQueue;
    static QueueHandle_t controlQueue;
};
