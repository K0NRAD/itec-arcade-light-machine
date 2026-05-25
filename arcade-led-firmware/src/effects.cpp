#include "effects.h"

bool applyEffect(Effect& current, const Effect& incoming) {
    if (incoming.priority >= current.priority) {
        current = incoming;
        return true;
    }
    return false;
}

void updateEffect(Effect& fx, CRGB* ledArray, uint16_t numLeds, const Segment& seg) {
    if (fx.finished) return;

    uint32_t now = millis();

    switch (fx.type) {

        case EFFECT_OFF: {
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i] = CRGB::Black;
            }
            fx.finished = true;
            break;
        }

        case EFFECT_FILL: {
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i] = fx.color;
            }
            // Bei endlos (repeat=-1) bleibt FILL aktiv, Farbe wird jeden Frame gesetzt
            if (fx.repeat != -1) {
                fx.finished = true;
            }
            break;
        }

        case EFFECT_BLINK: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            // Gerade step = LED an, ungerade = aus
            bool ledOn = (fx.step % 2 == 0);
            CRGB blinkColor = ledOn ? fx.color : CRGB::Black;
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i] = blinkColor;
            }
            fx.step++;

            // Ein vollständiger Blink = An + Aus
            if (!ledOn) {
                fx.repeatCount++;
                if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                    fx.finished = true;
                }
            }
            break;
        }

        case EFFECT_CHASE: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            uint16_t segLen = seg.count();

            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i] = CRGB::Black;
            }

            // Kopf und Schweif zeichnen; direction=-1 kehrt Laufrichtung um
            for (uint8_t t = 0; t < fx.length; t++) {
                int32_t pos = (fx.direction >= 0)
                    ? (int32_t)fx.step - t
                    : (int32_t)(segLen - 1 - fx.step) + t;
                if (pos >= 0 && pos < (int32_t)segLen) {
                    uint8_t brightness = 255 - (uint8_t)((255 / fx.length) * t);
                    ledArray[seg.start + pos] = fx.color;
                    ledArray[seg.start + pos].nscale8(brightness);
                }
            }

            fx.step++;
            if (fx.step >= segLen) {
                fx.step = 0;
                fx.repeatCount++;
                if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                    fx.finished = true;
                }
            }
            break;
        }

        case EFFECT_PULSE: {
            // speed = BPM — Periode = 60000ms / BPM
            uint32_t period = 60000UL / max((uint16_t)1, fx.speed);
            uint8_t  phase  = (uint8_t)((millis() % period) * 255UL / period);
            uint8_t  brightness = sin8(phase);

            CRGB pulseColor = fx.color;
            pulseColor.nscale8(brightness);
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i] = pulseColor;
            }
            // PULSE läuft immer endlos (repeat wird ignoriert)
            break;
        }

        case EFFECT_RAINBOW: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            uint16_t segLen = seg.count();
            uint8_t  baseHue = (uint8_t)(fx.step & 0xFF);

            for (uint16_t i = 0; i < segLen; i++) {
                uint8_t hue = baseHue + (uint8_t)(i * 256UL / segLen);
                ledArray[seg.start + i] = CHSV(hue, 255, 255);
            }

            fx.step++;
            // 256 Schritte = vollständige Farbrotation
            if ((fx.step & 0xFF) == 0) {
                fx.repeatCount++;
                if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                    fx.finished = true;
                }
            }
            break;
        }

        case EFFECT_SPARKLE: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            uint16_t segLen = seg.count();

            // Alle LEDs abdunkeln (sanftes Ausblenden alter Funken)
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i].nscale8(128);
            }

            // Zufällige LEDs auf volle Helligkeit setzen
            uint16_t sparks = max((uint16_t)1, (uint16_t)(segLen / 8));
            for (uint16_t s = 0; s < sparks; s++) {
                uint16_t idx = seg.start + random16(segLen);
                ledArray[idx] = fx.color;
            }

            fx.repeatCount++;
            if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                fx.finished = true;
            }
            break;
        }

        case EFFECT_WIPE: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            uint16_t segLen = seg.count();
            if (fx.step < segLen) {
                // direction=-1 füllt von Ende nach Anfang
                uint16_t ledIdx = (fx.direction >= 0)
                    ? seg.start + fx.step
                    : seg.end   - fx.step;
                ledArray[ledIdx] = fx.color;
                fx.step++;
            } else {
                fx.repeatCount++;
                if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                    fx.finished = true;
                } else {
                    for (uint16_t i = seg.start; i <= seg.end; i++) {
                        ledArray[i] = CRGB::Black;
                    }
                    fx.step = 0;
                }
            }
            break;
        }

        case EFFECT_SCANNER: {
            if (now - fx.lastUpdate < fx.speed) break;
            fx.lastUpdate = now;

            uint16_t segLen = seg.count();
            if (segLen < 3) break;

            uint16_t half       = segLen / 2;
            uint16_t totalSteps = half * 2; // Raus + Rein, je 1 Frame Pause an den Rändern

            // Ausblenden erzeugt den Nachzieh-Effekt — length steuert Schweif-Länge
            uint8_t fadeVal = (uint8_t)(255 - 200 / max((uint8_t)1, fx.length));
            for (uint16_t i = seg.start; i <= seg.end; i++) {
                ledArray[i].nscale8(fadeVal);
            }

            // Ping-Pong-Offset vom Mittelpunkt; direction=-1 startet von den Rändern
            uint16_t rawOffset = (fx.step < half)
                ? fx.step
                : (2 * half - 1) - fx.step;
            uint16_t offset = (fx.direction >= 0)
                ? rawOffset
                : (half - 1) - rawOffset;
            int32_t center = (int32_t)seg.start + (int32_t)half;

            int32_t lPos = center - (int32_t)offset;
            int32_t rPos = center + (int32_t)offset;

            if (lPos >= (int32_t)seg.start && lPos <= (int32_t)seg.end) {
                ledArray[lPos] = fx.color;
            }
            if (rPos != lPos && rPos >= (int32_t)seg.start && rPos <= (int32_t)seg.end) {
                ledArray[rPos] = fx.color;
            }

            fx.step++;
            if (fx.step >= totalSteps) {
                fx.step = 0;
                fx.repeatCount++;
                if (fx.repeat != -1 && fx.repeatCount >= fx.repeat) {
                    fx.finished = true;
                }
            }
            break;
        }
    }
}
