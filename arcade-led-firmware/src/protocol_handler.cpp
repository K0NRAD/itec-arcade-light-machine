#include "protocol_handler.h"
#include "serial_parser.h"

ProtocolHandler::ProtocolHandler()
    : _bufferIndex(0)
    , _lastCharTime(0)
{
    _serialBuffer[0] = '\0';
}

void ProtocolHandler::sendHeartbeat() const {
    Serial.printf(
        "{\"status\":\"ready\",\"version\":\"%s\",\"leds_a\":%d,\"leds_b\":%d}\n",
        FW_VERSION, LEDS_PER_CHAIN, LEDS_PER_CHAIN
    );
}

void ProtocolHandler::sendError(uint8_t code, const char* msg) {
    Serial.printf(
        "{\"status\":\"error\",\"code\":%d,\"msg\":\"%s\"}\n",
        code, msg
    );
}

void ProtocolHandler::readAndProcess(ChainController& chainA, ChainController& chainB) {
    while (Serial.available()) {
        char c = Serial.read();
        _lastCharTime = millis();

        if (c == '\n' || c == '\r') {
            if (_bufferIndex > 0) {
                _serialBuffer[_bufferIndex] = '\0';
                processCommand(_serialBuffer, chainA, chainB, &ProtocolHandler::sendError);
                _bufferIndex = 0;
            }
        } else if (_bufferIndex < SERIAL_BUFFER_LEN - 1) {
            _serialBuffer[_bufferIndex++] = c;
        }
        // Überlange Zeile: überschüssige Bytes schweigend verwerfen
    }

    // Timeout: unvollständigen Befehl nach SERIAL_TIMEOUT_MS verwerfen
    if (_bufferIndex > 0 && millis() - _lastCharTime > SERIAL_TIMEOUT_MS) {
        sendError(1, "serial timeout");
        _bufferIndex = 0;
    }
}
