# Yig'ish va ishga tushirish

## 0) Talablar

| Komponent | Talab |
| --- | --- |
| Firmware | ESP-IDF **5.2+**, Python 3.9+, USB-UART (yoki S3/C3 USB-Serial/JTAG) |
| Go bridge | Go 1.22+ (faqat standart kutubxona, `go mod download` shart emas) |
| Android | Android Studio / JDK 17 / Android SDK 34, Gradle wrapper (jar repo ichida) |
| Host testlari | clang yoki gcc, make |

## Tezkor skriptlar

```bash
./scripts/doctor.sh                         # qaysi tool'lar tayyor ekanini ko'rsatadi
./scripts/flash.sh esp32s3 --monitor        # yoki: esp32c3 --port /dev/cu.usbmodemXXXX
cp server/.env.example server/.env          # WiFi/token; fayl gitignore qilingan
./scripts/provision.sh                      # qurilmani kutadi, WiFi yuboradi, app ishga tushiradi
./scripts/build-android.sh --install        # APK yig'adi va ulangan telefonga o'rnatadi
```

`provision.sh` `server/.env` ni o'qiydi, lekin real environment o'zgaruvchilari har doim
ustun turadi. WiFi paroli hech qachon logga yoki jarayon argumentlariga chiqmaydi.
CI har pushda `esp32s3` va `esp32c3` firmware'ini hamda debug APK'ni yig'ib, artifact
sifatida saqlaydi.

## 1) Host testlari ( apparatsiz )

```bash
make -C firmware/host test
cd server && go test ./...
```

## 2) Firmware (ESP32-S3 / ESP32-C3)

```bash
# ESP-IDF ni faollashtiring (masalan ~/esp/esp-idf/export.sh)
. $IDF_PATH/export.sh

cd firmware
idf.py set-target esp32s3       # esp32c3 ham qo'llab-quvvatlanadi
idf.py menuconfig               # "FlipperLite hardware" bo'limi
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

Yoki tayyor skript bilan (target va portni o'zi tanlaydi):

```bash
./scripts/flash.sh esp32s3 --monitor
```

### Pinlar (default = ESP32-S3 uchun misol)

| Funksiya | Default GPIO |
| --- | --- |
| OLED SDA / SCL | 8 / 9 |
| UP / DOWN / LEFT | 4 / 5 / 6 |
| RIGHT / OK / BACK | 7 / 15 / 16 |

ESP32-C3 da boshqa pinlar kerak bo'lishi mumkin — `menuconfig` orqali o'zgartiring.
Tugmalar **internal pull-up** bilan ishlaydi: bir uchi GPIO, ikkinchisi GND.
SSD1306 odatda 0x3C (ba'zida 0x3D) manzilida; `FlipperLite hardware → SSD1306 I2C address`.

### Birinchi ishga tushirish

1. Qurilma `FlipperLite-XXXX` nomli ochiq SoftAP ochadi, IP `192.168.4.1`.
2. Telefon/kompyuter shu tarmoqqa ulanadi.
3. `http://192.168.4.1/api/status` — JSON status qaytadi.
4. Go bridge'ni ishga tushiring: `go run ./cmd/flite-bridge --device http://192.168.4.1`.
5. Agar bridge boshqa tarmoqda bo'lsa (masalan Wi-Fi ga STA ulanish orqali),
   `POST /api/v1/wifi` orqali **o'zingizning** tarmog'ingiz ma'lumotlarini yuboring.

## 3) Go bridge

```bash
cd server
go build -o flite-bridge ./cmd/flite-bridge
FLITE_DEVICE_URL=http://192.168.4.1 FLITE_LISTEN_ADDR=:8080 ./flite-bridge
# yoki
go run ./cmd/flite-bridge --device http://192.168.4.1 --listen :8080
```

`curl http://localhost:8080/health` → `{"status":"ok",...}`.

Veb-dashboard: `http://localhost:8080/` — qurilma holati, app tugmalari, D-pad, displey
preview va SSE voqealari. Sozlamalar `.env` dan o'qiladi (`server/.env.example` ni nusxalang):

```bash
cd server
cp .env.example .env
# FLITE_API_TOKEN ni o'zgartiring: openssl rand -hex 32
go run ./cmd/flite-bridge
```

Real muhit o'zgaruvchilari `.env` ustidan ustun turadi. Token qo'yilsa veb-panel va Android
ilovasida maxsus maydonga kiritiladi. Batafsil: [SECRETS.md](SECRETS.md).

**Apparatsiz to'liq sinov:** avval `go run ./cmd/fake-device --listen :9090` ni ishga tushiring,
keyin bridge'ni `--device http://localhost:9090` bilan oching. Soxta qurilma xuddi ESP32 kabi
`/api/status`, `/api/apps`, `/api/display`, `/api/input`, `/api/events` javob beradi; Android
ilovasidagi D-pad va app tugmalari shu bilan ishlaydi.

## 4) Android

```bash
cd android
./gradlew :app:assembleDebug
# yoki Android Studio'da android/ papkani ochib Run
```

Ilovada **Bridge URL** ni o'zgartiring:
- emulator: `http://10.0.2.2:8080`
- haqiqiy telefon: `http://<kompyuter-LAN-IP>:8080` (telefon va kompyuter bir tarmoqda bo'lishi kerak).

HTTP oddiy (cleartext) bo'lgani uchun `usesCleartextTraffic="true"` qo'yilgan — bu faqat
mahalliy tarmoq uchun. Internetga ochmang.

## Muammolar

- `i2c_master...` xatolari → SDA/SCL pinlari yoki OLED manzilini tekshiring.
- `HTTP 404 /api/...` → firmware eski versiyasi yoki noto'g'ri IP.
- Android `Bridge offline` → bridge ishlamayapti yoki URL noto'g'ri; avval `curl /health` sinang.
- ESP32-C3 da xotira yetishmasa → `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` va OLED/SSE
  bufferlarini kichraytiring (framebuffer 1 KB, SSE chunk 384 B).
