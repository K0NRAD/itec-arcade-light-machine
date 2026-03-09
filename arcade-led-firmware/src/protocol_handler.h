#pragma once

#include <Arduino.h>
#include "config.h"
#include "chain_controller.h"

class ProtocolHandler {
public:
    ProtocolHandler();

    void sendHeartbeat() const;
    static void sendError(uint8_t code, const char* msg);
    void readAndProcess(ChainController& chainA, ChainController& chainB);

private:
    char     _serialBuffer[SERIAL_BUFFER_LEN];
    uint8_t  _bufferIndex;
    uint32_t _lastCharTime;
};
