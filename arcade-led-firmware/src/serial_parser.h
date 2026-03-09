#pragma once

#include "chain_controller.h"

using ErrorCallback = void (*)(uint8_t code, const char* msg);

void processCommand(
    const char*      json,
    ChainController& chainA,
    ChainController& chainB,
    ErrorCallback    onError
);
