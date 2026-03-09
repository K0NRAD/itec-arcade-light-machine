import os

# Serielle Verbindung zum ESP32
# Überschreibbar via Umgebungsvariable: ARCADE_SERIAL_PORT

# Den genauen Windows Port (COM3, COM4, ...) im Geräte-Manager unter 
# Anschlüsse (COM & LPT) nachschauen 
# der ESP32 erscheint als "Silicon Labs CP210x USB to UART Bridge".
#
# SERIAL_PORT = os.environ.get("ARCADE_SERIAL_PORT", "COM3")

# SERIAL_PORT = os.environ.get("ARCADE_SERIAL_PORT", "/dev/ttyUSB0")
SERIAL_PORT = os.environ.get("ARCADE_SERIAL_PORT", "/dev/cu.SLAB_USBtoUART")
SERIAL_BAUD = 115200

# WebSocket-Server für Spiele
WS_HOST = "localhost"
WS_PORT = 8765
