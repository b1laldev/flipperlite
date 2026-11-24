# WiFi Badge (simulyator)

Qiziqish uchun yasalgan kichik "hardware" prototipi: WPS tugmasi bosilganda
tarmoq ma'lumotlarini displeyga chiqaradigan qurilma **mantig'i**.

> **E'tibor:** container ichida haqiqiy WiFi adapteri yo'q.(Ushbu loyiha hech qanday haqiqiy tarmoqni skanerlamaydi, ochmaydi yoki buzmaydi.
> `config.json` dagi ma'lumotlar to'liq soxta (`LAB-WIFI-TEST`).
> Faqat o'zingizga tegishli qurilmalar bilan tajriba qiling.

## Ishga tushirish

Qo'shimcha paketlar kerak emas, faqat Python 3.9+:

```bash
# terminal simulyatori
python3 sim.py

# veb-dashboard (faqat localhost): http://127.0.0.1:8765
python3 web.py

# testlar
python3 -m unittest discover -s tests -v
```

`sim.py` da tugmalar: `1` — WPS, `2` — Reset, `r` — bir sikl oldinga,
`0` — chiqish. WPS bosilgandan keyin qurilma bir necha bosqichdan o'tib,
ekranda SSID, parol va PIN ko'rsatadi. Agar WPS oynasi (6 s) o'tib ketsa,
ekranda xato chiqadi.

## Tuzilma

| Fayl | Vazifasi |
| --- | --- |
| `device/router.py` | Soxta router: WPS oynasi, vaqt chegarasi, ma'lumot qaytarish |
| `device/device.py` | Holat mashinasi: `BOOT → IDLE → WPS_WAIT → EXCHANGING → SHOWING / ERROR` |
| `device/display.py` | Displey buferi va terminalda chizish |
| `config.json` | Soxta tarmoq ma'lumotlari |
| `sim.py` | Terminal orqali boshqarish |
| `web.py` | Tugmali qurilma maketi (stdlib HTTP server) |
| `tests/` | Holat mashinasi testlari |

## MF680 (haqiqiy qurilma) — veb/API darajasida tekshirish

`tools/` papkasi firmwarega tegmaydi: faqat HTTP so'rovlari yuboradi.

```bash
python3 tools/recon.py --base http://192.168.100.1 --user admin --pass admin
python3 tools/sms_client.py endpoints          # topilgan nomzodlar
python3 tools/sms_client.py unhide             # CSS bilan yashirilgan bo'limlarni ochish
python3 tools/sms_client.py list --url '/goform/...'   # aniq endpoint bilan
```

`recon.py` avval interfeys/manzilni tekshiradi, keyin bosh sahifa + barcha JS/CSS
fayllarini `captures/mf680/` ga saqlaydi va `analysis.json` yaratadi
(`goform`, `sms`, `read_sms`, `display:none`, USSD va h.k. bo'yicha).
`sms_client.py` hech qanday endpointni taxmin qilmaydi — faqat recon topgan
URL bilan ishlaydi.

**Eslatma:** 2026-11-23 holatida ish muhitida qurilma ko'rinmadi
(USB tarmoq interfeysi yo'q, `192.168.100.1` timeout). Shu sabab endpointlar
tasdiqlanmagan — ularni qurilma ulangandan keyingina recon aniqlaydi.

## Haqiqiy hardware ga o'tish

Mantig o'zgarishsiz qoladi, faqat ikki qism almashtiriladi:

- `SimulatedRouter` → ESP32 da `esp_wps` (PBC rejimi) bilan haqiqiy router
  tomonidan berilgan ma'lumot;
- `EinkDisplay` → SSD1306 (OLED) yoki GxEPD2 (e-ink) kutubxonasi;
- `tick()` → `loop()` ichida chaqiriladi; tugmalar `press_wps()` /
  `press_reset()` ga bog'lanadi.

Buning uchun faqat o'zingiz boshqaradigan router va uning WPS tugmasidan
foydalaning.
