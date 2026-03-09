#include "arcade_controller.h"
#include "segments.h"

ArcadeController::ArcadeController()
    : _chainA(_ledsA, LEDS_PER_CHAIN, &getSegmentA)
    , _chainB(_ledsB, LEDS_PER_CHAIN, &getSegmentB)
{}

Effect ArcadeController::buildAttractEffectA() {
    Effect attractA;
    attractA.type      = EFFECT_PULSE;
    attractA.segmentId = 99;
    attractA.color     = CRGB(0, 100, 200);
    attractA.speed     = 30;    // 30 BPM ≈ 2-Sekunden-Atemrhythmus
    attractA.repeat    = -1;
    attractA.priority  = PRIO_LOW;
    return attractA;
}

Effect ArcadeController::buildAttractEffectB() {
    Effect attractB;
    attractB.type      = EFFECT_RAINBOW;
    attractB.segmentId = 99;
    attractB.speed     = 20;    // 20ms pro Farbschritt
    attractB.repeat    = -1;
    attractB.priority  = PRIO_LOW;
    return attractB;
}

void ArcadeController::initialize() {
    Serial.begin(SERIAL_BAUD);

    FastLED.addLeds<LED_TYPE, PIN_CHAIN_A, COLOR_ORDER>(_ledsA, LEDS_PER_CHAIN);
    FastLED.addLeds<LED_TYPE, PIN_CHAIN_B, COLOR_ORDER>(_ledsB, LEDS_PER_CHAIN);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear(true);

    _chainA.initialize(buildAttractEffectA());
    _chainB.initialize(buildAttractEffectB());

    _protocol.sendHeartbeat();
}

void ArcadeController::loop() {
    _protocol.readAndProcess(_chainA, _chainB);
    _chainA.update();
    _chainB.update();
    FastLED.show();
}
