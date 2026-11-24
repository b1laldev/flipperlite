# Yig'ish va ishga tushirish

## 0) Talablar

| Komponent | Talab |
| --- | --- |
| Firmware | ESP-IDF **5.2+**, Python 3.9+, USB-UART (yoki S3/C3 USB-Serial/JTAG) |
| Go bridge | Go 1.22+ (faqat standart kutubxona, `go mod download` shart emas) |
| Android | Android Studio / JDK 17 / Android SDK 34, Gradle wrapper (jar repo ichida) |
| Host testlari | clang yoki gcc, make |

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
