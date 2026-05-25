import json
import logging
import threading
import time

import serial

from config import SERIAL_RECONNECT_DELAY_INITIAL, SERIAL_RECONNECT_DELAY_MAX

logger = logging.getLogger(__name__)


class SerialHandler:
    def __init__(self, port: str, baud: int):
        self._port = port
        self._baud = baud
        self._serial: serial.Serial | None = None
        self._lock = threading.Lock()
        self._running = False

    def start(self, callback) -> threading.Thread:
        """Reconnect-Loop in Daemon-Thread starten.

        Verbindet sofort und reconnectet automatisch bei Verbindungsverlust.
        """
        self._running = True
        thread = threading.Thread(
            target=self._run_with_reconnect,
            args=(callback,),
            daemon=True,
            name="serial-reconnect",
        )
        thread.start()
        return thread

    def send(self, json_str: str):
        """JSON-Befehl an ESP32 senden (thread-safe).

        Verwirft den Befehl wenn Serial nicht verbunden ist — kein Puffern,
        da LED-Befehle zeitkritisch und nach einem Reconnect veraltet wären.
        """
        with self._lock:
            if not self._serial or not self._serial.is_open:
                logger.warning("Serial nicht offen — Befehl verworfen: %s", json_str)
                return

            line = json_str if json_str.endswith("\n") else json_str + "\n"
            self._serial.write(line.encode("utf-8"))
            logger.debug("Gesendet: %s", line.strip())

    def disconnect(self):
        """Reconnect-Loop und Serial-Verbindung sauber stoppen."""
        self._running = False
        with self._lock:
            if self._serial and self._serial.is_open:
                self._serial.close()
        logger.info("Serial-Verbindung getrennt")

    def _run_with_reconnect(self, callback):
        """Hauptschleife: verbinden, lesen, bei Fehler mit Backoff neu verbinden."""
        delay = SERIAL_RECONNECT_DELAY_INITIAL

        while self._running:
            try:
                self._open_connection()
                delay = SERIAL_RECONNECT_DELAY_INITIAL
                self._listen_loop(callback)
            except serial.SerialException as exc:
                logger.error("Serial-Fehler: %s", exc)

            if self._running:
                logger.info("Reconnect in %.1fs...", delay)
                time.sleep(delay)
                delay = min(delay * 2, SERIAL_RECONNECT_DELAY_MAX)

    def _open_connection(self):
        """Serielle Verbindung öffnen — wirft SerialException bei Fehler."""
        new_serial = serial.Serial(self._port, self._baud, timeout=1)
        with self._lock:
            self._serial = new_serial
        logger.info("Verbunden mit ESP32 auf %s @ %d Baud", self._port, self._baud)

    def _listen_loop(self, callback):
        """Zeilen vom ESP32 lesen bis ein Fehler auftritt oder _running=False."""
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
                raise
            except UnicodeDecodeError as exc:
                logger.warning("Ungültige Zeichenkodierung vom ESP32: %s", exc)

    def _validate_and_dispatch(self, line: str, callback):
        """JSON-Validierung vor Weitergabe an Callback."""
        try:
            json.loads(line)
        except json.JSONDecodeError:
            logger.warning("Ungültiges JSON vom ESP32: %s", line)
            return

        callback(line)
