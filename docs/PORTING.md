# Flipper Zero'dan ESP32 ga: nima olindi, nima tashlandi

Manba: [flipperzero-firmware `dev`](https://github.com/flipperdevices/flipperzero-firmware)
(tekshirilgan commit `e459e2d939db7f292d7dcfe6935b8fb2641409e3`). To'liq klon olinmadi:
faqat ochiq manbalar o'rganildi, barcha kod qaytadan yozildi.

## Arxitektura xaritasi

| Flipper tizimi | Asl manba (raw URL) | Bizdagi ekvivalent |
| --- | --- | --- |
| Furi core / record / event loop | [furi.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/furi.h), [kernel.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/core/kernel.h), [event_loop.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/core/event_loop.h), [pubsub.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/core/pubsub.h) | `flite_event_bus.c` (FreeRTOS queue, 4 obunachi). FuriRecord, threads va event loop wrapper'lari kerak emas — to'g'ridan-to'g'ri FreeRTOS/esp_timer ishlatiladi |
| Message queue / mutex / timer | [message_queue.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/core/message_queue.h), [timer.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/furi/core/timer.h) | `QueueHandle_t`, `SemaphoreHandle_t`, `esp_timer_get_time()` |
| Input servisi | [input.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/input/input.h), [input.c](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/input/input.c) | `flite_input_esp.c`: GPIO ISR → queue → short/long/repeat. `FliteKey` enum'i Up/Down/Left/Right/Ok/Back shaklini saqlaydi |
| GUI / Canvas / View / ViewDispatcher | [gui.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/gui.h), [canvas.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/canvas.h), [view.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/view.h), [view_dispatcher.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/view_dispatcher.h) | `flite_canvas.c` + bitta UI taski. Monoxrom 128×64, o'z shriftimiz. To'liq View ob'ektlari/layer/animatsiya tizimi yo'q |
| SceneManager | [scene_manager.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/scene_manager.h), [scene_manager.c](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/gui/scene_manager.c) | `flite_scene.c`: const jadvallar `on_enter/on_event/on_render/on_exit`, scene indeksi. Scene stack o'rniga oddiy qaytish (BACK → scene 0) |
| App skeleti / loader | [loader_menu.c](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/loader/loader_menu.c), [example_view_dispatcher.c](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/examples/example_view_dispatcher/example_view_dispatcher.c) | `flite_apps.c`: statik app ro'yxati + `flite_manager_start(id)`. Dinamik FAP yuklagich yo'q |
| Storage / Notification | [storage.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/storage/storage.h), [notification.h](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/services/notification/notification.h) | Hozircha yo'q. Keyingi qadam: NVS (SSID/sozlamalar) + LED/VIBRO servisi |
| Build / target | [fbt.md](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/documentation/fbt.md), [AppManifests.md](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/documentation/AppManifests.md), [targets/ReadMe.md](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/targets/ReadMe.md) | ESP-IDF CMake (`firmware/CMakeLists.txt`), Kconfig (`Kconfig.projbuild`), `sdkconfig.defaults`. Barcha pinlar bitta konfiguratsiya bo'limida |

## Chip farqi

| | ESP32-S3 (tavsiya) | ESP32-C3 |
| --- | --- | --- |
| Yadro | 2× Xtensa LX7, 240 MHz | 1× RISC-V, 160 MHz |
| RAM | 512 KB (PSRAM li modellar bor) | ~400 KB, PSRAM yo'q |
| Simsiz | 2.4 GHz WiFi + BLE, USB OTG | 2.4 GHz WiFi + BLE, USB Serial/JTAG |
| Port uchun | To'liq GUI + SSE + kelajakda preview | Kichik UI; bufferlarni tejamkor ishlatish |

Ma'lumotlar: [ESP32-S3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf),
[ESP32-C3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf),
[esp_http_server docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/esp_http_server.html).

## Qasddan port qilinmadi

SubGHz, NFC, LFRFID, iButton, infraqizil, BadUSB/U2F/HID, dinamik FAP loader,
RPC/protobuf, JS dvijogi, dolphin/animatsiyalar, STM32 HAL va koprozessor radio firmware,
SD/FatFS (anon ehtiyoj tug'ilmaguncha).

## Litsenziya

Asl firmware **GPL-3.0** ([LICENSE](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/LICENSE)).
Arxitektura g'oyasi ochiq hujjatlarda tasvirlangan, ammo **kod, header, shrift, ikonka yoki
asset ko'chirilsa**, ish GPL-3.0 hosilasiga aylanadi: manba kodi bilan birga tarqatish,
upstream bildirishnomalarini saqlash va hosilani GPL-3.0 ostida litsenziyalash kerak.
Biz bunday materiallarni ko'chirmadik; shuning uchun yangi kod MIT ostida.
Qo'shimcha ma'lumot: [NOTICE.md](../NOTICE.md). Bu yuridik maslahat emas.

Foydali hujjatlar: [Flipper Developer Docs](https://developer.flipper.net/flipperzero/doxygen/),
[targets/ReadMe.md](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/targets/ReadMe.md).
