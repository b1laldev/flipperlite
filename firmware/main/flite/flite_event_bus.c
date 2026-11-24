#include "flite_event_bus.h"

#include "esp_log.h"

#define FLITE_MAX_SUBSCRIBERS 4

static const char* TAG = "flite/bus";
static QueueHandle_t subscribers[FLITE_MAX_SUBSCRIBERS];
static SemaphoreHandle_t bus_lock;

void flite_event_bus_init(void) {
    bus_lock = xSemaphoreCreateMutex();
    for(size_t i = 0; i < FLITE_MAX_SUBSCRIBERS; i++) subscribers[i] = NULL;
}

QueueHandle_t flite_event_bus_subscribe(void) {
    QueueHandle_t queue = xQueueCreate(16, sizeof(FliteEvent));
    if(queue == NULL) return NULL;
    if(xSemaphoreTake(bus_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        vQueueDelete(queue);
        return NULL;
    }
    for(size_t i = 0; i < FLITE_MAX_SUBSCRIBERS; i++) {
        if(subscribers[i] == NULL) {
            subscribers[i] = queue;
            xSemaphoreGive(bus_lock);
            return queue;
        }
    }
    xSemaphoreGive(bus_lock);
    ESP_LOGW(TAG, "subscriber limit reached");
    vQueueDelete(queue);
    return NULL;
}

void flite_event_bus_unsubscribe(QueueHandle_t subscriber) {
    if(subscriber == NULL || xSemaphoreTake(bus_lock, pdMS_TO_TICKS(1000)) != pdTRUE) return;
    for(size_t i = 0; i < FLITE_MAX_SUBSCRIBERS; i++) {
        if(subscribers[i] == subscriber) subscribers[i] = NULL;
    }
    xSemaphoreGive(bus_lock);
    vQueueDelete(subscriber);
}

bool flite_event_bus_post(const FliteEvent* event) {
    if(event == NULL || bus_lock == NULL) return false;
    bool delivered = false;
    if(xSemaphoreTake(bus_lock, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    for(size_t i = 0; i < FLITE_MAX_SUBSCRIBERS; i++) {
        if(subscribers[i] != NULL) {
            if(xQueueSend(subscribers[i], event, 0) == pdTRUE) delivered = true;
        }
    }
    xSemaphoreGive(bus_lock);
    return delivered;
}
