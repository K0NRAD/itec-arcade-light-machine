#pragma once

// JSON-Befehl aus dem Serial-Buffer verarbeiten und Effekte anwenden.
// Greift auf globale Effekt-Zustände aus main.cpp zu (via extern).
void processCommand(const char* json);
