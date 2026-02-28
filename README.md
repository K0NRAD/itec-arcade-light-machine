## Gesamtkonzept: Arcade Console LED-Steuerung

---

## Hardware

| Komponente        | Spezifikation                          |
| ----------------- | -------------------------------------- |
| Mikrocontroller   | ESP32                                  |
| LED-Typ           | WS2812B, 60 LEDs/Meter                 |
| Helligkeit        | 20% (Betriebshelligkeit)               |
| Ketten            | 2 unabhängige Ketten à 186 LEDs / 3,1m |
| Netzteil          | 2× 5V/3A (je eine pro Kette)           |
| Stromeinspeisung  | je 1× am Anfang pro Kette              |
| Daten-Pin Kette A | ESP32 Pin 5                            |
| Daten-Pin Kette B | ESP32 Pin 18                           |
| PC-Verbindung     | USB Serial                             |

---

## LED-Segmente

**Kette A — Spieler-nah (sichtbar)**

| Segment | Zone           | LEDs | Index   |
| ------- | -------------- | ---- | ------- |
| 0       | Marquee        | 42   | 0–41    |
| 1       | Monitor rechts | 30   | 42–71   |
| 2       | Monitor unten  | 30   | 72–101  |
| 3       | Monitor links  | 30   | 102–131 |
| 4       | Monitor oben   | 12   | 132–143 |
| 5       | Control Panel  | 42   | 144–185 |

**Kette B — Ambient (Gehäuse)**

| Segment | Zone         | LEDs | Index   |
| ------- | ------------ | ---- | ------- |
| 0       | Seite links  | 72   | 0–71    |
| 1       | Boden        | 42   | 72–113  |
| 2       | Seite rechts | 72   | 114–185 |

---

## Software-Architektur

```
Pac-Man (Godot)         ──┐
Asteroids (JS/Browser)  ──┼──► Python Bridge ──► USB Serial ──► ESP32 ──► LEDs
Space Invaders (Python) ──┘
         │
         └── WebSocket (ws://localhost:8765)
```

| Spiel          | Sprache            | Anbindung           |
| -------------- | ------------------ | ------------------- |
| Pac-Man        | Godot              | WebSocket localhost |
| Asteroids      | JavaScript/Browser | WebSocket localhost |
| Space Invaders | Python             | WebSocket localhost |

Die Python Bridge ist ein eigenständiger Prozess — einzige Schnittstelle zum ESP32.

---

## Protokoll

**Format:** JSON, eine Zeile, terminiert mit `\n`
**Richtung:** Bidirektional
**Fehlerbehandlung:** ESP32 meldet nur Fehler (NACK) — kein ACK bei Erfolg
**Heartbeat:** ESP32 sendet beim Start automatisch seinen Status

```json
// Befehl PC → ESP32
{"cmd": "effect", "type": "chase", "segment": "monitor", "color": {"r": 255, "g": 200, "b": 0}, "speed": 50, "repeat": 1, "priority": 2}

// Fehler ESP32 → PC
{"status": "error", "code": 2, "msg": "unknown segment"}

// Heartbeat ESP32 → PC
{"status": "ready", "version": "1.0.0", "leds_a": 186, "leds_b": 186}
```

---

## Effekt-Typen

| Typ     | Beschreibung                       |
| ------- | ---------------------------------- |
| FILL    | Ganzes Segment in einer Farbe      |
| BLINK   | Segment blinkt in Intervallen      |
| CHASE   | Lauflicht mit Schweif              |
| PULSE   | Helligkeit atmet auf/ab            |
| RAINBOW | Regenbogen läuft durch Segment     |
| SPARKLE | Zufällige LEDs blitzen auf         |
| WIPE    | Segment füllt sich von einer Seite |
| OFF     | Segment aus                        |

**Effekt-Parameter:** type, segment, color, speed, length, repeat, brightness, priority

---

## Effekt-Priorisierung

| Ebene  | Priorität | Beispiele                               |
| ------ | --------- | --------------------------------------- |
| HIGH   | 3         | Game Over, Level Complete, Player Death |
| MEDIUM | 2         | Treffer, Bonus, Pill gegessen           |
| LOW    | 1         | Attract-Mode, Idle                      |

- Höhere Priorität verdrängt immer
- Gleiche Priorität: neuer Effekt verdrängt laufenden
- Niedrigere Priorität wird ignoriert
- Nach HIGH-Effekt: automatischer Rückfall auf Attract-Mode
- Jede Kette hat eigene unabhängige Priorisierung

---

## Attract-Mode

| Phase          | Zeitraum | Kette A              | Kette B              |
| -------------- | -------- | -------------------- | -------------------- |
| Soft Idle      | 0–5 Min  | Pulse Spielfarbe     | Langsamer Rainbow    |
| Active Attract | 5+ Min   | Spielfarben rotieren | Chase Seitenstreifen |

Spielfarben: Pac-Man Gelb → Space Invaders Grün → Asteroids Weiß/Cyan

---

## Arcade-Effekt-Bibliothek (Kurzübersicht)

| Spiel          | Schlüsselereignisse                                               | Primärfarbe   |
| -------------- | ----------------------------------------------------------------- | ------------- |
| Pac-Man        | Pill, Power Pill, Geist, Level Complete, Death, Game Over, Bonus  | Gelb          |
| Space Invaders | Alien Hit, Spieler Hit, UFO, Welle komplett, Game Over            | Grün          |
| Asteroids      | Asteroid Hit, Spieler Hit, Extra Leben, Level Complete, Game Over | Weiß/Cyan     |
| System         | Spiel Start, Spiel Ende, Highscore, Münze                         | Spielabhängig |

---

Das Konzept ist vollständig. Bereit für die **Implementierung** — womit starten wir?