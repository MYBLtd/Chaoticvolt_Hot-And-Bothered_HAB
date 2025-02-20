#pragma once

#include <Arduino.h>
#include <vector>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "ForwardDeclarations.h"
#include "Config.h"
#include "SystemTypes.h"
#include "Logger.h"
#include "Config.h"

class OneWireManager {
public:
    explicit OneWireManager(uint8_t pin);
    
    void startTemperatureConversion();
    bool checkAndCollectTemperatures();
    bool scanDevices();
    void updateSensorList(const std::vector<TemperatureSensor>& newList);
    
    float getCachedTemperature(const uint8_t* address);
    String addressToString(const uint8_t* address) const;
    const std::vector<TemperatureSensor>& getSensorList() const;
    
    bool shouldScan() const;
    bool shouldRead() const;
    bool isConversionInProgress() const;
    bool isBusBusy() const;
    
private:
    OneWire oneWire;
    DallasTemperature sensors;
    std::vector<TemperatureSensor> sensorList;
    bool busyFlag;
    mutable SemaphoreHandle_t sensorMutex;
    
    uint32_t lastScanTime;
    uint32_t lastReadTime;
    uint32_t conversionStartTime;
    bool conversionInProgress;
    
    bool verifyMutex() const;
    void setBusBusy(bool busy);
    bool processFoundDevices(uint8_t deviceCount, std::vector<TemperatureSensor>& tempList);
};