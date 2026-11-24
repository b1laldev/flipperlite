"""Simulyatsiya qilingan router.

Haqiqiy WiFi adapteri yoki tarmog'i ishlatilmaydi: bu obyekt faqat
WPS tugmasi bosilganda vaqt chegaralangan oyna ochib, soxta ma'lumot
qaytaradi.(container ichida xavfsiz ishlash uchun).
"""

import secrets
from dataclasses import dataclass


@dataclass(frozen=True)
class Credentials:
    """Router "admin panelida" saqlangan soxta tarmoq ma'lumotlari."""

    ssid: str
    password: str


class SimulatedRouter:
    def __init__(self, ssid: str, password: str, wps_window: float = 6.0):
        self.credentials = Credentials(ssid=ssid, password=password)
        self.wps_window = wps_window
        self._pin = None
        self._wps_until = 0.0

    @property
    def wps_open(self) -> bool:
        """WPS oynasi hozir ochiqmi?"""
        return self._pin is not None

    def push_wps(self, now: float) -> dict:
        """Routerdagi WPS tugmasini bosish (PBC rejimi)."""
        self._pin = "%06d" % secrets.randbelow(1_000_000)
        self._wps_until = now + self.wps_window
        return {"pin": self._pin, "expires_at": self._wps_until}

    def exchange(self, now: float):
        """Qurilma WPS orqali ma'lumot so'raganda chaqiriladi.

        Qaytaradi: (natija, xato). Faqat oyna ochiq va vaqt tugamagan
        bo'lgandagina ma'lumot beriladi.
        """
        if self._pin is None:
            return None, "Routerda WPS tugmasi bosilmagan"
        if now > self._wps_until:
            self._pin = None
            self._wps_until = 0.0
            return None, "WPS vaqti tugagan"

        pin = self._pin
        self._pin = None
        self._wps_until = 0.0
        return {"pin": pin, "credentials": self.credentials}, None
