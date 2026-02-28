#include "serial_parser.h"
#include "effects.h"

#include <Arduino.h>
#include <ArduinoJson.h>

// Globale Effekt-Zustände aus main.cpp
extern Effect activeA;
extern Effect activeB;
extern Effect attractA;
extern Effect attractB;
extern bool   attractPaused;

// Fehlerausgabe aus main.cpp
extern void sendError(uint8_t code, const char* msg);

// Effekttyp aus JSON-String auflösen — unbekannte Namen liefern 255
static EffectType parseEffectType(const char* name) {
    if (strcmp(name, "fill")    == 0) return EFFECT_FILL;
    if (strcmp(name, "blink")   == 0) return EFFECT_BLINK;
    if (strcmp(name, "chase")   == 0) return EFFECT_CHASE;
    if (strcmp(name, "pulse")   == 0) return EFFECT_PULSE;
    if (strcmp(name, "rainbow") == 0) return EFFECT_RAINBOW;
    if (strcmp(name, "sparkle") == 0) return EFFECT_SPARKLE;
    if (strcmp(name, "wipe")    == 0) return EFFECT_WIPE;
    if (strcmp(name, "off")     == 0) return EFFECT_OFF;
    return static_cast<EffectType>(255);
}

static Priority parsePriority(uint8_t val) {
    if (val >= 3) return PRIO_HIGH;
    if (val >= 2) return PRIO_MEDIUM;
    return PRIO_LOW;
}

static void handleEffectCommand(const JsonDocument& doc) {
    const char* chainStr = doc["chain"] | "A";
    const char* typeStr  = doc["type"]  | "";

    EffectType type = parseEffectType(typeStr);
    if (type == static_cast<EffectType>(255)) {
        sendError(3, "unknown effect type");
        return;
    }

    uint8_t segId = doc["segment"] | 99;
    if (segId > 5 && segId != 99) {
        sendError(2, "unknown segment");
        return;
    }

    uint16_t speed = doc["speed"] | 100;
    if (speed == 0) {
        sendError(4, "speed must not be zero");
        return;
    }

    Effect newEffect;
    newEffect.type      = type;
    newEffect.segmentId = segId;
    newEffect.color     = CRGB(
        (uint8_t)(doc["color"]["r"] | 255),
        (uint8_t)(doc["color"]["g"] | 255),
        (uint8_t)(doc["color"]["b"] | 255)
    );
    newEffect.speed    = speed;
    newEffect.length   = doc["length"]   | 5;
    newEffect.repeat   = doc["repeat"]   | -1;
    newEffect.priority = parsePriority(doc["priority"] | 1);

    bool isChainA = (chainStr[0] == 'A' || chainStr[0] == 'a');
    if (isChainA) {
        applyEffect(activeA, newEffect);
    } else {
        applyEffect(activeB, newEffect);
    }
}

static void handleAttractCommand(const JsonDocument& doc) {
    const char* state = doc["state"] | "";

    if (strcmp(state, "pause") == 0) {
        attractPaused = true;
    } else if (strcmp(state, "resume") == 0) {
        attractPaused = false;
        activeA = attractA;
        activeB = attractB;
    } else {
        sendError(4, "unknown attract state");
    }
}

void processCommand(const char* json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);

    if (err) {
        sendError(1, "json parse error");
        return;
    }

    const char* cmd = doc["cmd"] | "";

    if (strcmp(cmd, "effect") == 0) {
        handleEffectCommand(doc);
    } else if (strcmp(cmd, "attract") == 0) {
        handleAttractCommand(doc);
    } else {
        sendError(5, "unknown command");
    }
}
