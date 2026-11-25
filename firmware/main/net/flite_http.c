#include "flite_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "flite_apps.h"
#include "flite_display_esp.h"
#include "flite_event_bus.h"
#include "flite_manager.h"
#include "flite_protocol.h"
#include "flite_wifi.h"

static const char* TAG = "flite/http";
static httpd_handle_t server;

static void set_cors(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

static bool read_body(httpd_req_t* req, char* buf, size_t size) {
    if(req->content_len <= 0 || (size_t)req->content_len >= size) return false;
    int received = httpd_req_recv(req, buf, req->content_len);
    if(received <= 0) return false;
    buf[received] = '\0';
    return true;
}

/* Avoid breaking JSON when an SSID contains quotes or backslashes. */
static void json_safe(const char* src, char* dst, size_t size) {
    size_t j = 0;
    for(size_t i = 0; src[i] != '\0' && j + 1 < size; i++) {
        char c = src[i];
        dst[j++] = (c == '"' || c == '\\') ? ' ' : c;
    }
    dst[j] = '\0';
}

static esp_err_t status_get(httpd_req_t* req) {
    FliteDeviceInfo info;
    flite_wifi_update_info(&info);
    char mode[16], ssid[64], ip[32], app[32];
    json_safe(info.mode, mode, sizeof(mode));
    json_safe(info.ssid, ssid, sizeof(ssid));
    json_safe(info.ip, ip, sizeof(ip));
    json_safe(flite_manager_active_id(flite_manager()), app, sizeof(app));

    char json[640];
    int len = snprintf(json, sizeof(json),
                       "{\"name\":\"FlipperLite\",\"version\":\"%s\",\"app\":\"%s\","
                       "\"mode\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\","
                       "\"uptime_s\":%lu,\"free_heap\":%lu,"
                       "\"display\":{\"width\":%d,\"height\":%d}}",
                       FLITE_VERSION, app, mode, ssid, ip,
                       (unsigned long)info.uptime_s, (unsigned long)info.free_heap,
                       FLITE_SCREEN_W, FLITE_SCREEN_H);
    if(len < 0 || (size_t)len >= sizeof(json)) return ESP_ERR_NO_MEM;
    set_cors(req);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t apps_get(httpd_req_t* req) {
    char json[512];
    size_t used = 0;
    used += (size_t)snprintf(json + used, sizeof(json) - used, "{\"apps\":[");
    for(size_t i = 0; i < flite_apps_count && used + 96 < sizeof(json); i++) {
        used += (size_t)snprintf(json + used, sizeof(json) - used,
                                 "%s{\"id\":\"%s\",\"name\":\"%s\"}",
                                 i == 0 ? "" : ",", flite_apps[i]->id, flite_apps[i]->name);
    }
    snprintf(json + used, sizeof(json) - used, "]}");
    set_cors(req);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t display_get(httpd_req_t* req) {
    char* encoded = malloc(1408);
    if(encoded == NULL) return ESP_ERR_NO_MEM;
    FliteCanvas* canvas = flite_display_canvas();
    size_t len = flite_base64_encode(canvas->pixels, sizeof(canvas->pixels), encoded, 1408);
    char json[1450];
    int n = snprintf(json, sizeof(json),
                     "{\"width\":%d,\"height\":%d,\"format\":\"1bit-row-major\",\"data\":\"%s\"}",
                     FLITE_SCREEN_W, FLITE_SCREEN_H, encoded);
    free(encoded);
    if(n < 0 || (size_t)n >= sizeof(json)) return ESP_ERR_NO_MEM;
    set_cors(req);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t input_post(httpd_req_t* req) {
    char body[192];
    FliteInputEvent input;
    if(!read_body(req, body, sizeof(body)) || !flite_parse_input_json(body, &input)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad input JSON");
    }
    FliteEvent event;
    memset(&event, 0, sizeof(event));
    event.type = FliteEventInput;
    event.input = input;
    event.input.ts_ms = (uint32_t)(esp_timer_get_time() / 1000);
    event.ts_ms = event.input.ts_ms;
    flite_event_bus_post(&event);
    set_cors(req);
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

static esp_err_t launch_post(httpd_req_t* req) {
    char body[192], id[32];
    if(!read_body(req, body, sizeof(body)) || !flite_parse_id_json(body, id, sizeof(id))) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad launch JSON");
    }
    FliteEvent event;
    memset(&event, 0, sizeof(event));
    event.type = FliteEventAppStart;
    snprintf(event.text, sizeof(event.text), "%s", id);
    flite_event_bus_post(&event);
    set_cors(req);
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

static esp_err_t wifi_post(httpd_req_t* req) {
    char body[192], ssid[33], password[65];
    if(!read_body(req, body, sizeof(body)) ||
       !flite_parse_wifi_json(body, ssid, sizeof(ssid), password, sizeof(password)) ||
       flite_wifi_connect_sta(ssid, password) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad WiFi JSON");
    }
    set_cors(req);
    return httpd_resp_send(req, "{\"ok\":true}", 11);
}

static esp_err_t events_get(httpd_req_t* req) {
    QueueHandle_t subscriber = flite_event_bus_subscribe();
    if(subscriber == NULL) {
#ifdef HTTPD_503_SERVICE_UNAVAILABLE
        return httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE, "busy");
#else
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "busy");
#endif
    }

    set_cors(req);
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    const char* hello = "retry: 3000\n\n";
    if(httpd_resp_send_chunk(req, hello, strlen(hello)) != ESP_OK) {
        flite_event_bus_unsubscribe(subscriber);
        return ESP_OK;
    }

    for(int i = 0; i < 240; i++) {
        FliteEvent event;
        if(xQueueReceive(subscriber, &event, pdMS_TO_TICKS(500)) == pdTRUE) {
            char json[256], chunk[384];
            if(flite_event_to_json(&event, json, sizeof(json))) {
                int n = snprintf(chunk, sizeof(chunk), "event: flite\ndata: %s\n\n", json);
                if(n > 0 && httpd_resp_send_chunk(req, chunk, strlen(chunk)) != ESP_OK) break;
            }
        } else if(httpd_resp_send_chunk(req, ": ping\n\n", strlen(": ping\n\n")) != ESP_OK) {
            break;
        }
    }
    httpd_resp_send_chunk(req, NULL, 0);
    flite_event_bus_unsubscribe(subscriber);
    return ESP_OK;
}

static esp_err_t options_all(httpd_req_t* req) {
    set_cors(req);
    return httpd_resp_send(req, NULL, 0);
}

void flite_http_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    const httpd_uri_t routes[] = {
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/api/apps", .method = HTTP_GET, .handler = apps_get},
        {.uri = "/api/display", .method = HTTP_GET, .handler = display_get},
        {.uri = "/api/input", .method = HTTP_POST, .handler = input_post},
        {.uri = "/api/launch", .method = HTTP_POST, .handler = launch_post},
        {.uri = "/api/wifi", .method = HTTP_POST, .handler = wifi_post},
        {.uri = "/api/events", .method = HTTP_GET, .handler = events_get},
        {.uri = "/api/*", .method = HTTP_OPTIONS, .handler = options_all},
    };
    for(size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &routes[i]));
    }
    ESP_LOGI(TAG, "HTTP API ready on port 80");
}
