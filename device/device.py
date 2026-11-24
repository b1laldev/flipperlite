"""Qurilmaning asosiy holat mashinasi (state machine).

Haqiqiy ESP32 loyihasida bu sinov o'zgarishsiz deyarli qoladi: faqat
SimulatedRouter o'rniga esp_wps, EinkDisplay o'rniga esa haqiqiy
displey kutubxonasi qo'yiladi.
"""

import time
from enum import Enum

from .display import EinkDisplay
from .router import SimulatedRouter


class State(Enum):
    BOOT = "BOOT"
    IDLE = "IDLE"
    WPS_WAIT = "WPS_WAIT"
    EXCHANGING = "EXCHANGING"
    SHOWING = "SHOWING"
    ERROR = "ERROR"


class WifiBadge:
    """WPS tugmasi bosilganda tarmoq ma'lumotlarini ekranga chiqaradigan qurilma."""

    EXCHANGE_STEPS = (
        "WPS: PIN olinyapti...",
        "WPS: kalit olinyapti...",
        "WPS: tasdiqlanyapti...",
    )

    def __init__(self, router: SimulatedRouter, display=None, clock=None):
        self.router = router
        self.display = display or EinkDisplay()
        self.clock = clock or time.monotonic
        self.state = State.BOOT
        self._phase = 0
        self._message = ""
        self._credentials = None
        self._last_pin = ""
        self._boot()

    # --- ichki yordamchilar -------------------------------------------------

    def _boot(self) -> None:
        self.state = State.BOOT
        self._phase = 0
        self.display.show(["  WiFi BADGE v0.1", "  ishga tushyapti..."])

    def _show_password(self) -> None:
        creds = self._credentials
        self.display.show(
            [
                "SSID: " + creds.ssid,
                "PASS: " + creds.password,
                "WPS PIN: " + self._last_pin,
                "",
                "  * ulanildi",
            ]
        )

    # --- tashqi tugmalar ----------------------------------------------------

    def press_wps(self) -> bool:
        """WPS tugmasi: routerda oyna ochib, almashinuvni boshlaydi."""
        if self.state in (State.WPS_WAIT, State.EXCHANGING):
            return False  # band holatda ikkinchi bosish e'tiborga olinmaydi

        self._message = ""
        self._credentials = None
        self.router.push_wps(self.clock())
        self.state = State.WPS_WAIT
        self._phase = 0
        self.display.show(["  WPS tugmasi bosildi", "  router javob kutyapti..."])
        return True

    def press_reset(self) -> bool:
        """Reset: ekranni va saqlangan ma'lumotlarni tozalaydi."""
        self._credentials = None
        self._message = ""
        self._last_pin = ""
        self.state = State.IDLE
        self._phase = 0
        self.display.show(["  Tayyor.", "  WPS tugmasini bosing."])
        return True

    # --- vaqt bosqichi ------------------------------------------------------

    def tick(self) -> dict:
        """Bitta ish sikli. Vaqtni faqat chaqiruvchi boshqaradi (test uchun qulay)."""
        if self.state == State.BOOT:
            self._phase += 1
            if self._phase >= 2:
                self.press_reset()

        elif self.state == State.WPS_WAIT:
            self._phase += 1
            if self._phase >= 2:
                self.state = State.EXCHANGING
                self._phase = 0

        elif self.state == State.EXCHANGING:
            step = min(self._phase, len(self.EXCHANGE_STEPS) - 1)
            self.display.show(["  " + self.EXCHANGE_STEPS[step], "  kutib turing..."])
            self._phase += 1

            if self._phase >= len(self.EXCHANGE_STEPS):
                result, error = self.router.exchange(self.clock())
                if error:
                    self.state = State.ERROR
                    self._message = error
                    self.display.show(["  XATO: " + error[:17], "  qaytadan bosing."])
                else:
                    self._credentials = result["credentials"]
                    self._last_pin = result["pin"]
                    self.state = State.SHOWING
                    self._show_password()

        return self.snapshot()

    def snapshot(self) -> dict:
        """CLI va veb-ui uchun qurilma holati."""
        return {
            "state": self.state.value,
            "lines": list(self.display.lines),
            "message": self._message,
            "ssid": self._credentials.ssid if self._credentials else None,
            "pin": self._last_pin or None,
            # Parol faqat SHOWING holatida "ko'rinadi"
            "password": (
                self._credentials.password
                if self.state == State.SHOWING and self._credentials
                else None
            ),
            "wps_open": self.router.wps_open,
        }
