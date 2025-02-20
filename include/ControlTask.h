// include/ControlTask.h
#pragma once

#include <Arduino.h>
#include "DisplayManager.h"
#include "RelayState.h"
#include "SystemTypes.h"

class ControlTask {
public:
    static void init();
    static void start();
    static void updateRelayRequest(uint8_t relayId, bool state);
    static bool getRelayState(uint8_t relayId);

private:
    static DisplayManager display;
    static RelayState relayStates[2];
    static QueueHandle_t controlQueue;
    static SemaphoreHandle_t stateMutex;
    static void taskFunction(void* parameter);
};