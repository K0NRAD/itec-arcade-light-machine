#pragma once
#include <FastLED.h>
#include "segments.h"

enum EffectType : uint8_t {
    EFFECT_OFF     = 0,
    EFFECT_FILL    = 1,
    EFFECT_BLINK   = 2,
    EFFECT_CHASE   = 3,
    EFFECT_PULSE   = 4,  // speed = BPM
    EFFECT_RAINBOW = 5,  // speed = ms pro Schritt
    EFFECT_SPARKLE = 6,  // speed = ms pro Frame
    EFFECT_WIPE    = 7   // speed = ms pro LED
};

enum Priority : uint8_t {
    PRIO_LOW    = 1,  // Attract-Mode, Idle
    PRIO_MEDIUM = 2,  // Treffer, Bonus, Pill
    PRIO_HIGH   = 3   // Game Over, Death, Level Complete
};

struct Effect {
    EffectType type      = EFFECT_OFF;
    uint8_t    segmentId = 99;        // 99 = gesamte Kette
    CRGB       color     = CRGB::Black;
    uint16_t   speed     = 100;
    uint8_t    length    = 5;         // Chase-Schweif-Länge in LEDs
    int16_t    repeat    = -1;        // -1 = endlos, n = n Wiederholungen

    Priority   priority  = PRIO_LOW;

    // Interner Laufzeit-Zustand — nicht über Protokoll setzen
    uint16_t   step        = 0;
    uint32_t   lastUpdate  = 0;
    int16_t    repeatCount = 0;
    bool       finished    = false;
};

// Effekt-Frame rendern — non-blocking via millis()
void updateEffect(Effect& fx, CRGB* ledArray, uint16_t numLeds, const Segment& seg);

// Neuen Effekt anwenden — prüft Priorität
// Gibt true zurück wenn der Effekt übernommen wurde
bool applyEffect(Effect& current, const Effect& incoming);
