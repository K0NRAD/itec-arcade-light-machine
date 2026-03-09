#pragma once

#include <FastLED.h>
#include "config.h"
#include "chain_controller.h"
#include "protocol_handler.h"

class ArcadeController {
    // LED-Arrays VOR ChainController deklarieren (Initialisierungsreihenfolge)
    CRGB _ledsA[LEDS_PER_CHAIN];
    CRGB _ledsB[LEDS_PER_CHAIN];

    ChainController _chainA;
    ChainController _chainB;
    ProtocolHandler _protocol;

    static Effect buildAttractEffectA();
    static Effect buildAttractEffectB();

public:
    ArcadeController();

    void initialize();
    void loop();
};
