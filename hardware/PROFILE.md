# Hardware profili (keyingi ma'lumotlar uchun)

Hozircha **default taxminlar** yozilgan. Siz aniq qismlarni sotib olgach, shu jadvalni
to'ldiring — firmware pinlari va `sdkconfig.defaults` shunga qarab o'zgartiriladi.

| Parametr | Hozirgi taxmin | Sizning qiymatingiz |
| --- | --- | --- |
| MCU | ESP32-S3 (alternativ: ESP32-C3) | _TBD_ |
| Yadro / chastota | 2× Xtensa LX7 240 MHz / 1× RISC-V 160 MHz | _TBD_ |
| Flash | 4 MB (QIO) | _TBD_ |
| PSRAM | yo'q (opsional) | _TBD_ |
| WiFi | chip ichidagi 2.4 GHz, AP+STA | _TBD_ |
| BLE | chip ichidagi BLE (keyingi bosqich) | _TBD_ |
| Displey | SSD1306 128×64, I2C 0x3C, SDA=8, SCL=9 | _TBD_ |
| Tugmalar | 6 ta, active-low + internal pull-up | _TBD_ |
| Tugma pinlari | UP=4, DOWN=5, LEFT=6, RIGHT=7, OK=15, BACK=16 | _TBD_ |
| Boot | USB-UART / USB-Serial-JTAG, EN + GPIO0 strapping | _TBD_ |
| Quvvat | 5V USB yoki 3.7V LiPo + regulator | _TBD_ |
| Batareya o'lchash | hozircha yo'q (UI da 75% demo) | _TBD_ |
| Korpus | TBD | _TBD_ |

## Boot haqida eslatma

- ESP32-S3: USB OTG (native USB) yoki UART bootloader; `GPIO0=0` bo'lsa download mode,
  `EN` past bo'lsa reset. Ko'p devboard'larda `BOOT`/`EN` tugmalari bor.
- ESP32-C3: USB Serial/JTAG orqali flash qilinadi, alohida USB-UART kerak emas.
- `idf.py flash monitor` bilan birinchi yuklashda port nomini aniqlang:
  `ls /dev/cu.usb*` (macOS).

## Qabul qilingach nima qilamiz

1. `firmware/main/Kconfig.projbuild` dagi pin/default qiymatlarini yangilaymiz.
2. `firmware/sdkconfig.defaults` da target/flash/PSRAM sozlamalarini tasdiqlaymiz.
3. Agar displey SPI yoki boshqa o'lcham bo'lsa, `flite_display_esp.c` ni moslashtiramiz.
4. Batareya ADC qo'shilsa, `flite_canvas_battery()` ga haqiqiy foiz beriladi.
5. WiFi/BLE profili tasdiqlangach, telefon bilan ulanish usulini tanlaymiz (WiFi yoki BLE).
