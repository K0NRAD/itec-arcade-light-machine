#pragma once

#include <FastLED.h>
#include "effects.h"
#include "segments.h"

using SegmentResolver = const Segment& (*)(uint8_t id);

class ChainController {
public:
    static constexpr uint8_t SEGMENT_COUNT = 6; // Segmente 0–5

    ChainController(CRGB* leds, uint16_t numLeds, SegmentResolver resolver);

    void initialize(const Effect& attractEffect);

    // Effekt auf ein einzelnes Segment oder alle (segmentId=99) anwenden
    void applyEffect(const Effect& effect);

    void pauseAttract();
    void resumeAttract();
    void update();

private:
    CRGB*           _leds;
    uint16_t        _numLeds;
    SegmentResolver _resolver;
    Effect          _effects[SEGMENT_COUNT]; // Ein unabhängiger Effekt pro Segment
    Effect          _attractEffect;
    bool            _attractPaused;
};
