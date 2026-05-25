#include "segments.h"
#include "config.h"

static const Segment SEG_A_ALL = {0, LEDS_PER_CHAIN - 1};

const Segment& getSegmentA(uint8_t id) {
    switch (id) {
        case 0:  return SEG_A_MARQUEE;
        case 1:  return SEG_A_MONITOR_TOP;
        case 2:  return SEG_A_MONITOR_LEFT;
        case 3:  return SEG_A_MONITOR_BOTTOM;
        case 4:  return SEG_A_MONITOR_RIGHT;
        case 5:  return SEG_A_CONTROL_PANEL;
        default: return SEG_A_ALL;  // 99 = gesamte Kette
    }
}
