import asyncio
import json
import logging

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
            "ESP32 bereit — Firmware %s | Kette A: %d LEDs",
            data.get("version", "?"),
            data.get("leds_a", 0),
        )
    elif status == "error":
        logger.error(
            "ESP32-Fehler [Code %s]: %s",
            data.get("code", "?"),
            data.get("msg", ""),
        )


def main():
    logger.info("Arcade LED Bridge startet...")
    logger.info("Serial: %s @ %d Baud  |  WebSocket: ws://%s:%d", SERIAL_PORT, SERIAL_BAUD, WS_HOST, WS_PORT)

    serial_handler = SerialHandler(SERIAL_PORT, SERIAL_BAUD)
    serial_handler.start(on_esp32_message)

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
