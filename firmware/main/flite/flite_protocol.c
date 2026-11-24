#include "flite_protocol.h"

#include <stdio.h>
#include <string.h>

static const char* kKeyNames[FliteKeyCount] = {
    "up", "down", "left", "right", "ok", "back",
};

static const char* kTypeNames[FliteInputTypeCount] = {
    "short", "long", "repeat",
};

const char* flite_key_name(FliteKey key) {
    return (key >= 0 && key < FliteKeyCount) ? kKeyNames[key] : "unknown";
}

const char* flite_input_type_name(FliteInputType type) {
    return (type >= 0 && type < FliteInputTypeCount) ? kTypeNames[type] : "unknown";
}

bool flite_key_from_name(const char* name, FliteKey* out) {
    if(name == NULL || out == NULL) return false;
    for(int i = 0; i < FliteKeyCount; i++) {
        if(strcmp(name, kKeyNames[i]) == 0) {
            *out = (FliteKey)i;
            return true;
        }
    }
    return false;
}

bool flite_input_type_from_name(const char* name, FliteInputType* out) {
    if(name == NULL || out == NULL) return false;
    for(int i = 0; i < FliteInputTypeCount; i++) {
        if(strcmp(name, kTypeNames[i]) == 0) {
            *out = (FliteInputType)i;
            return true;
        }
    }
    return false;
}

static bool find_field(const char* json, const char* field, const char** value_start,
                       size_t* value_len) {
    if(json == NULL || field == NULL) return false;
    char needle[40];
    int n = snprintf(needle, sizeof(needle), "\"%s\"", field);
    if(n < 0 || (size_t)n >= sizeof(needle)) return false;
    const char* p = strstr(json, needle);
    if(p == NULL) return false;
    p += strlen(needle);
    while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if(*p != ':') return false;
    p++;
    while(*p == ' ' || *p == '\t') p++;
    if(*p != '"') return false;
    p++;
    const char* start = p;
    while(*p != '\0' && *p != '"') {
        if(*p == '\\' && *(p + 1) != '\0') p++;
        p++;
    }
    if(*p != '"') return false;
    *value_start = start;
    *value_len = (size_t)(p - start);
    return true;
}

static bool copy_field(const char* json, const char* field, char* out, size_t out_len) {
    const char* start = NULL;
    size_t len = 0;
    if(!find_field(json, field, &start, &len) || out_len == 0) return false;
    if(len >= out_len) return false;
    memcpy(out, start, len);
    out[len] = '\0';
    return true;
}

bool flite_event_to_json(const FliteEvent* event, char* buf, size_t len) {
    if(event == NULL || buf == NULL || len == 0) return false;
    if(event->type == FliteEventInput) {
        int n = snprintf(buf, len,
                         "{\"type\":\"input\",\"key\":\"%s\",\"action\":\"%s\",\"ts_ms\":%lu}",
                         flite_key_name(event->input.key),
                         flite_input_type_name(event->input.type),
                         (unsigned long)event->input.ts_ms);
        return n > 0 && (size_t)n < len;
    }
    const char* type = "state";
    if(event->type == FliteEventTick) type = "tick";
    if(event->type == FliteEventNet) type = "net";
    if(event->type == FliteEventAppStart) type = "app";
    int n = snprintf(buf, len, "{\"type\":\"%s\",\"text\":\"%s\",\"ts_ms\":%lu}",
                     type, event->text, (unsigned long)event->ts_ms);
    return n > 0 && (size_t)n < len;
}

bool flite_parse_input_json(const char* json, FliteInputEvent* out) {
    char key[24] = {0};
    char action[24] = {0};
    if(out == NULL || !copy_field(json, "key", key, sizeof(key)) ||
       !copy_field(json, "action", action, sizeof(action))) {
        return false;
    }
    if(!flite_key_from_name(key, &out->key) ||
       !flite_input_type_from_name(action, &out->type)) {
        return false;
    }
    out->ts_ms = 0;
    return true;
}

bool flite_parse_id_json(const char* json, char* out, size_t len) {
    return copy_field(json, "id", out, len);
}

bool flite_parse_wifi_json(const char* json, char* ssid, size_t ssid_len,
                           char* pass, size_t pass_len) {
    return copy_field(json, "ssid", ssid, ssid_len) &&
           copy_field(json, "password", pass, pass_len);
}

size_t flite_base64_encode(const uint8_t* src, size_t len, char* dst, size_t dst_len) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out = 0;
    for(size_t i = 0; i < len; i += 3) {
        uint32_t chunk = (uint32_t)src[i] << 16;
        if(i + 1 < len) chunk |= (uint32_t)src[i + 1] << 8;
        if(i + 2 < len) chunk |= (uint32_t)src[i + 2];
        if(out + 4 > dst_len) return out;
        dst[out++] = table[(chunk >> 18) & 0x3f];
        dst[out++] = table[(chunk >> 12) & 0x3f];
        dst[out++] = (i + 1 < len) ? table[(chunk >> 6) & 0x3f] : '=';
        dst[out++] = (i + 2 < len) ? table[chunk & 0x3f] : '=';
    }
    if(out < dst_len) dst[out] = '\0';
    return out;
}
