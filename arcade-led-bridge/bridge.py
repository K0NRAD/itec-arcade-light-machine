import asyncio
import json
import logging
import sys
import threading

from config import SERIAL_BAUD, SERIAL_PORT, WS_HOST, WS_PORT
from serial_handler import SerialHandler
from ws_server import WebSocketServer

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(name)s — %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger("bridge")


def on_esp32_message(message: str):
    """ESP32-Antworten auswerten und loggen."""
    logger.info("ESP32 → %s", message)

    try:
        data = json.loads(message)
    except json.JSONDecodeError:
        logger.warning("ESP32 sendete ungültiges JSON: %s", message)
        return

    status = data.get("status")

    if status == "ready":
        logger.info(
            "ESP32 bereit — Firmware %s | Kette A: %d LEDs | Kette B: %d LEDs",
            data.get("version", "?"),
            data.get("leds_a", 0),
            data.get("leds_b", 0),
        )

    elif status == "error":
        logger.error(
            "ESP32-Fehler [Code %s]: %s",
            data.get("code", "?"),
            data.get("msg", ""),
        )


def main():
    logger.info("Arcade LED Bridge startet...")

    # Serial-Verbindung zum ESP32 herstellen
    serial_handler = SerialHandler(SERIAL_PORT, SERIAL_BAUD)
    try:
        serial_handler.connect()
    except Exception as exc:
        logger.critical("Serial-Verbindung fehlgeschlagen: %s", exc)
        sys.exit(1)

    # ESP32-Antworten in eigenem Daemon-Thread empfangen
    listen_thread = threading.Thread(
        target=serial_handler.listen,
        args=(on_esp32_message,),
        daemon=True,
        name="esp32-listener",
    )
    listen_thread.start()

    # WebSocket-Server starten
    server = WebSocketServer(WS_HOST, WS_PORT, serial_handler)

    try:
        asyncio.run(server.start())
    except KeyboardInterrupt:
        logger.info("Bridge wird beendet (SIGINT)...")
    finally:
        serial_handler.disconnect()
        logger.info("Bridge gestoppt.")


if __name__ == "__main__":
    main()
