#pragma once
#include <stdint.h>

// LED-Bereich eines physischen Segments
struct Segment {
    uint16_t start;
    uint16_t end;
    uint16_t count() const { return end - start + 1; }
};

// --- Kette A: Spieler-nah (sichtbar) ---
constexpr Segment SEG_A_MARQUEE        = {0,   33};
constexpr Segment SEG_A_MONITOR_TOP    = {34,  64};
constexpr Segment SEG_A_MONITOR_RIGHT  = {65,  82};
constexpr Segment SEG_A_MONITOR_BOTTOM = {83,  113};
constexpr Segment SEG_A_MONITOR_LEFT   = {114, 131};
constexpr Segment SEG_A_CONTROL_PANEL  = {132, 165};

// Segment-ID nach JSON-Protokoll auflösen
// Kette A: 0=marquee, 1=monitor_top, 2=monitor_left,
//          3=monitor_bottom, 4=monitor_right, 5=control_panel, 99=all
const Segment& getSegmentA(uint8_t id);
