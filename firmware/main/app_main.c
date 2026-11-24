#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "flite_apps.h"
#include "flite_display_esp.h"
#include "flite_event_bus.h"
#include "flite_input_esp.h"
#include "flite_manager.h"
#include "flite_http.h"
#include "flite_wifi.h"

static const char* TAG = "flite/main";
static FliteManager manager;
static QueueHandle_t event_queue;
static uint32_t last_info_ms;

static void main_task(void* arg) {
    (void)arg;
    for(;;) {
        FliteEvent event;
        if(xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            flite_manager_dispatch(&manager, &event);
        }
        uint32_t now = pdTICKS_TO_MS(xTaskGetTickCount());
        if(now - last_info_ms > 1000) {
            flite_wifi_update_info(&manager.info);
            manager.dirty = true;
            last_info_ms = now;
        }
        if(manager.dirty) {
            flite_manager_render(&manager);
            flite_display_refresh();
        }
    }
}

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    flite_event_bus_init();
    event_queue = flite_event_bus_subscribe();

    flite_display_init();
    flite_apps_init(flite_display_canvas());
    flite_manager_init(&manager, flite_apps, flite_apps_count, flite_display_canvas());
    flite_manager_start(&manager, "menu");

    flite_wifi_init();
    flite_wifi_update_info(&manager.info);
    flite_input_init();
    flite_http_init();

    xTaskCreate(main_task, "flite_main", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "FlipperLite %s started", FLITE_VERSION);
}
