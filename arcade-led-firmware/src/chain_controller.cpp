#include "chain_controller.h"

ChainController::ChainController(CRGB* leds, uint16_t numLeds, SegmentResolver resolver)
    : _leds(leds)
    , _numLeds(numLeds)
    , _resolver(resolver)
    , _attractPaused(false)
{}

void ChainController::initialize(const Effect& attractEffect) {
    _attractEffect = attractEffect;
    for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
        _effects[i]           = attractEffect;
        _effects[i].segmentId = i;
    }
}

void ChainController::applyEffect(const Effect& effect) {
    if (effect.segmentId == 99) {
        for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
            Effect fx = effect;
            fx.segmentId = i;
            // Laufzeit-Zustand zurücksetzen damit alle Segmente synchron starten
            fx.step        = 0;
            fx.repeatCount = 0;
            fx.finished    = false;
            fx.lastUpdate  = 0;
            ::applyEffect(_effects[i], fx);
        }
    } else if (effect.segmentId < SEGMENT_COUNT) {
        ::applyEffect(_effects[effect.segmentId], effect);
    }
}

void ChainController::pauseAttract() {
    _attractPaused = true;
}

void ChainController::resumeAttract() {
    _attractPaused = false;
    for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
        _effects[i]           = _attractEffect;
        _effects[i].segmentId = i;
    }
}

void ChainController::update() {
    for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
        Effect& fx = _effects[i];
        if (fx.finished && !_attractPaused) {
            fx           = _attractEffect;
            fx.segmentId = i;
        }
        ::updateEffect(fx, _leds, _numLeds, _resolver(i));
    }
}
