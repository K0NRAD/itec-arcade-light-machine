# Arcade LED Bridge — WebSocket API

Die Bridge stellt einen WebSocket-Server bereit, über den Spiele LED-Effekte auslösen können.

**Endpunkt:** `ws://localhost:8765`  
**Format:** JSON, eine Zeile, `\n`-terminiert  
**Richtung:** Spiel → Bridge (Befehle), Bridge → Spiel (Fehler + Status)

---

## Verbindung herstellen

Die Bridge muss laufen bevor das Spiel eine Verbindung aufbaut. Nach dem Verbindungsaufbau kann das Spiel sofort Befehle senden — kein Handshake nötig.

Wenn der ESP32 beim Start der Bridge bereit ist, sendet er automatisch einen `ready`-Status, den die Bridge an alle verbundenen Clients weiterleitet:

```json
{"status": "ready", "version": "1.0.0", "leds_a": 166}
```

---

## Befehle

### `effect` — LED-Effekt auslösen

```json
{
  "cmd":      "effect",
  "chain":    "A",
  "type":     "chase",
  "segment":  1,
  "color":    {"r": 255, "g": 215, "b": 0},
  "speed":    40,
  "length":   5,
  "repeat":   1,
  "dir":      1,
  "priority": 2
}
```

#### Pflichtfelder

| Feld       | Typ    | Beschreibung                        |
| ---------- | ------ | ----------------------------------- |
| `cmd`      | string | Muss `"effect"` sein                |
| `chain`    | string | Immer `"A"`                         |
| `type`     | string | Effekt-Typ (siehe unten)            |
| `segment`  | int \| array | Segment-ID oder Array von IDs |
| `color`    | object | RGB-Farbe `{"r":0–255, "g":0–255, "b":0–255}` |
| `priority` | int    | 1 = Low, 2 = Medium, 3 = High       |

#### Optionale Felder

| Feld     | Typ         | Standard | Beschreibung                                       |
| -------- | ----------- | -------- | -------------------------------------------------- |
| `speed`  | int         | 100      | Geschwindigkeit (Einheit je nach Effekt-Typ)       |
| `length` | int         | 5        | Länge (Einheit je nach Effekt-Typ)                 |
| `repeat` | int         | -1       | Anzahl Wiederholungen, `-1` = endlos               |
| `dir`    | int \| array | 1       | Richtung: `1` = vorwärts, `-1` = rückwärts        |

---

### `attract` — Attract-Mode steuern

```json
{"cmd": "attract", "state": "pause"}
{"cmd": "attract", "state": "resume"}
```

`pause` unterbricht den Attract-Mode (z.B. wenn ein Spiel startet).  
`resume` aktiviert ihn wieder und setzt alle Segmente auf Attract zurück.

---

## Segmente

| ID  | Zone           | LEDs |
| --- | -------------- | ---- |
| 0   | Marquee        | 34   |
| 1   | Monitor oben   | 31   |
| 2   | Monitor links  | 18   |
| 3   | Monitor unten  | 31   |
| 4   | Monitor rechts | 18   |
| 5   | Control Panel  | 34   |
| 99  | Alle Segmente  | 166  |

---

## Mehrere Segmente gleichzeitig

`segment` und `dir` können als JSON-Array übergeben werden. Ein `dir`-Array muss genau so viele Einträge haben wie das `segment`-Array.

```json
{
  "cmd":      "effect",
  "chain":    "A",
  "type":     "chase",
  "segment":  [0, 3, 5],
  "dir":      [1, -1, 1],
  "color":    {"r": 0, "g": 255, "b": 0},
  "speed":    30,
  "repeat":   -1,
  "priority": 2
}
```

Wird `dir` als einzelner Integer angegeben, gilt dieser für alle Segmente im Array.

---

## Effekt-Typen

| Typ       | Beschreibung                        | `speed`          | `length`       |
| --------- | ----------------------------------- | ---------------- | -------------- |
| `fill`    | Ganzes Segment in einer Farbe       | —                | —              |
| `blink`   | Segment blinkt                      | ms/Toggle        | —              |
| `chase`   | Lauflicht mit Schweif               | ms/Schritt       | Schweif-LEDs   |
| `pulse`   | Helligkeit atmet auf/ab             | BPM              | —              |
| `rainbow` | Regenbogen läuft durch Segment      | ms/Schritt       | —              |
| `sparkle` | Zufällige LEDs blitzen auf          | ms/Frame         | —              |
| `wipe`    | Füllt Segment von einer Seite       | ms/LED           | —              |
| `scanner` | Knight-Rider (symmetrisch)          | ms/Schritt       | Fade-Länge     |
| `off`     | Segment aus                         | —                | —              |

`dir` wirkt auf: `chase`, `wipe`, `scanner`

---

## Effekt-Priorisierung

| Wert | Name   | Wann verwenden                          |
| ---- | ------ | --------------------------------------- |
| 3    | High   | Game Over, Level Complete, Player Death |
| 2    | Medium | Treffer, Bonus, Ereignis                |
| 1    | Low    | Hintergrundeffekte, Idle                |

