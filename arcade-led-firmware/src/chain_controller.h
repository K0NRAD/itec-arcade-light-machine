#pragma once

#include <FastLED.h>
#include "effects.h"
#include "segments.h"

using SegmentResolver = const Segment& (*)(uint8_t id);

class ChainController {
public:
    ChainController(CRGB* leds, uint16_t numLeds, SegmentResolver resolver);

    void initialize(const Effect& attractEffect);
    void applyEffect(const Effect& effect);
    void pauseAttract();
    void resumeAttract();
    void update();

private:
    CRGB*           _leds;
    uint16_t        _numLeds;
    SegmentResolver _resolver;
    Effect          _activeEffect;
    Effect          _attractEffect;
    bool            _attractPaused;
};
