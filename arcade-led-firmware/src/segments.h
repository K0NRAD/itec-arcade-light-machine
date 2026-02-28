#pragma once
#include <stdint.h>

// LED-Bereich eines physischen Segments
struct Segment {
    uint16_t start;
    uint16_t end;
    uint16_t count() const { return end - start + 1; }
};

// --- Kette A: Spieler-nah (sichtbar) ---
constexpr Segment SEG_A_MARQUEE        = {0,   41};
constexpr Segment SEG_A_MONITOR_RIGHT  = {42,  71};
constexpr Segment SEG_A_MONITOR_BOTTOM = {72,  101};
constexpr Segment SEG_A_MONITOR_LEFT   = {102, 131};
constexpr Segment SEG_A_MONITOR_TOP    = {132, 143};
constexpr Segment SEG_A_CONTROL_PANEL  = {144, 185};

// --- Kette B: Ambient (Gehäuse) ---
constexpr Segment SEG_B_SIDE_LEFT  = {0,   71};
constexpr Segment SEG_B_FLOOR      = {72,  113};
constexpr Segment SEG_B_SIDE_RIGHT = {114, 185};

// Segment-ID nach JSON-Protokoll auflösen
// Kette A: 0=marquee, 1=monitor_right, 2=monitor_bottom,
//          3=monitor_left, 4=monitor_top, 5=control_panel, 99=all
// Kette B: 0=side_left, 1=floor, 2=side_right, 99=all
const Segment& getSegmentA(uint8_t id);
const Segment& getSegmentB(uint8_t id);
