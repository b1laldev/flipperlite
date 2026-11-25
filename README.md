# FlipperLite — Flipper Zero g'oyasining arzon ESP32 porti

**Public repo:** https://github.com/b1laldev/flipperlite

Flipper Zero firmwareining **arxitekturasidan ilhomlanib**, oddiy va arzon ESP32-S3 / ESP32-C3
chiplariga mo'ljallangan yengil qurilma platformasi. To'liq firmware kloni emas: faqat kerakli
qismlar (holat mashinasi, scene/view tizimi, input, kichik displey, app ro'yxati) qaytadan yozilgan.

> **Xavfsizlik:** loyiha faqat **o'zingizga tegishli** tarmoq va qurilmalar uchun. Hech qanday
> tarmoqni skanerlash, buzish yoki begona ma'lumot olish vositasi yo'q. Badge demosidagi
> SSID/PIN faqat namuna qiymatlar.

## Tuzilma

| Papka | Vazifasi |
| --- | --- |
| `firmware/main/flite/` | Furi-uslubidagi yadro: event bus, canvas, scene, manager, JSON protokol |
| `firmware/main/apps/` | Menu, Badge, WiFi, Settings ilovalari |
| `firmware/main/hw/` | SSD1306 (I2C) displey va 6 tugma (short/long/repeat) |
| `firmware/main/net/` | SoftAP/STA WiFi, REST + SSE API (port 80) |
| `firmware/host/` | Apparatsiz C testlari (`make -C firmware/host test`) |
| `server/` | Go bridge: REST/SSE + veb-dashboard, boshqa ilovalar uchun ochiq API |
| `server/internal/api/web/` | Veb boshqaruv paneli (HTML/CSS/JS, Go `embed` ichida) |
| `android/` | Kotlin + Compose ilova: status, displey, app ishga tushirish, D-pad, voqealar |
| `docs/` | Arxitektura, protokol, tashqi API, sirlar, yig'ish, port xaritasi |
| `scripts/` | `flash.sh`, `provision.sh`, `build-android.sh`, `doctor.sh` — apparat kelganda tayyor ishga tushirish |
| `hardware/PROFILE.md` | Prosessor/WiFi/boot/displey pinlari — kelguchi ma'lumotlar uchun shablon |

## Tez boshlash

```bash
# 1) Apparatsiz testlar (ESP-IDF kerak emas)
make -C firmware/host test
cd server && go test ./... && cd ..

# 2) Go bridge (qurilmasiz sinash uchun soxta qurilma + bridge)
cd server
cp .env.example .env                          # kerak bo'lsa token/CORS ni shu yerga yozing
go run ./cmd/fake-device --listen :9090 &     # apparatsiz demo
go run ./cmd/flite-bridge --device http://localhost:9090 --listen :8080
# veb-panel: http://localhost:8080/  (Android ilovasi ham shu API ga ulanadi)

# 3) ESP32 firmware + provisioning (ESP-IDF >= 5.2 kerak)
./scripts/flash.sh esp32s3 --monitor    # yoki: esp32c3 --port /dev/cu.usbmodemXXXX
./scripts/provision.sh                  # server/.env dagi WiFi ma'lumotlari bilan

# 4) Android ilova (Android SDK + JDK 17 kerak)
./scripts/build-android.sh --install    # yoki --release / --clean
```

`./scripts/doctor.sh` mahalliy tool'larning tayyorligini ko'rsatadi. GitHub Actions har pushda
ikki target uchun firmware bin fayllarini va debug APK'ni artifact qilib yig'adi.

Veb-panel `http://<bridge-host>:8080/` manzilida ishlaydi: qurilma holati, app tugmalari,
D-pad, displey preview va SSE voqealari. Telefon/veb/boshqa ilova **bir xil REST API** dan
foydalanadi — [docs/EXTERNAL_API.md](docs/EXTERNAL_API.md).

Telefon ilovasida **Bridge URL** ni kompyuteringizning LAN manziliga o'zingiz o'zgartiring
(masalan `http://192.168.1.20:8080`). Emulator uchun `http://10.0.2.2:8080` mos keladi.
Agar `FLITE_API_TOKEN` qo'yilgan bo'lsa, tokenni veb-panel yoki Android ilovasidagi
maxsus maydonga kiriting. Sirlar va token xavfsizligi: [docs/SECRETS.md](docs/SECRETS.md).
Batafsil yig'ish: [docs/BUILD.md](docs/BUILD.md).

## Maqsad va keyingi qadam

Hozirgi bosqich: **firmware + Go bridge + Android boshqaruv paneli** tayyor.
Keyingi bosqich rejalari:

- haqiqiy displey/board kelgach `hardware/PROFILE.md` dagi pinlarni tasdiqlash;
- BLE orqali qurilma topish va sozlash (Wi-Fi parolni yuborish o'rniga);
- sozlamalarni NVS da saqlash, OTA yangilash;
- ESP32-C3 uchun xotira profilini alohida optimallashtirish.

Eski Python simulyatori va MF680 vositalari o'chirildi, lekin birinchi commit
(`c27ee32`) da to'liq saqlanib qolgan — kerak bo'lsa `git show c27ee32` bilan qaytarish mumkin.

## Litsenziya

Ushbu loyiha kodi **MIT** ([LICENSE](LICENSE)). Flipper Zero firmwareining o'zi **GPL-3.0**;
biz uning kodini ko'chirmadik, faqat arxitektura g'oyalarini o'rgandik.
Batafsil: [NOTICE.md](NOTICE.md) va [docs/PORTING.md](docs/PORTING.md).
