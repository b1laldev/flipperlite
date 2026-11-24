# Tashqi ilovalar uchun API

Bridge orqali istalgan til/ilova qurilmani boshqara oladi: veb sahifa, Android, Node, Python,
Home Assistant va h.k. Barcha endpointlar `http://<bridge-host>:8080` ostida.

## Autentifikatsiya

`FLITE_API_TOKEN` bo'sh bo'lsa — hech qanday header kerak emas. Token qo'yilgan bo'lsa:

```
Authorization: Bearer <token>
```

`OPTIONS` preflight va `/health` ochiq. CORS `FLITE_CORS_ORIGIN` bilan boshqariladi.

## Endpointlar

| Method | Yo'l | Javob |
| --- | --- | --- |
| GET | `/health` | `{"status":"ok","service":"flite-bridge","auth":"true|false"}` |
| GET | `/api/v1/status` | nom, versiya, faol app, WiFi, uptime, heap, displey o'lchami |
| GET | `/api/v1/apps` | `{"apps":[{"id","name"},...]}` |
| GET | `/api/v1/display` | `{"width","height","format","data"}` (base64, 1-bit) |
| POST | `/api/v1/input` | `{"key":"ok","action":"short"}` |
| POST | `/api/v1/launch` | `{"id":"badge"}` |
| POST | `/api/v1/wifi` | `{"ssid":"...","password":"..."}` |
| GET | `/api/v1/events` | SSE oqimi: `event: flite`, `data: <event JSON>` |

`key`: `up|down|left|right|ok|back`; `action`: `short|long|repeat`.
Xato: `400` (noto'g'ri JSON/key), `401` (token yo'q), `502` (qurilma javob bermadi).

## Misollar

### curl

```bash
export TOKEN="..."   # bo'sh bo'lsa header liniyalarini olib tashlang
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/v1/status
curl -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
  -d '{"key":"down","action":"short"}' http://localhost:8080/api/v1/input
curl -N -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/v1/events
```

### JavaScript (veb yoki Node 18+)

```js
const headers = { 'Content-Type': 'application/json' };
if (token) headers.Authorization = `Bearer ${token}`;

await fetch('http://localhost:8080/api/v1/launch', {
  method: 'POST',
  headers,
  body: JSON.stringify({ id: 'badge' }),
});

// SSE: EventSource header yubora olmaydi, shuning uchun fetch + stream ishlating
const response = await fetch('http://localhost:8080/api/v1/events', { headers });
const reader = response.body.getReader();
// ma'lumotlarni '\n\n' bo'yicha bo'lib, 'data: ' qatorlarini o'qing
```

### Python (standart kutubxona)

```python
import json, urllib.request

base = "http://localhost:8080"
headers = {"Content-Type": "application/json"}
if token := "":  # o'z tokeningizni qo'ying
    headers["Authorization"] = f"Bearer {token}"

request = urllib.request.Request(
    base + "/api/v1/input",
    data=json.dumps({"key": "ok", "action": "short"}).encode(),
    headers=headers,
)
with urllib.request.urlopen(request) as response:
    print(response.read().decode())
```

### Kotlin (Android / JVM, OkHttp)

```kotlin
val request = Request.Builder()
    .url("http://192.168.1.20:8080/api/v1/input")
    .header("Authorization", "Bearer $token")
    .post("""{"key":"ok","action":"short"}""".toRequestBody("application/json".toMediaType()))
    .build()
OkHttpClient().newCall(request).execute().use { println(it.code) }
```

## Displey ma'lumotlarini dekodlash

`data` — 128×64, 1 bit/piksel, gorizontal joylashuv:

```
index = (y / 8) * width + x
bit   = y % 8
```

`data` ni base64 decode qilib yuqoridagi formulaga ko'ra rangli piksel chizing. Tayyor
misollar: `server/internal/api/web/app.js` va
`android/app/src/main/java/com/example/flipperlite/ui/AppScreen.kt`.

## Cheklovlar

Hozircha rate limit va TLS yo'q. Bridge'ni faqat ishonchli LAN ichida ishlating; Internetga
ochish uchun reverse proxy (TLS + token) kerak.
