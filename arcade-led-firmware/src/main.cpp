#include "arcade_controller.h"

ArcadeController controller;  // global → BSS-Segment, kein Stack-Overhead

void setup() { controller.initialize(); }
void loop()  { controller.loop(); }
