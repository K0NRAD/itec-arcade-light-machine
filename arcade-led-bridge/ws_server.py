import asyncio
import json
import logging

import websockets
from websockets.server import WebSocketServerProtocol

from serial_handler import SerialHandler

logger = logging.getLogger(__name__)

# Erlaubte Befehle — verhindert unbekannte Kommandos am ESP32
ALLOWED_COMMANDS = {"effect", "attract"}


class WebSocketServer:
    def __init__(self, host: str, port: int, serial_handler: SerialHandler):
        self._host = host
        self._port = port
        self._serial = serial_handler
        self._clients: set[WebSocketServerProtocol] = set()

    async def start(self):
        """WebSocket-Server starten und bis SIGINT laufen."""
        async with websockets.serve(self._handle_client, self._host, self._port):
            logger.info("Bridge läuft auf ws://%s:%d", self._host, self._port)
            await asyncio.Future()

    async def _handle_client(self, websocket: WebSocketServerProtocol):
        """Einzelne Client-Verbindung verwalten."""
        self._clients.add(websocket)
        logger.info("Spiel verbunden: %s", websocket.remote_address)

        try:
            async for message in websocket:
                await self._process_message(message, websocket)
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            self._clients.discard(websocket)
            logger.info("Spiel getrennt: %s", websocket.remote_address)

    async def _process_message(self, message: str, websocket: WebSocketServerProtocol):
        """Nachricht validieren und an ESP32 weiterleiten."""
        try:
            data = json.loads(message)
        except json.JSONDecodeError:
            await self._send_error(websocket, code=1, msg="invalid json")
            return

        command = data.get("cmd")
        if not command:
            await self._send_error(websocket, code=5, msg="missing cmd field")
            return

        if command not in ALLOWED_COMMANDS:
            await self._send_error(websocket, code=5, msg=f"unknown cmd: {command}")
            return

        self._serial.send(message)
        logger.debug("Weitergeleitet an ESP32: %s", message.strip())

    @staticmethod
    async def _send_error(
        websocket: WebSocketServerProtocol, code: int, msg: str
    ):
        """Fehlerantwort an den aufrufenden Client senden."""
        response = json.dumps({"status": "error", "code": code, "msg": msg})
        await websocket.send(response)
        logger.warning("Fehler an Client gesendet [%d]: %s", code, msg)
