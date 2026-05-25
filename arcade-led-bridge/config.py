import os

# Serielle Verbindung zum ESP32
# Überschreibbar via Umgebungsvariable: ARCADE_SERIAL_PORT
#
# Windows:  COM3, COM4, ... (Geräte-Manager → Anschlüsse)
# Linux:    /dev/ttyUSB0
# macOS:    /dev/cu.SLAB_USBtoUART
SERIAL_PORT = os.environ.get("ARCADE_SERIAL_PORT", "/dev/cu.SLAB_USBtoUART")
SERIAL_BAUD = 115200

# Reconnect-Verhalten bei Serial-Verbindungsverlust
SERIAL_RECONNECT_DELAY_INITIAL = 1.0   # Sekunden bis zum ersten Retry
SERIAL_RECONNECT_DELAY_MAX     = 30.0  # Maximaler Delay (exponentielles Backoff)

# WebSocket-Server für Spiele
WS_HOST = "localhost"
WS_PORT = 8765
