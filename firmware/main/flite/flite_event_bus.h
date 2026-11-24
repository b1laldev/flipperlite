#ifndef FLITE_EVENT_BUS_H
#define FLITE_EVENT_BUS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ESP-IDF only: pub/sub fan-out for input, network and app events. */
void flite_event_bus_init(void);
QueueHandle_t flite_event_bus_subscribe(void);
void flite_event_bus_unsubscribe(QueueHandle_t subscriber);
bool flite_event_bus_post(const FliteEvent* event);

#ifdef __cplusplus
}
#endif

#endif
