#ifndef FLITE_DISPLAY_ESP_H
#define FLITE_DISPLAY_ESP_H

#include "flite_canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

void flite_display_init(void);
FliteCanvas* flite_display_canvas(void);
void flite_display_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
