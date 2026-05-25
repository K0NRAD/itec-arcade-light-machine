#include "serial_parser.h"
#include "effects.h"

#include <Arduino.h>
#include <ArduinoJson.h>

static EffectType parseEffectType(const char* name) {
    if (strcmp(name, "fill")    == 0) return EFFECT_FILL;
    if (strcmp(name, "blink")   == 0) return EFFECT_BLINK;
    if (strcmp(name, "chase")   == 0) return EFFECT_CHASE;
    if (strcmp(name, "pulse")   == 0) return EFFECT_PULSE;
    if (strcmp(name, "rainbow") == 0) return EFFECT_RAINBOW;
    if (strcmp(name, "sparkle") == 0) return EFFECT_SPARKLE;
    if (strcmp(name, "wipe")    == 0) return EFFECT_WIPE;
    if (strcmp(name, "off")     == 0) return EFFECT_OFF;
    if (strcmp(name, "scanner") == 0) return EFFECT_SCANNER;
    return static_cast<EffectType>(255);
}

static Priority parsePriority(uint8_t val) {
    if (val >= 3) return PRIO_HIGH;
    if (val >= 2) return PRIO_MEDIUM;
    return PRIO_LOW;
}

static bool buildBaseEffect(const JsonDocument& doc, Effect& out, ErrorCallback onError) {
    const char* typeStr = doc["type"] | "";
    EffectType type = parseEffectType(typeStr);
    if (type == static_cast<EffectType>(255)) {
        onError(3, "unknown effect type");
        return false;
    }

    uint16_t speed = doc["speed"] | 100;
    if (speed == 0) {
        onError(4, "speed must not be zero");
        return false;
    }

    out.type      = type;
    out.color     = CRGB(
        (uint8_t)(doc["color"]["r"] | 0),
        (uint8_t)(doc["color"]["g"] | 0),
        (uint8_t)(doc["color"]["b"] | 0)
    );
    out.speed    = speed;
    out.length   = doc["length"]   | 5;
    out.repeat   = doc["repeat"]   | -1;
    out.priority = parsePriority(doc["priority"] | 1);
    return true;
}

static void applySegment(
    ChainController& chainA,
    Effect base,
    uint8_t segId,
    int8_t dir,
    ErrorCallback onError
) {
    if (segId > 5 && segId != 99) {
        onError(2, "unknown segment");
        return;
    }
    base.segmentId = segId;
    base.direction = dir;
    chainA.applyEffect(base);
}

static void handleEffectCommand(
    const JsonDocument& doc,
    ChainController& chainA,
    ErrorCallback onError
) {
    Effect base;
    if (!buildBaseEffect(doc, base, onError)) return;

    // "segment" kann eine Ganzzahl oder ein Array sein
    if (doc["segment"].is<JsonArrayConst>()) {
        JsonArrayConst segs = doc["segment"].as<JsonArrayConst>();
        bool dirIsArray = doc["dir"].is<JsonArrayConst>();
        JsonArrayConst dirs;
        if (dirIsArray) dirs = doc["dir"].as<JsonArrayConst>();
        int8_t singleDir = (int8_t)(doc["dir"] | 1);

        for (size_t i = 0; i < segs.size(); i++) {
            uint8_t segId = segs[i] | 99;
            int8_t  dir   = dirIsArray ? (int8_t)(dirs[i] | 1) : singleDir;
            applySegment(chainA, base, segId, dir, onError);
        }
    } else {
        uint8_t segId = doc["segment"] | 99;
        int8_t  dir   = (int8_t)(doc["dir"] | 1);
        applySegment(chainA, base, segId, dir, onError);
    }
}

static void handleAttractCommand(
    const JsonDocument& doc,
    ChainController& chainA,
    ErrorCallback onError
) {
    const char* state = doc["state"] | "";

    if (strcmp(state, "pause") == 0) {
        chainA.pauseAttract();
    } else if (strcmp(state, "resume") == 0) {
        chainA.resumeAttract();
    } else {
        onError(4, "unknown attract state");
    }
}

void processCommand(
    const char*      json,
    ChainController& chainA,
    ErrorCallback    onError
) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);

    if (err) {
        onError(1, "json parse error");
        return;
    }

    const char* cmd = doc["cmd"] | "";

    if (strcmp(cmd, "effect") == 0) {
        handleEffectCommand(doc, chainA, onError);
    } else if (strcmp(cmd, "attract") == 0) {
        handleAttractCommand(doc, chainA, onError);
    } else {
        onError(5, "unknown command");
    }
}
