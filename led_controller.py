import asyncio
import time
import websockets
import json
import threading
import logging

logger = logging.getLogger(__name__)

# Farb-Paletten für die Demo
DEMO_COLORS = [
    (255, 0,   0),    # Rot
    (0,   255, 0),    # Grün
    (0,   0,   255),  # Blau
    (255, 165, 0),    # Orange
    (128, 0,   128),  # Lila
    (0,   255, 255),  # Cyan
    (255, 255, 0),    # Gelb
]

DEMO_EFFECTS = ["fill", "blink", "chase", "pulse", "wipe", "sparkle"]


class LedController:
    def __init__(self, url: str = "ws://localhost:8765"):
        self._url = url
        self._ws = None
        self._loop = asyncio.new_event_loop()
        self._thread = threading.Thread(target=self._run_loop, daemon=True)
        self._thread.start()

    def _run_loop(self):
        self._loop.run_until_complete(self._connect())

    async def _connect(self):
        try:
            async with websockets.connect(self._url) as ws:
                self._ws = ws
                logger.info("LED Bridge verbunden")
                await asyncio.Future()  # Verbindung offen halten
        except Exception as e:
            logger.warning(f"LED Bridge nicht erreichbar: {e}")

    def send_effect(
        self,
        chain: str,
        effect_type: str,
        segment: int,
        r: int,
        g: int,
        b: int,
        speed: int = 50,
        repeat: int = 1,
        priority: int = 2,
    ):
        payload = {
            "cmd": "effect",
            "chain": chain,
            "type": effect_type,
            "segment": segment,
            "color": {"r": r, "g": g, "b": b},
            "speed": speed,
            "repeat": repeat,
            "priority": priority,
        }
        asyncio.run_coroutine_threadsafe(
            self._send(json.dumps(payload)), self._loop
        )

    def attract_pause(self):
        asyncio.run_coroutine_threadsafe(
            self._send('{"cmd":"attract","state":"pause"}'), self._loop
        )

    def attract_resume(self):
        asyncio.run_coroutine_threadsafe(
            self._send('{"cmd":"attract","state":"resume"}'), self._loop
        )

    async def _send(self, message: str):
        if self._ws:
            await self._ws.send(message)

    def is_connected(self) -> bool:
        return self._ws is not None


def run_demo():
    """
    Testet die WebSocket-Verbindung zur Bridge und alle LED-Segmente beider Ketten.
    Durchläuft jeden Effekttyp mit verschiedenen Farben auf allen Segmenten.
    """
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s  %(levelname)-8s  %(message)s",
        datefmt="%H:%M:%S",
    )

    print("=== Arcade LED Demo ===")
    print("Verbinde mit Bridge auf ws://localhost:8765 ...")

    controller = LedController()
    time.sleep(1.0)  # Verbindungsaufbau abwarten

    if not controller.is_connected():
        print("FEHLER: Keine Verbindung zur Bridge. Läuft bridge.py?")
        return

    print("Verbindung OK. Starte Demo...\n")

    # Attract-Mode pausieren
    controller.attract_pause()
    time.sleep(0.5)

    # --- Kette A: gesamte Kette (segment 99) ---
    # Einzelne Segmente beginnen ab Index 42 — außerhalb der aktuell
    # angeschlossenen 35 LEDs. Segment 99 adressiert die volle Kette,
    # FastLED schreibt auf alle vorhandenen LEDs.
    print("--- Kette A (Spieler-nah): gesamte Kette (segment 99) ---")
    for i, (r, g, b) in enumerate(DEMO_COLORS):
        print(f"  chase ({r},{g},{b})")
        controller.send_effect("A", "chase", 99, r, g, b, speed=40, repeat=3, priority=2)
        time.sleep(1.5)

    # --- Kette B: alle 3 Segmente durchlaufen ---
    print("\n--- Kette B (Ambient): Segmente 0–2 ---")
    segment_names_b = ["Seite links", "Boden", "Seite rechts"]
    for seg_id, seg_name in enumerate(segment_names_b):
        r, g, b = DEMO_COLORS[(seg_id + 3) % len(DEMO_COLORS)]
        print(f"  Seg {seg_id} [{seg_name}] → wipe ({r},{g},{b})")
        controller.send_effect("B", "wipe", seg_id, r, g, b, speed=30, repeat=2, priority=2)
        time.sleep(1.5)

    # --- Alle Effekttypen auf gesamter Kette A (segmentId 99) ---
    print("\n--- Alle Effekte auf Kette A gesamt (segment 99) ---")
    for i, effect in enumerate(DEMO_EFFECTS):
        r, g, b = DEMO_COLORS[i % len(DEMO_COLORS)]
        print(f"  Effekt: {effect:10s} → ({r},{g},{b})")
        # controller.send_effect("A", effect, 99, r, g, b, speed=50, repeat=2, priority=2)
        controller.send_effect("A", effect, 0, r, g, b, speed=50, repeat=2, priority=2)
        time.sleep(2.0)

    # --- Rainbow auf Kette B gesamt ---
    print("\n--- Rainbow auf Kette B gesamt ---")
    controller.send_effect("B", "rainbow", 99, 0, 0, 0, speed=20, repeat=3, priority=2)
    time.sleep(4.0)

    # Attract-Mode wieder aktivieren
    print("\nDemo abgeschlossen. Attract-Mode wird reaktiviert.")
    controller.attract_resume()


if __name__ == "__main__":
    run_demo()