Ein Effekt mit höherer Priorität verdrängt immer einen laufenden Effekt auf demselben Segment. Gleiche Priorität: neuer Effekt gewinnt. Niedrigere Priorität: wird ignoriert solange ein höherwertiger Effekt läuft. Nach Ablauf eines Effekts fällt das Segment automatisch auf den Attract-Mode zurück.

---

## Antworten

Die Bridge sendet ausschließlich Fehlermeldungen und ESP32-Statusinformationen. Erfolgreiche Befehle werden nicht bestätigt.

### Fehler

```json
{"status": "error", "code": 1, "msg": "invalid json"}
{"status": "error", "code": 5, "msg": "unknown cmd: reboot"}
```

| Code | Ursache                     |
| ---- | --------------------------- |
| 1    | Ungültiges JSON             |
| 2    | Unbekanntes Segment         |
| 3    | Unbekannter Effekt-Typ      |
| 4    | Ungültige Parameter         |
| 5    | Unbekannter oder fehlender Befehl |

### ESP32 bereit

```json
{"status": "ready", "version": "1.0.0", "leds_a": 166}
```

---

## Spielfarben (Empfehlung)

| Spiel          | Farbe | RGB             |
| -------------- | ----- | --------------- |
| Pac-Man        | Gelb  | r:255 g:215 b:0 |
| Space Invaders | Grün  | r:0 g:255 b:0   |
| Asteroids      | Cyan  | r:0 g:255 b:255 |

---

## Code-Beispiele

### Python

```python
import asyncio
import json
import websockets

async def send_effect():
    async with websockets.connect("ws://localhost:8765") as ws:
        # Pac-Man frisst Punkt
        await ws.send(json.dumps({
            "cmd": "effect", "chain": "A",
            "type": "blink",
            "segment": 1,
            "color": {"r": 255, "g": 215, "b": 0},
            "speed": 80, "repeat": 2, "priority": 2,
        }))

        # Auf Antwort prüfen (optional)
        try:
            response = await asyncio.wait_for(ws.recv(), timeout=0.5)
            data = json.loads(response)
            if data.get("status") == "error":
                print(f"Fehler [{data['code']}]: {data['msg']}")
        except asyncio.TimeoutError:
            pass  # Kein Fehler = Erfolg

asyncio.run(send_effect())
```

### JavaScript

```js
const ws = new WebSocket("ws://localhost:8765");

ws.addEventListener("open", () => {
  // Game Over — alle Segmente rot blitzen
  ws.send(JSON.stringify({
    cmd: "effect", chain: "A",
    type: "blink",
    segment: 99,
    color: { r: 255, g: 0, b: 0 },
    speed: 120, repeat: 4, priority: 3,
  }));
});

ws.addEventListener("message", (event) => {
  const data = JSON.parse(event.data);
  if (data.status === "error") {
    console.error(`LED-Fehler [${data.code}]: ${data.msg}`);
  }
});
```

### GDScript (Godot)

```gdscript
extends Node

var _socket := WebSocketPeer.new()

func _ready() -> void:
    _socket.connect_to_url("ws://localhost:8765")

func _process(_delta: float) -> void:
    _socket.poll()

func trigger_effect(payload: Dictionary) -> void:
    if _socket.get_ready_state() != WebSocketPeer.STATE_OPEN:
        return
    _socket.send_text(JSON.stringify(payload))

# Beispiel: Spieler stirbt
func on_player_death() -> void:
    trigger_effect({
        "cmd": "effect", "chain": "A",
        "type": "wipe",
        "segment": [1, 2, 3, 4],
        "dir": [-1, -1, 1, 1],
        "color": {"r": 255, "g": 50, "b": 0},
        "speed": 25, "repeat": 1, "priority": 3,
    })
```

---

## Typische Spielereignisse

```json
// Punkt gefressen (Pac-Man)
{"cmd":"effect","chain":"A","type":"blink","segment":1,"color":{"r":255,"g":215,"b":0},"speed":60,"repeat":1,"priority":2}

// Geist gefressen (Pac-Man)
{"cmd":"effect","chain":"A","type":"chase","segment":[1,2,3,4],"color":{"r":0,"g":0,"b":255},"speed":30,"length":6,"repeat":2,"priority":2}

// Alien abgeschossen (Space Invaders)
{"cmd":"effect","chain":"A","type":"sparkle","segment":1,"color":{"r":0,"g":255,"b":0},"speed":40,"repeat":3,"priority":2}

// Asteroid getroffen (Asteroids)
{"cmd":"effect","chain":"A","type":"scanner","segment":99,"color":{"r":0,"g":255,"b":255},"speed":20,"length":5,"repeat":1,"priority":2}

// Game Over (alle Spiele)
{"cmd":"effect","chain":"A","type":"wipe","segment":99,"color":{"r":255,"g":0,"b":0},"speed":15,"repeat":1,"priority":3}

// Spiel startet — Attract pausieren
{"cmd":"attract","state":"pause"}

// Spiel endet — Attract reaktivieren
{"cmd":"attract","state":"resume"}
```
