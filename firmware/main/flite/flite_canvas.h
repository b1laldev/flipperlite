#ifndef FLITE_CANVAS_H
#define FLITE_CANVAS_H

#include <stdbool.h>
#include <stddef.h>

#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Horizontal bit packing: bit (y % 8) of pixel at row (y / 8), column x. */
    uint8_t pixels[FLITE_SCREEN_W * (FLITE_SCREEN_H / 8)];
} FliteCanvas;

void flite_canvas_clear(FliteCanvas* canvas);
void flite_canvas_pixel(FliteCanvas* canvas, int x, int y, bool on);
bool flite_canvas_get_pixel(const FliteCanvas* canvas, int x, int y);
void flite_canvas_rect(FliteCanvas* canvas, int x, int y, int w, int h, bool filled);
void flite_canvas_line(FliteCanvas* canvas, int x0, int y0, int x1, int y1, bool on);
void flite_canvas_text(FliteCanvas* canvas, int x, int y, const char* text, bool invert);
size_t flite_canvas_text_width(const char* text);
void flite_canvas_header(FliteCanvas* canvas, const char* title);
void flite_canvas_battery(FliteCanvas* canvas, int x, int y, int percent);

#ifdef __cplusplus
}
#endif

#endif
