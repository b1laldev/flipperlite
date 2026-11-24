#ifndef FLITE_INPUT_ESP_H
#define FLITE_INPUT_ESP_H

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO buttons -> short/long/repeat FliteInputEvent on the event bus. */
void flite_input_init(void);

#ifdef __cplusplus
}
#endif

#endif
