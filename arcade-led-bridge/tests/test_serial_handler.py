import json
import threading
from unittest.mock import MagicMock, patch

import pytest
import serial

from config import SERIAL_RECONNECT_DELAY_INITIAL
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


class TestSerialHandlerReconnect:
    def setup_method(self):
        self.handler = SerialHandler("/dev/ttyUSB0", 115200)

    def test_start_spawns_daemon_thread(self):
        """start() gibt einen laufenden Daemon-Thread zurück."""
        with patch.object(self.handler, "_run_with_reconnect"):
            thread = self.handler.start(MagicMock())
            self.handler.disconnect()
            thread.join(timeout=1.0)

        assert thread.daemon is True

    def test_reconnect_retries_after_serial_exception(self):
        """Reconnect-Loop versucht nach einem Fehler automatisch neu zu verbinden."""
        attempt_count = 0

        def flaky_serial(*args, **kwargs):
            nonlocal attempt_count
            attempt_count += 1
            if attempt_count >= 3:
                self.handler._running = False
            raise serial.SerialException("port unavailable")

        with patch("serial_handler.serial.Serial", side_effect=flaky_serial):
            with patch("serial_handler.time.sleep"):
                self.handler._running = True
                self.handler._run_with_reconnect(MagicMock())

        assert attempt_count == 3

    def test_reconnect_delay_increases_exponentially(self):
        """Delay verdoppelt sich nach jedem Verbindungsfehler."""
        attempt_count = 0
        sleep_calls = []

        def failing_serial(*args, **kwargs):
            nonlocal attempt_count
            attempt_count += 1
            if attempt_count >= 4:
                self.handler._running = False
            raise serial.SerialException("port unavailable")

        with patch("serial_handler.serial.Serial", side_effect=failing_serial):
            with patch("serial_handler.time.sleep", side_effect=lambda d: sleep_calls.append(d)):
                self.handler._running = True
                self.handler._run_with_reconnect(MagicMock())

        assert sleep_calls[0] == SERIAL_RECONNECT_DELAY_INITIAL
        assert sleep_calls[1] == SERIAL_RECONNECT_DELAY_INITIAL * 2
        assert sleep_calls[2] == SERIAL_RECONNECT_DELAY_INITIAL * 4

    def test_reconnect_delay_resets_after_successful_connect(self):
        """Delay wird nach erfolgreicher Verbindung auf den Initialwert zurückgesetzt."""
        attempt_count = 0
        sleep_calls = []

        mock_serial = MagicMock()
        mock_serial.is_open = True
        # Erfolgreiche Verbindung bricht sofort mit SerialException ab (simuliert Disconnect)
        mock_serial.readline.side_effect = serial.SerialException("link lost")

        def serial_factory(*args, **kwargs):
            nonlocal attempt_count
            attempt_count += 1
            if attempt_count == 1:
                raise serial.SerialException("not ready yet")
            if attempt_count >= 3:
                self.handler._running = False
            return mock_serial

        with patch("serial_handler.serial.Serial", side_effect=serial_factory):
            with patch("serial_handler.time.sleep", side_effect=lambda d: sleep_calls.append(d)):
                self.handler._running = True
                self.handler._run_with_reconnect(MagicMock())

        # Erster Fehler: Delay = initial (1s)
        assert sleep_calls[0] == SERIAL_RECONNECT_DELAY_INITIAL
        # Nach erfolgreicher Verbindung + erneutem Fehler: Delay wieder = initial (kein Verdoppeln)
        assert sleep_calls[1] == SERIAL_RECONNECT_DELAY_INITIAL

    def test_reconnect_delay_capped_at_maximum(self):
        """Delay überschreitet nie den konfigurierten Maximalwert."""
        from config import SERIAL_RECONNECT_DELAY_MAX
        attempt_count = 0
        sleep_calls = []

        def failing_serial(*args, **kwargs):
            nonlocal attempt_count
            attempt_count += 1
            if attempt_count >= 10:
                self.handler._running = False
            raise serial.SerialException("port unavailable")

        with patch("serial_handler.serial.Serial", side_effect=failing_serial):
            with patch("serial_handler.time.sleep", side_effect=lambda d: sleep_calls.append(d)):
                self.handler._running = True
                self.handler._run_with_reconnect(MagicMock())

        assert all(d <= SERIAL_RECONNECT_DELAY_MAX for d in sleep_calls)

    def test_no_reconnect_after_disconnect(self):
        """Nach disconnect() wird keine neue Verbindung aufgebaut."""
        attempt_count = 0

        def failing_serial(*args, **kwargs):
            nonlocal attempt_count
            attempt_count += 1
            self.handler._running = False  # sofort stoppen
            raise serial.SerialException("port unavailable")

        with patch("serial_handler.serial.Serial", side_effect=failing_serial):
            with patch("serial_handler.time.sleep"):
                self.handler._running = True
                self.handler._run_with_reconnect(MagicMock())

        assert attempt_count == 1
