# Protokol

Barcha JSON maydonlari kichik harflar bilan. Qurilma port **80**, Go bridge port **8080**.

## Qurilma API (ESP32)

| Method | Yo'l | Tavsif |
| --- | --- | --- |
| GET | `/api/status` | Nomi, versiya, faol app, WiFi rejimi/SSID/IP, uptime, heap, displey o'lchami |
| GET | `/api/apps` | `{"apps":[{"id":"menu","name":"Apps"}, ...]}` |
| GET | `/api/display` | `{"width":128,"height":64,"format":"1bit-row-major","data":"<base64>"}` — 1024 bayt framebuffer |
| POST | `/api/input` | `{"key":"ok","action":"short"}` |
| POST | `/api/launch` | `{"id":"badge"}` |
| POST | `/api/wifi` | `{"ssid":"...","password":"..."}` (faqat o'z tarmog'ingiz!) |
| GET | `/api/events` | SSE: `event: flite\ndata: <event JSON>\n\n` |

`key`: `up|down|left|right|ok|back`. `action`: `short|long|repeat`.

Event JSON misoli:

```json
{"type":"input","key":"ok","action":"short","ts_ms":12345}
```

`/api/display` dagi `data` — `firmware/main/flite/flite_canvas.c` dagi gorizontal bit joylashuvi:
`index = (y / 8) * 128 + x`, bit `y % 8`. Bu format keyinroq telefonda displeyni qayta chizish uchun
ishlatiladi.

## Bridge API (Go)

| Method | Yo'l | Tavsif |
| --- | --- | --- |
| GET | `/health` | Bridge holati |
| GET | `/api/v1/status` | Qurilma statusi (proxy) |
| GET | `/api/v1/apps` | App ro'yxati |
| GET | `/api/v1/display` | Framebuffer (proxy) |
| POST | `/api/v1/input` | Tugma voqeasi (tekshiriladi) |
| POST | `/api/v1/launch` | App ishga tushirish |
| POST | `/api/v1/wifi` | STA ulanish so'rovi |
| GET | `/api/v1/events` | Qurilma SSE oqimini telefonga uzatadi |

Noto'g'ri `key`/`action` yoki bo'sh JSON → **400**. Qurilma javob bermasa → **502**.
Barcha javoblarda `Access-Control-Allow-Origin: *` (mahalliy tarmoq uchun).

## Misollar

```bash
curl http://192.168.4.1/api/status
curl -X POST http://localhost:8080/api/v1/input \
  -H 'Content-Type: application/json' -d '{"key":"down","action":"short"}'
curl -N http://localhost:8080/api/v1/events
```
