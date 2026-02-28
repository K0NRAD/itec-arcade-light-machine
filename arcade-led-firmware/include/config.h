#pragma once

// --- LED-Konfiguration ---
#define PIN_CHAIN_A       5       // GPIO für Kette A (Spieler-nah)
#define PIN_CHAIN_B       18      // GPIO für Kette B (Ambient)
#define LEDS_PER_CHAIN    186
#define LED_BRIGHTNESS    51      // 20% von 255
#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB

// --- Serial-Konfiguration ---
#define SERIAL_BAUD       115200
#define SERIAL_BUFFER_LEN 256
#define SERIAL_TIMEOUT_MS 2000    // Unvollständige Befehle nach 2s verwerfen

// --- Firmware-Version ---
#define FW_VERSION        "1.0.0"
