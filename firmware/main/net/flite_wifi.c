#include "flite_wifi.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"

static const char* TAG = "flite/wifi";
static char ap_ssid[33] = CONFIG_FLITE_AP_SSID;
static char ap_ip[16] = "192.168.4.1";
static char sta_ip[16] = "0.0.0.0";
static bool sta_link_up = false;

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    (void)arg;
    (void)data;
    if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "STA disconnected, retrying");
        esp_wifi_connect();
    }
}

static void ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    (void)arg;
    (void)base;
    (void)data;
    if(id == IP_EVENT_STA_GOT_IP) {
        sta_link_up = true;
        ESP_LOGI(TAG, "STA got IP: %s", sta_ip);
    } else if(id == IP_EVENT_STA_LOST_IP) {
        sta_link_up = false;
    }
}

static void refresh_ips(void) {
    esp_netif_ip_info_t info;
    esp_netif_t* ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if(ap != NULL && esp_netif_get_ip_info(ap, &info) == ESP_OK) {
        snprintf(ap_ip, sizeof(ap_ip), IPSTR, IP2STR(&info.ip));
    }
    esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if(sta != NULL && esp_netif_get_ip_info(sta, &info) == ESP_OK) {
        snprintf(sta_ip, sizeof(sta_ip), IPSTR, IP2STR(&info.ip));
        sta_link_up = esp_netif_is_netif_up(sta);
    }
}

void flite_wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                                        &ip_event_handler, NULL, NULL));

    uint8_t mac[6] = {0};
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));
    snprintf(ap_ssid, sizeof(ap_ssid), "%s-%02X%02X", CONFIG_FLITE_AP_SSID, mac[4], mac[5]);

    wifi_config_t ap_config = {0};
    strncpy((char*)ap_config.ap.ssid, ap_ssid, sizeof(ap_config.ap.ssid) - 1);
    ap_config.ap.ssid_len = strlen(ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_OPEN; /* local setup network only */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    refresh_ips();
    ESP_LOGI(TAG, "AP '%s' at %s", ap_ssid, ap_ip);
}

esp_err_t flite_wifi_connect_sta(const char* ssid, const char* password) {
    if(ssid == NULL || password == NULL || strlen(ssid) == 0 || strlen(ssid) > 32 ||
       strlen(password) > 64) {
        return ESP_ERR_INVALID_ARG;
    }
    wifi_config_t sta_config = {0};
    strncpy((char*)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char*)sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);
    sta_config.sta.threshold.authmode = strlen(password) >= 8 ? WIFI_AUTH_WPA2_PSK
                                                              : WIFI_AUTH_OPEN;
    esp_wifi_disconnect(); /* no-op when STA is not associated */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    return esp_wifi_connect();
}

void flite_wifi_update_info(FliteDeviceInfo* info) {
    if(info == NULL) return;
    refresh_ips();
    if(sta_link_up) {
        strncpy(info->mode, "STA", sizeof(info->mode) - 1);
        wifi_config_t sta_config;
        if(esp_wifi_get_config(WIFI_IF_STA, &sta_config) == ESP_OK) {
            snprintf(info->ssid, sizeof(info->ssid), "%s", (const char*)sta_config.sta.ssid);
        }
        snprintf(info->ip, sizeof(info->ip), "%s", sta_ip);
    } else {
        strncpy(info->mode, "AP", sizeof(info->mode) - 1);
        snprintf(info->ssid, sizeof(info->ssid), "%s", ap_ssid);
        snprintf(info->ip, sizeof(info->ip), "%s", ap_ip);
    }
    info->uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);
    info->free_heap = esp_get_free_heap_size();
}
