#pragma once

#include <FastLED.h>
#include "config.h"
#include "chain_controller.h"
#include "protocol_handler.h"

class ArcadeController {
    // LED-Array VOR ChainController deklarieren (Initialisierungsreihenfolge)
    CRGB _ledsA[LEDS_PER_CHAIN];

    ChainController _chainA;
    ProtocolHandler _protocol;

    static Effect buildAttractEffect();

public:
    ArcadeController();

    void initialize();
    void loop();
};
