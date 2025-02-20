// include/RelayState.h
#pragma once

#include <cstdint>

struct RelayState {
    bool requested;
    bool actual;
    uint32_t lastChangeTime;
};