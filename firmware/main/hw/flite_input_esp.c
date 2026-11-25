#include "flite_input_esp.h"

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "flite_event_bus.h"
#include "flite_types.h"

static const char* TAG = "flite/input";

typedef struct {
    int gpio;
    uint32_t level;
    int64_t us;
} RawButton;

static const int button_pins[FliteKeyCount] = {
    CONFIG_FLITE_PIN_UP, CONFIG_FLITE_PIN_DOWN, CONFIG_FLITE_PIN_LEFT,
    CONFIG_FLITE_PIN_RIGHT, CONFIG_FLITE_PIN_OK, CONFIG_FLITE_PIN_BACK,
};

static QueueHandle_t raw_queue;

static void IRAM_ATTR button_isr(void* arg) {
    int gpio = (int)(intptr_t)arg;
    RawButton event = {
        .gpio = gpio,
        .level = (uint32_t)gpio_get_level(gpio),
        .us = esp_timer_get_time(),
    };
    xQueueSendFromISR(raw_queue, &event, NULL);
}

static int key_for_gpio(int gpio) {
    for(int i = 0; i < FliteKeyCount; i++) {
        if(button_pins[i] == gpio) return i;
    }
    return -1;
}

static void post_input(int key, FliteInputType type) {
    FliteEvent event;
    event.type = FliteEventInput;
    event.input.key = (FliteKey)key;
    event.input.type = type;
    event.input.ts_ms = (uint32_t)(esp_timer_get_time() / 1000);
    event.text[0] = '\0';
    event.ts_ms = event.input.ts_ms;
    flite_event_bus_post(&event);
}

static void input_task(void* arg) {
    (void)arg;
    bool pressed[FliteKeyCount] = {false};
    int64_t down_us[FliteKeyCount] = {0};
    int64_t repeat_us[FliteKeyCount] = {0};
    const int64_t long_threshold = (int64_t)CONFIG_FLITE_LONG_PRESS_MS * 1000;
    RawButton raw;

    for(;;) {
        if(xQueueReceive(raw_queue, &raw, pdMS_TO_TICKS(50)) == pdTRUE) {
            int key = key_for_gpio(raw.gpio);
            if(key >= 0) {
                if(raw.level == 0) { /* active low, internal pull-up */
                    pressed[key] = true;
                    down_us[key] = raw.us;
                    repeat_us[key] = raw.us;
                } else if(pressed[key]) {
                    pressed[key] = false;
                    post_input(key, (raw.us - down_us[key]) > long_threshold
                                        ? FliteInputLong
                                        : FliteInputShort);
                }
            }
        }
        int64_t now = esp_timer_get_time();
        for(int key = 0; key < FliteKeyCount; key++) {
            if(pressed[key] && (now - down_us[key]) > long_threshold &&
               (now - repeat_us[key]) > 250000) {
                post_input(key, FliteInputRepeat);
                repeat_us[key] = now;
            }
        }
    }
}

void flite_input_init(void) {
    raw_queue = xQueueCreate(32, sizeof(RawButton));
    if(raw_queue == NULL) {
        ESP_LOGE(TAG, "failed to create button queue");
        return;
    }
    uint64_t mask = 0;
    for(int i = 0; i < FliteKeyCount; i++) mask |= (uint64_t)1 << button_pins[i];

    gpio_config_t config = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));
    for(int i = 0; i < FliteKeyCount; i++) {
        ESP_ERROR_CHECK(gpio_isr_handler_add(button_pins[i], button_isr,
                                             (void*)(intptr_t)button_pins[i]));
    }
    xTaskCreate(input_task, "flite_input", 3072, NULL, 4, NULL);
    ESP_LOGI(TAG, "6 buttons configured");
}
