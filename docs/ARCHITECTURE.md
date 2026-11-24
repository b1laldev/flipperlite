# Arxitektura

## Umumiy ko'rinish

```
┌──────────────┐   HTTP/JSON + SSE   ┌──────────────┐   REST/SSE   ┌──────────────┐
│  ESP32-S3/C3 │ ──────────────────▶ │  Go bridge   │ ───────────▶ │  Android app │
│  firmware    │ ◀────────────────── │  :8080       │ ◀─────────── │  (Compose)   │
└──────────────┘   tugma/app buyruq  └──────────────┘   status     └──────────────┘
        │
        ├─ hw/     SSD1306 I2C displey, 6 tugma (short/long/repeat)
        ├─ net/    AP+STA WiFi, httpd (port 80), SSE event stream
        └─ flite/  event bus, canvas, scene, manager, protokol
```

## Firmware qatlamlari

1. **Event bus** (`flite_event_bus.c`) — FreeRTOS queue asosida pub/sub. 4 tagacha obunachi
   (asosiy UI taski + SSE ulanishlari). `FliteEvent` turlari: `input`, `tick`, `net`, `app`, `custom`.
2. **Input** (`flite_input_esp.c`) — GPIO ANYEDGE ISR → raw queue → 50 ms task:
   - 600 ms dan qisqa → `short`;
   - 600 ms dan uzoq → `long`;
   - uzoq bosib turganda har 250 ms → `repeat`.
3. **Canvas** (`flite_canvas.c`) — 128×64, 1 bit/px, gorizontal bayt joylashuvi;
   3×5 shrift (2× masshtab), to'rtburchak, chiziq, header va battery indikatori.
   Sof C, ESP-IDF ga bog'liq emas — host testlarida kompilyatsiya qilinadi.
4. **Scene/App** (`flite_scene.c`, `flite_apps.c`) — Flipper'dagi `SceneManager` naqshining
   soddalashtirilgan versiyasi: har appning `on_enter/on_event/on_render/on_exit` jadvallari,
   `BACK` tugmasi birinchi scene'ga qaytadi. App ro'yxati statik massiv (dynamic loader yo'q).
5. **Manager** (`flite_manager.c`) — faol app, qurilma ma'lumotlari (mode/SSID/IP/uptime/heap),
   `dirty` bayroqi. Faqat o'zgarganda displey yangilanadi (taxminan 10 Hz).
6. **Display** (`flite_display_esp.c`) — SSD1306 I2C, framebuffer → 8 sahifaga aylantirib
   bitta I2C transaktsiyasida yuboriladi.
7. **HTTP** (`flite_http.c`) — REST + SSE. SSE orqali tugma voqealari telefonga real vaqtda keladi.

## Ma'lumot oqimi (tugma bosilganda)

```
GPIO ISR → raw queue → input task → event bus
                                      ├─→ main task → manager.dispatch → scene.on_event → render → OLED
                                      └─→ SSE subscriber → Go bridge → Android (event log)
```

Telefondan buyruq kelganda yo'l teskari: Android → `POST /api/v1/input` → Go →
`POST /api/input` → event bus → xuddi shu UI yo'li.

## Nega to'liq klon yo'q?

Asl firmware STM32WB55, ikki yadroli 1 MB RAM + 4 MB flash, Furi RTOS, fbt/SCons,
ko'p servislar (SubGHz, NFC, USB HID, grafikalar, dinamik FAP yuklagich) ustiga qurilgan.
Arzon ESP32-S3/C3 da bularning ko'pchiligi na kerak, na sig'adi. Shuning uchun biz
**foydali arxitektura naqshlarini** oldik, qolganini ataylab tashladik
(batafsil [PORTING.md](PORTING.md)).
