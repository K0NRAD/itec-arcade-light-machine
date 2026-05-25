# Arcade Console LED-Steuerung

---

## Hardware

| Komponente       | Spezifikation                    |
| ---------------- | -------------------------------- |
| Mikrocontroller  | ESP32                            |
| LED-Typ          | WS2812B, 60 LEDs/Meter (GRB)    |
| Helligkeit       | 20% (Betriebshelligkeit)         |
| Kette A          | 166 LEDs, Pin 5                  |
| Netzteil         | 5V/3A                            |
| Stromeinspeisung | Anfang der Kette                 |
| PC-Verbindung    | USB Serial, 115200 Baud          |

---

## LED-Segmente

| Segment | Zone           | LEDs | Buffer-Range | Richtung |
| ------- | -------------- | ---- | ------------ | -------- |
| 0       | Marquee        | 34   | 0 – 33       | reversed |
| 1       | Monitor oben   | 31   | 34 – 64      | vorwärts |
| 2       | Monitor links  | 18   | 114 – 131    | reversed |
| 3       | Monitor unten  | 31   | 83 – 113     | reversed |
| 4       | Monitor rechts | 18   | 65 – 82      | vorwärts |
| 5       | Control Panel  | 34   | 132 – 165    | vorwärts |

Physische Strip-Reihenfolge: Marquee → Monitor oben → Monitor rechts → Monitor unten → Monitor links → Control Panel

Segment `99` adressiert alle 6 Segmente gleichzeitig.

---

## Software-Architektur

```
Pac-Man (Godot)         ──┐
Asteroids (JS/Browser)  ──┼──► Python Bridge ──► USB Serial ──► ESP32 ──► LEDs
Space Invaders (Python) ──┘
         │
         └── WebSocket ws://localhost:8765
```

| Komponente    | Verzeichnis            | Beschreibung                             |
| ------------- | ---------------------- | ---------------------------------------- |
| Firmware      | `arcade-led-firmware/` | ESP32-Firmware (PlatformIO, FastLED)     |
| Bridge        | `arcade-led-bridge/`   | Python-Prozess: WebSocket ↔ USB Serial   |
| Control Panel | `index.html`           | Browser-UI: Effekt-Builder + Simulator   |

---

## Protokoll

**Format:** JSON, eine Zeile, `\n`-terminiert  
**Transport:** WebSocket (Spiele → Bridge) und USB Serial (Bridge → ESP32)

### Effekt-Befehl

```json
{
  "cmd": "effect",
  "chain": "A",
  "type": "chase",
  "segment": 1,
  "color": {"r": 255, "g": 215, "b": 0},
  "speed": 40,
  "length": 5,
  "repeat": 1,
  "dir": 1,
  "priority": 2
}
```

`segment` und `dir` können auch als Array übergeben werden, um mehrere Segmente mit individuellen Richtungen anzusprechen:

```json
{"cmd": "effect", "segment": [0, 2, 4], "dir": [1, -1, 1], ...}
```

### Attract-Mode steuern

```json
{"cmd": "attract", "state": "pause"}
{"cmd": "attract", "state": "resume"}
```

### ESP32-Antworten

```json
{"status": "ready", "version": "1.0.0", "leds_a": 166}
{"status": "error", "code": 2, "msg": "unknown segment"}
```

| Fehler-Code | Bedeutung         |
| ----------- | ----------------- |
| 1           | Ungültiges JSON   |
| 2           | Unbekanntes Segment |
| 3           | Unbekannter Effekt-Typ |
| 4           | Ungültige Parameter |
| 5           | Unbekannter Befehl |

---

## Effekt-Typen

| Typ     | Beschreibung                              | `speed`-Bedeutung   | `length`-Bedeutung |
| ------- | ----------------------------------------- | ------------------- | ------------------ |
| fill    | Ganzes Segment in einer Farbe             | —                   | —                  |
| blink   | Segment blinkt in Intervallen             | ms/Toggle           | —                  |
| chase   | Lauflicht mit Schweif                     | ms/Schritt          | Schweif-LEDs       |
| pulse   | Helligkeit atmet auf/ab (BPM)             | BPM                 | —                  |
| rainbow | Regenbogen läuft durch Segment            | ms/Schritt          | —                  |
| sparkle | Zufällige LEDs blitzen auf                | ms/Frame            | —                  |
| wipe    | Segment füllt sich von einer Seite        | ms/LED              | —                  |
| scanner | Knight-Rider-Effekt (symmetrisch)         | ms/Schritt          | Fade-Länge         |
| off     | Segment aus                               | —                   | —                  |

`dir`: `1` = vorwärts, `-1` = rückwärts (wirkt auf chase, wipe, scanner)

---

## Effekt-Priorisierung

| Priorität | Name   | Beispiele                               |
| --------- | ------ | --------------------------------------- |
| 3         | HIGH   | Game Over, Level Complete, Player Death |
| 2         | MEDIUM | Treffer, Bonus, Pill gegessen           |
| 1         | LOW    | Attract-Mode, Idle                      |

- Höhere Priorität verdrängt immer einen laufenden Effekt
- Gleiche Priorität: neuer Effekt verdrängt den laufenden
- Niedrigere Priorität wird ignoriert
- Nach Ablauf eines Effekts: automatischer Rückfall auf Attract-Mode

---

## Attract-Mode

- Effekt: Pulse Blau (`r:0, g:100, b:200`) auf allen Segmenten, BPM=30
- Wird automatisch aktiviert wenn kein Spiel läuft
- Kann über `{"cmd":"attract","state":"pause"}` pausiert werden

---

## Spielfarben

| Spiel          | Farbe  | RGB              |
| -------------- | ------ | ---------------- |
| Pac-Man        | Gelb   | 255 / 215 / 0    |
| Space Invaders | Grün   | 0 / 255 / 0      |
| Asteroids      | Cyan   | 0 / 255 / 255    |
| Attract        | Blau   | 0 / 100 / 200    |

---

## Quickstart

### 1. ESP32-Firmware flashen

```bash
cd arcade-led-firmware
pio run --target upload
```

### 2. Python Bridge starten

```bash
cd arcade-led-bridge
python -m venv .venv
source .venv/bin/activate        # macOS/Linux
# .venv\Scripts\activate.bat     # Windows CMD

pip install -r requirements.txt
ARCADE_SERIAL_PORT=/dev/cu.SLAB_USBtoUART python bridge.py
```

Seriellen Port ermitteln (macOS):

```bash
ls /dev/tty.*
# ESP32 erscheint als /dev/tty.SLAB_USBtoUART oder /dev/tty.usbmodemXXXX
```

### 3. Control Panel öffnen

`index.html` direkt im Browser öffnen — verbindet automatisch mit `ws://localhost:8765`.

---

## Verzeichnisstruktur

```
itec-arcade-light-machine/
├── arcade-led-firmware/        # ESP32-Firmware (PlatformIO)
│   ├── src/
│   │   ├── main.cpp
│   │   ├── arcade_controller.*
│   │   ├── chain_controller.*
│   │   ├── effects.*
│   │   ├── segments.*
│   │   ├── serial_parser.*
│   │   └── protocol_handler.*
│   ├── include/config.h
│   └── platformio.ini
├── arcade-led-bridge/          # Python Bridge
│   ├── bridge.py
│   ├── serial_handler.py
│   ├── ws_server.py
│   ├── config.py
│   └── tests/
└── index.html                  # Browser Control Panel + Simulator
```
