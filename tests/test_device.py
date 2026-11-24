"""Qurilma mantig'i uchun testlar (haqiqiy vaqt kutmaydi)."""

import unittest

from device import SimulatedRouter, WifiBadge


class FakeClock:
    def __init__(self, now=0.0):
        self.now = now

    def __call__(self):
        return self.now

    def advance(self, seconds):
        self.now += seconds


def make_badge(clock=None):
    clock = clock or FakeClock()
    router = SimulatedRouter("LAB-WIFI-TEST", "TestNet-2026!demo", wps_window=6.0)
    return WifiBadge(router, clock=clock), router, clock


def to_showing(badge):
    badge.tick()
    badge.tick()  # BOOT -> IDLE
    badge.press_wps()
    for _ in range(5):
        badge.tick()


class BadgeTests(unittest.TestCase):
    def test_boot_then_idle(self):
        badge, _, _ = make_badge()
        self.assertEqual(badge.state.value, "BOOT")
        badge.tick()
        badge.tick()
        self.assertEqual(badge.state.value, "IDLE")
        self.assertTrue(any("WPS tugmasini bosing" in line for line in badge.display.lines))

    def test_wps_flow_shows_password(self):
        badge, _, _ = make_badge()
        to_showing(badge)
        self.assertEqual(badge.state.value, "SHOWING")
        snapshot = badge.snapshot()
        self.assertEqual(snapshot["ssid"], "LAB-WIFI-TEST")
        self.assertEqual(snapshot["password"], "TestNet-2026!demo")
        self.assertTrue(any("PASS:" in line for line in snapshot["lines"]))

    def test_password_hidden_before_showing(self):
        badge, _, _ = make_badge()
        badge.tick()
        badge.tick()
        badge.press_wps()
        self.assertIsNone(badge.snapshot()["password"])
        badge.tick()
        self.assertIsNone(badge.snapshot()["password"])

    def test_expired_wps_window_gives_error(self):
        badge, _, clock = make_badge()
        badge.tick()
        badge.tick()
        badge.press_wps()
        clock.advance(10.0)
        for _ in range(5):
            badge.tick()
        self.assertEqual(badge.state.value, "ERROR")
        self.assertIn("vaqti tugagan", badge.snapshot()["message"])

    def test_reset_clears_display_and_password(self):
        badge, _, _ = make_badge()
        to_showing(badge)
        badge.press_reset()
        snapshot = badge.snapshot()
        self.assertEqual(snapshot["state"], "IDLE")
        self.assertIsNone(snapshot["password"])
        self.assertFalse(any("PASS:" in line for line in snapshot["lines"]))

    def test_second_press_ignored_while_busy(self):
        badge, _, _ = make_badge()
        badge.tick()
        badge.tick()
        self.assertTrue(badge.press_wps())
        self.assertFalse(badge.press_wps())

    def test_display_truncates_long_lines(self):
        from device import EinkDisplay

        display = EinkDisplay(columns=8, rows=2)
        display.show(["123456789012", "abc"])
        self.assertEqual(display.lines, ["12345678", "abc     "])


if __name__ == "__main__":
    unittest.main()
