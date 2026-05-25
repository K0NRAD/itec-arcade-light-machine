# arcade-led-bridge

Python-Prozess der WebSocket-Befehle von Spielen (Godot, JavaScript, Python) entgegennimmt und als JSON-Kommandos über USB Serial an den ESP32 weiterleitet.

---

## Architektur

```
Spiele (Godot / JS / Python)
  │
  │  WebSocket ws://localhost:8765
  ▼
WebSocketServer          ← ws_server.py
  │
  │  Validierung + Weiterleitung
  ▼
SerialHandler            ← serial_handler.py
  │
  │  USB Serial 115200 Baud
  ▼
ESP32
```

---

## Installation

```bash
# Im Projektverzeichnis arcade-led-bridge/
python -m venv .venv

# macOS / Linux
source .venv/bin/activate

# Windows CMD
.venv\Scripts\activate.bat

# Windows PowerShell
.venv\Scripts\Activate.ps1

pip install -r requirements.txt
```

---

## Starten

```bash
# macOS / Linux — Standard-Port
python bridge.py

# macOS — ESP32 S3 (nativer USB)
ARCADE_SERIAL_PORT=/dev/tty.usbmodem14301 python bridge.py

# Windows
set ARCADE_SERIAL_PORT=COM3
python bridge.py
```

Seriellen Port ermitteln (macOS):

```bash
ls /dev/tty.*
# ESP32 erscheint als /dev/tty.SLAB_USBtoUART oder /dev/tty.usbmodemXXXX
```

---

## Konfiguration

Alle Werte in `config.py`, überschreibbar via Umgebungsvariablen:

| Variable               | Standard                  | Beschreibung                            |
| ---------------------- | ------------------------- | --------------------------------------- |
| `ARCADE_SERIAL_PORT`   | `/dev/cu.SLAB_USBtoUART`  | Serieller Port des ESP32                |
| `SERIAL_BAUD`          | `115200`                  | Baudrate (muss mit Firmware übereinstimmen) |
| `SERIAL_RECONNECT_DELAY_INITIAL` | `1.0`         | Sekunden bis zum ersten Retry           |
| `SERIAL_RECONNECT_DELAY_MAX`     | `30.0`        | Maximaler Retry-Delay (Backoff-Deckel)  |
| `WS_HOST`              | `localhost`               | WebSocket-Bind-Adresse                  |
| `WS_PORT`              | `8765`                    | WebSocket-Port                          |

---

## Ausfallsicherheit

Die Bridge ist auf dauerhaften Betrieb ausgelegt — beide Verbindungen erholen sich automatisch:

**Serial-Reconnect:**  
Der `SerialHandler` versucht nach einem Verbindungsverlust automatisch neu zu verbinden. Der Delay wächst exponentiell (1s → 2s → 4s → ... → max 30s) und wird nach einer erfolgreichen Verbindung auf den Initialwert zurückgesetzt. Die Bridge startet auch dann, wenn der ESP32 beim Start noch nicht erreichbar ist.

**WebSocket-Clients:**  
Der WebSocket-Server bleibt dauerhaft aktiv. Spiele verbinden sich selbst neu wenn sie die Verbindung verlieren. Befehle die während eines Serial-Ausfalls eingehen werden verworfen — LED-Befehle sind zeitkritisch und nach einem Reconnect nicht mehr relevant.

---

## Tests

```bash
pytest tests/ -v
```

---

## Dateien

| Datei                | Inhalt                                                  |
| -------------------- | ------------------------------------------------------- |
| `bridge.py`          | Einstiegspunkt — verbindet SerialHandler und WS-Server  |
| `serial_handler.py`  | Serial-Verbindung mit automatischem Reconnect           |
| `ws_server.py`       | Asynchroner WebSocket-Server (asyncio + websockets)     |
| `config.py`          | Port, Baudrate, Reconnect-Delays, WS-Host/Port          |
| `tests/`             | Unit-Tests für SerialHandler und WebSocketServer        |
