# NOTICE

FlipperLite is an independent, clean-room implementation inspired by the architecture of
Flipper Zero firmware (https://github.com/flipperdevices/flipperzero-firmware), which is
licensed under **GPL-3.0**.

- No Flipper Zero source code, headers, fonts, icons, assets, protobuf definitions or generated
  files were copied into this repository.
- Names such as "scene", "view dispatcher", "input event" describe common design patterns that
  are documented publicly in the Flipper developer docs; our implementations are original and
  written for ESP-IDF/FreeRTOS.
- ESP-IDF, FreeRTOS and related Espressif components keep their own licenses; when building the
  firmware you must comply with the licenses of the toolchain and SDK you install.
- Android and Go dependencies keep their respective licenses (Apache-2.0 for AndroidX/Compose,
  Apache-2.0 for OkHttp, BSD-style for Go standard library tooling).

If you later decide to copy or adapt GPL-covered Flipper code (for example a widget or an app),
this repository's MIT license does not apply to that code: you must preserve upstream notices,
publish corresponding source and license the derivative under GPL-3.0. See docs/PORTING.md.

This is a technical notice, not legal advice.
