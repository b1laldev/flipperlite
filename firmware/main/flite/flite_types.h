#ifndef FLITE_TYPES_H
#define FLITE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLITE_SCREEN_W 128
#define FLITE_SCREEN_H 64
#define FLITE_VERSION "0.1.0"

typedef enum {
    FliteKeyUp = 0,
    FliteKeyDown,
    FliteKeyLeft,
    FliteKeyRight,
    FliteKeyOk,
    FliteKeyBack,
    FliteKeyCount
} FliteKey;

typedef enum {
    FliteInputShort = 0,
    FliteInputLong,
    FliteInputRepeat,
    FliteInputTypeCount
} FliteInputType;

typedef struct {
    FliteKey key;
    FliteInputType type;
    uint32_t ts_ms;
} FliteInputEvent;

typedef enum {
    FliteEventInput = 0,
    FliteEventTick,
    FliteEventNet,
    FliteEventAppStart,
    FliteEventCustom
} FliteEventType;

typedef struct {
    FliteEventType type;
    FliteInputEvent input;
    char text[64];
    uint32_t ts_ms;
} FliteEvent;

typedef struct {
    char mode[8];
    char ssid[33];
    char ip[16];
    uint32_t uptime_s;
    uint32_t free_heap;
} FliteDeviceInfo;

const char* flite_key_name(FliteKey key);
const char* flite_input_type_name(FliteInputType type);
bool flite_key_from_name(const char* name, FliteKey* out);
bool flite_input_type_from_name(const char* name, FliteInputType* out);

#ifdef __cplusplus
}
#endif

#endif
