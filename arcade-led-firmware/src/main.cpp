#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "effects.h"
#include "segments.h"
#include "serial_parser.h"

// --- LED-Arrays ---
CRGB ledsA[LEDS_PER_CHAIN];
CRGB ledsB[LEDS_PER_CHAIN];

// --- Effekt-Zustände (Kette A und B) ---
Effect activeA;
Effect activeB;
Effect attractA;   // Attract-Mode Vorlage für Kette A
Effect attractB;   // Attract-Mode Vorlage für Kette B
bool   attractPaused = false;

// --- Serial-Eingabe-Buffer ---
static char     serialBuffer[SERIAL_BUFFER_LEN];
static uint8_t  bufferIndex  = 0;
static uint32_t lastCharTime = 0;

// --- Protokoll-Ausgaben ---

void sendHeartbeat() {
    Serial.printf(
        "{\"status\":\"ready\",\"version\":\"%s\",\"leds_a\":%d,\"leds_b\":%d}\n",
        FW_VERSION, LEDS_PER_CHAIN, LEDS_PER_CHAIN
    );
}

void sendError(uint8_t code, const char* msg) {
    Serial.printf(
        "{\"status\":\"error\",\"code\":%d,\"msg\":\"%s\"}\n",
        code, msg
    );
}

// --- Attract-Mode Initialisierung ---

static void initAttractMode() {
    // Kette A: sanftes Pulsieren in Arcade-Blau
    attractA           = Effect{};
    attractA.type      = EFFECT_PULSE;
    attractA.segmentId = 99;
    attractA.color     = CRGB(0, 100, 200);
    attractA.speed     = 30;    // 30 BPM ≈ 2-Sekunden-Atemrhythmus
    attractA.repeat    = -1;
    attractA.priority  = PRIO_LOW;

    // Kette B: langsamer Regenbogen
    attractB           = Effect{};
    attractB.type      = EFFECT_RAINBOW;
    attractB.segmentId = 99;
    attractB.speed     = 20;    // 20ms pro Farbschritt
    attractB.repeat    = -1;
    attractB.priority  = PRIO_LOW;

    activeA = attractA;
    activeB = attractB;
}

// --- Serial-Lesen ---

static void readSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        lastCharTime = millis();

        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                serialBuffer[bufferIndex] = '\0';
                processCommand(serialBuffer);
                bufferIndex = 0;
            }
        } else if (bufferIndex < SERIAL_BUFFER_LEN - 1) {
            serialBuffer[bufferIndex++] = c;
        }
        // Überlange Zeile: überschüssige Bytes schweigend verwerfen
    }

    // Timeout: unvollständigen Befehl nach SERIAL_TIMEOUT_MS verwerfen
    if (bufferIndex > 0 && millis() - lastCharTime > SERIAL_TIMEOUT_MS) {
        sendError(1, "serial timeout");
        bufferIndex = 0;
    }
}

// --- Arduino Entry Points ---

void setup() {
    Serial.begin(SERIAL_BAUD);

    // Beide LED-Ketten registrieren
    FastLED.addLeds<LED_TYPE, PIN_CHAIN_A, COLOR_ORDER>(ledsA, LEDS_PER_CHAIN);
    FastLED.addLeds<LED_TYPE, PIN_CHAIN_B, COLOR_ORDER>(ledsB, LEDS_PER_CHAIN);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear(true);

    initAttractMode();
    sendHeartbeat();
}

void loop() {
    readSerial();

    // Nach abgeschlossenem HIGH-Effekt automatisch auf Attract-Mode zurückfallen
    if (activeA.finished && !attractPaused) activeA = attractA;
    if (activeB.finished && !attractPaused) activeB = attractB;

    // Effekt-Frames rendern
    updateEffect(activeA, ledsA, LEDS_PER_CHAIN, getSegmentA(activeA.segmentId));
    updateEffect(activeB, ledsB, LEDS_PER_CHAIN, getSegmentB(activeB.segmentId));

    FastLED.show();
}
