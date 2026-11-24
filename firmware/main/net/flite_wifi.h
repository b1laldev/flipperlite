#ifndef FLITE_WIFI_H
#define FLITE_WIFI_H

#include "esp_err.h"

#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Starts AP + STA. AP is always available so the phone/bridge can connect. */
void flite_wifi_init(void);
esp_err_t flite_wifi_connect_sta(const char* ssid, const char* password);
void flite_wifi_update_info(FliteDeviceInfo* info);

#ifdef __cplusplus
}
#endif

#endif
