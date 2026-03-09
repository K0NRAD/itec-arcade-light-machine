#include "chain_controller.h"

ChainController::ChainController(CRGB* leds, uint16_t numLeds, SegmentResolver resolver)
    : _leds(leds)
    , _numLeds(numLeds)
    , _resolver(resolver)
    , _activeEffect()
    , _attractEffect()
    , _attractPaused(false)
{}

void ChainController::initialize(const Effect& attractEffect) {
    _attractEffect = attractEffect;
    _activeEffect  = attractEffect;
}

void ChainController::applyEffect(const Effect& effect) {
    ::applyEffect(_activeEffect, effect);
}

void ChainController::pauseAttract() {
    _attractPaused = true;
}

void ChainController::resumeAttract() {
    _attractPaused = false;
    _activeEffect  = _attractEffect;
}

void ChainController::update() {
    if (_activeEffect.finished && !_attractPaused) {
        _activeEffect = _attractEffect;
    }
    ::updateEffect(_activeEffect, _leds, _numLeds, _resolver(_activeEffect.segmentId));
}
