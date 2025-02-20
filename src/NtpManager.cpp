// src/NtpManager.cpp
#include "NtpManager.h"

// Static member definitions
bool NtpManager::timeIsSynced = false;
SemaphoreHandle_t NtpManager::syncSemaphore = nullptr;
const char* NtpManager::currentTimezone = "UTC0";
const char* NtpManager::ntpServer = "pool.ntp.org";

void NtpManager::init() {
    Logger::info("Initializing NTP Manager");
    
    if (!syncSemaphore) {
        syncSemaphore = xSemaphoreCreateBinary();
        if (!syncSemaphore) {
            Logger::error("Failed to create NTP sync semaphore");
            return;
        }
    }
    
    // Configure NTP
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setservername(0, (char*)ntpServer);
    sntp_set_time_sync_notification_cb(ntpCallback);
    
    // Set timezone
    setenv("TZ", currentTimezone, 1);
    tzset();
    
    // Start NTP
    sntp_init();
    Logger::info("NTP initialization complete");
}

bool NtpManager::waitForSync(uint32_t timeout_ms) {
    if (timeIsSynced) {
        return true;
    }
    
    Logger::info("Waiting for NTP sync...");
    
    TickType_t xTicksToWait = pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(syncSemaphore, xTicksToWait) == pdTRUE) {
        timeIsSynced = true;
        
        // Log current time
        time_t now;
        time(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        Logger::info("NTP sync complete. Current time: " + String(timeStr));
        
        return true;
    }
    
    Logger::error("NTP sync timeout");
    return false;
}

void NtpManager::setTimeZone(const char* tz) {
    currentTimezone = tz;
    setenv("TZ", tz, 1);
    tzset();
    
    if (timeIsSynced) {
        time_t now;
        time(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        Logger::info("Timezone updated. Current time: " + String(timeStr));
    }
}

void NtpManager::updateNTPServer(const char* server) {
    ntpServer = server;
    if (sntp_enabled()) {
        sntp_stop();
        sntp_setservername(0, (char*)server);
        sntp_init();
        Logger::info("NTP server updated to: " + String(server));
    }
}

void NtpManager::ntpCallback(struct timeval *tv) {
    timeIsSynced = true;
    if (syncSemaphore) {
        xSemaphoreGive(syncSemaphore);
    }
    Logger::info("NTP sync callback received");
}