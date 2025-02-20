#pragma once

#include <Arduino.h>
#include "Logger.h"
#include <time.h>
#include <esp_sntp.h>    // Updated to use newer ESP-IDF SNTP API

class NtpManager {
public:
    static void init();
    static bool waitForSync(uint32_t timeout_ms);
    static void setTimeZone(const char* tz);
    static void updateNTPServer(const char* server);

private:
    static void ntpCallback(struct timeval *tv);
    
    static bool timeIsSynced;
    static SemaphoreHandle_t syncSemaphore;
    static const char* currentTimezone;
    static const char* ntpServer;
};