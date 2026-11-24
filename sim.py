#!/usr/bin/env python3
"""Terminal simulyatori: qurilma tugmalari va displeyi."""

import json
import time
from pathlib import Path

from device import SimulatedRouter, WifiBadge

BASE = Path(__file__).resolve().parent


def load_config() -> dict:
    with (BASE / "config.json").open(encoding="utf-8") as file:
        return json.load(file)


def new_badge() -> WifiBadge:
    config = load_config()
    router = SimulatedRouter(
        ssid=config["ssid"],
        password=config["password"],
        wps_window=config.get("wps_window", 6.0),
    )
    return WifiBadge(router)


def draw(badge: WifiBadge) -> None:
    snapshot = badge.snapshot()
    print("\n" + badge.display.render_ascii())
    led = "●" if snapshot["wps_open"] else "○"
    print("  holat: {:11s}  WPS LED: {}".format(snapshot["state"], led))
    if snapshot["message"]:
        print("  xabar: " + snapshot["message"])


def auto_tick(badge: WifiBadge, count: int = 8, delay: float = 0.45) -> None:
    for _ in range(count):
        time.sleep(delay)
        draw(badge.tick())


def main() -> None:
    badge = new_badge()
    print("=== WiFi BADGE — simulyator ===")
    print("Haqiqiy tarmoqlar ishlatilmaydi, faqat LAB-WIFI-TEST soxta routeri.\n")
    draw(badge.tick())
    draw(badge.tick())  # BOOT -> IDLE

    while True:
        print("\n[1] WPS tugmasi   [2] Reset   [r] yangilash   [0] chiqish")
        command = input("tanlang: ").strip().lower()

        if command == "1":
            if badge.press_wps():
                draw(badge.snapshot())
                auto_tick(badge)
            else:
                print("  qurilma band, avval jarayon tugasin.")
        elif command == "2":
            badge.press_reset()
            draw(badge.snapshot())
        elif command == "r":
            draw(badge.tick())
        elif command == "0":
            print("Xayr!")
            break
        else:
            print("  noto'g'ri buyruq.")


if __name__ == "__main__":
    main()
