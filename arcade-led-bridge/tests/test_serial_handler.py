import json
import threading
from unittest.mock import MagicMock, patch, call

import pytest

from serial_handler import SerialHandler


class TestSerialHandlerSend:
    def setup_method(self):
        self.handler = SerialHandler("/dev/ttyUSB0", 115200)

    def test_send_appends_newline(self):
        mock_serial = MagicMock()
        mock_serial.is_open = True
        self.handler._serial = mock_serial

        self.handler.send('{"cmd":"effect"}')

        mock_serial.write.assert_called_once_with(b'{"cmd":"effect"}\n')

    def test_send_does_not_duplicate_newline(self):
        mock_serial = MagicMock()
        mock_serial.is_open = True
        self.handler._serial = mock_serial

        self.handler.send('{"cmd":"effect"}\n')

        mock_serial.write.assert_called_once_with(b'{"cmd":"effect"}\n')

    def test_send_discards_when_serial_none(self):
        self.handler._serial = None
        # Kein Exception erwartet
        self.handler.send('{"cmd":"effect"}')

    def test_send_discards_when_serial_closed(self):
        mock_serial = MagicMock()
        mock_serial.is_open = False
        self.handler._serial = mock_serial

        self.handler.send('{"cmd":"effect"}')

        mock_serial.write.assert_not_called()

    def test_send_is_thread_safe(self):
        mock_serial = MagicMock()
        mock_serial.is_open = True
        self.handler._serial = mock_serial

        threads = [
            threading.Thread(target=self.handler.send, args=(f'{{"cmd":"effect","i":{i}}}',))
            for i in range(20)
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        assert mock_serial.write.call_count == 20


class TestSerialHandlerValidateAndDispatch:
    def setup_method(self):
        self.handler = SerialHandler("/dev/ttyUSB0", 115200)

    def test_valid_json_calls_callback(self):
        callback = MagicMock()
        line = '{"status":"ready","version":"1.0.0"}'

        self.handler._validate_and_dispatch(line, callback)

        callback.assert_called_once_with(line)

    def test_invalid_json_does_not_call_callback(self):
        callback = MagicMock()

        self.handler._validate_and_dispatch("configsip: 0, SPIWP:0xee", callback)

        callback.assert_not_called()

    def test_empty_json_object_calls_callback(self):
        callback = MagicMock()

        self.handler._validate_and_dispatch("{}", callback)

        callback.assert_called_once_with("{}")

    def test_esp32_error_response_calls_callback(self):
        callback = MagicMock()
        line = '{"status":"error","code":3,"msg":"unknown effect type"}'

        self.handler._validate_and_dispatch(line, callback)

        callback.assert_called_once_with(line)


class TestSerialHandlerDisconnect:
    def test_disconnect_closes_serial(self):
        handler = SerialHandler("/dev/ttyUSB0", 115200)
        mock_serial = MagicMock()
        mock_serial.is_open = True
        handler._serial = mock_serial
        handler._running = True

        handler.disconnect()

        assert handler._running is False
        mock_serial.close.assert_called_once()

    def test_disconnect_when_already_closed(self):
        handler = SerialHandler("/dev/ttyUSB0", 115200)
        mock_serial = MagicMock()
        mock_serial.is_open = False
        handler._serial = mock_serial

        # Kein Exception erwartet
        handler.disconnect()

        mock_serial.close.assert_not_called()
