import json
import logging
import threading

import serial

logger = logging.getLogger(__name__)


class SerialHandler:
    def __init__(self, port: str, baud: int):
        self._port = port
        self._baud = baud
        self._serial: serial.Serial | None = None
        self._lock = threading.Lock()
        self._running = False

    def connect(self):
        """Verbindung zum ESP32 herstellen."""
        self._serial = serial.Serial(self._port, self._baud, timeout=1)
        self._running = True
        logger.info("Verbunden mit ESP32 auf %s @ %d Baud", self._port, self._baud)

    def send(self, json_str: str):
        """JSON-Befehl an ESP32 senden (thread-safe)."""
        with self._lock:
            if not self._serial or not self._serial.is_open:
                logger.warning("Serial nicht offen — Befehl verworfen: %s", json_str)
                return

            line = json_str if json_str.endswith("\n") else json_str + "\n"
            self._serial.write(line.encode("utf-8"))
            logger.debug("Gesendet: %s", line.strip())

    def listen(self, callback):
        """Antworten vom ESP32 empfangen (blockierend — in eigenem Thread starten).

        Ruft callback(message: str) für jede empfangene Zeile auf.
        """
        while self._running and self._serial and self._serial.is_open:
            try:
                raw = self._serial.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8").strip()
                if not line:
                    continue

                self._validate_and_dispatch(line, callback)

            except serial.SerialException as exc:
                logger.error("Serial-Verbindungsfehler: %s", exc)
                break
            except UnicodeDecodeError as exc:
                logger.warning("Ungültige Zeichenkodierung vom ESP32: %s", exc)

    def disconnect(self):
        """Verbindung zum ESP32 sauber schließen."""
        self._running = False
        if self._serial and self._serial.is_open:
            self._serial.close()
            logger.info("Serial-Verbindung getrennt")

    def _validate_and_dispatch(self, line: str, callback):
        """JSON-Validierung vor Weitergabe an Callback."""
        try:
            json.loads(line)
        except json.JSONDecodeError:
            logger.warning("Ungültiges JSON vom ESP32: %s", line)
            return

        callback(line)
