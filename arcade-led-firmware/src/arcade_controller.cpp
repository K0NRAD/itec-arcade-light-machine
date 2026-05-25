#include "arcade_controller.h"
#include "segments.h"

ArcadeController::ArcadeController()
    : _chainA(_ledsA, LEDS_PER_CHAIN, &getSegmentA)
{}

Effect ArcadeController::buildAttractEffect() {
    Effect attract;
    attract.type      = EFFECT_PULSE;
    attract.segmentId = 99;
    attract.color     = CRGB(0, 100, 200);
    attract.speed     = 30;    // 30 BPM ≈ 2-Sekunden-Atemrhythmus
    attract.repeat    = -1;
    attract.priority  = PRIO_LOW;
    return attract;
}

void ArcadeController::initialize() {
    Serial.begin(SERIAL_BAUD);

    FastLED.addLeds<LED_TYPE, PIN_CHAIN_A, COLOR_ORDER>(_ledsA, LEDS_PER_CHAIN);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear(true);

    _chainA.initialize(buildAttractEffect());

    _protocol.sendHeartbeat();
}

void ArcadeController::loop() {
    _protocol.readAndProcess(_chainA);
    _chainA.update();
    FastLED.show();
}
