import asyncio
import json
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

from ws_server import WebSocketServer


def make_server():
    serial = MagicMock()
    serial.send = MagicMock()
    return WebSocketServer("localhost", 8765, serial), serial


def make_websocket():
    ws = MagicMock()
    ws.send = AsyncMock()
    ws.remote_address = ("127.0.0.1", 12345)
    return ws


class TestProcessMessage:
    @pytest.mark.asyncio
    async def test_valid_effect_command_forwarded_to_serial(self):
        server, serial = make_server()
        ws = make_websocket()
        message = json.dumps({
            "cmd": "effect", "chain": "A", "type": "chase",
            "segment": 0, "color": {"r": 255, "g": 0, "b": 0},
            "speed": 50, "repeat": 3, "priority": 2,
        })

        await server._process_message(message, ws)

        serial.send.assert_called_once_with(message)
        ws.send.assert_not_called()

    @pytest.mark.asyncio
    async def test_valid_attract_command_forwarded_to_serial(self):
        server, serial = make_server()
        ws = make_websocket()
        message = '{"cmd":"attract","state":"pause"}'

        await server._process_message(message, ws)

        serial.send.assert_called_once_with(message)

    @pytest.mark.asyncio
    async def test_invalid_json_returns_error(self):
        server, serial = make_server()
        ws = make_websocket()

        await server._process_message("kein json{{{", ws)

        serial.send.assert_not_called()
        response = json.loads(ws.send.call_args[0][0])
        assert response["status"] == "error"
        assert response["code"] == 1

    @pytest.mark.asyncio
    async def test_missing_cmd_returns_error(self):
        server, serial = make_server()
        ws = make_websocket()

        await server._process_message('{"type":"chase"}', ws)

        serial.send.assert_not_called()
        response = json.loads(ws.send.call_args[0][0])
        assert response["status"] == "error"
        assert response["code"] == 5

    @pytest.mark.asyncio
    async def test_unknown_cmd_returns_error(self):
        server, serial = make_server()
        ws = make_websocket()

        await server._process_message('{"cmd":"reboot"}', ws)

        serial.send.assert_not_called()
        response = json.loads(ws.send.call_args[0][0])
        assert response["status"] == "error"
        assert response["code"] == 5
        assert "reboot" in response["msg"]

    @pytest.mark.asyncio
    async def test_error_response_contains_msg_field(self):
        server, serial = make_server()
        ws = make_websocket()

        await server._process_message("invalid", ws)

        response = json.loads(ws.send.call_args[0][0])
        assert "msg" in response


class TestSendError:
    @pytest.mark.asyncio
    async def test_send_error_format(self):
        ws = make_websocket()

        await WebSocketServer._send_error(ws, code=3, msg="unknown effect type")

        response = json.loads(ws.send.call_args[0][0])
        assert response == {"status": "error", "code": 3, "msg": "unknown effect type"}

    @pytest.mark.asyncio
    async def test_send_error_code_1(self):
        ws = make_websocket()

        await WebSocketServer._send_error(ws, code=1, msg="invalid json")

        response = json.loads(ws.send.call_args[0][0])
        assert response["code"] == 1


class TestClientManagement:
    @pytest.mark.asyncio
    async def test_client_added_on_connect(self):
        server, serial = make_server()
        ws = make_websocket()

        async def single_message(_self):
            yield '{"cmd":"effect","chain":"A","type":"fill","segment":0,"color":{"r":0,"g":0,"b":0}}'

        ws.__aiter__ = single_message

        await server._handle_client(ws)

        # Client nach Trennung aus Set entfernt
        assert ws not in server._clients

    @pytest.mark.asyncio
    async def test_client_removed_on_disconnect(self):
        import websockets.exceptions
        server, serial = make_server()
        ws = make_websocket()

        async def raise_disconnect(_self):
            raise websockets.exceptions.ConnectionClosed(None, None)
            yield  # macht es zum async generator

        ws.__aiter__ = raise_disconnect

        await server._handle_client(ws)

        assert ws not in server._clients
