import os

# Serielle Verbindung zum ESP32
# Überschreibbar via Umgebungsvariable: ARCADE_SERIAL_PORT
SERIAL_PORT = os.environ.get("ARCADE_SERIAL_PORT", "/dev/ttyUSB0")
SERIAL_BAUD = 115200

# WebSocket-Server für Spiele
WS_HOST = "localhost"
WS_PORT = 8765
