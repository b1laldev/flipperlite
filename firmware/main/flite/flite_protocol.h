#ifndef FLITE_PROTOCOL_H
#define FLITE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Small, dependency-free JSON helpers shared by firmware, host tests and docs. */
bool flite_event_to_json(const FliteEvent* event, char* buf, size_t len);
bool flite_parse_input_json(const char* json, FliteInputEvent* out);
bool flite_parse_id_json(const char* json, char* out, size_t len);
bool flite_parse_wifi_json(const char* json, char* ssid, size_t ssid_len,
                           char* pass, size_t pass_len);
size_t flite_base64_encode(const uint8_t* src, size_t len, char* dst, size_t dst_len);

#ifdef __cplusplus
}
#endif

#endif
