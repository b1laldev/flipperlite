#include "flite_canvas.h"

#include <string.h>

/* Compact 3x5 font, two pixels per font pixel on a 128x64 OLED. */
static const char kGlyphChars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :.-_/>+%?!#";

static const uint8_t kGlyphs[][5] = {
    {2,5,7,5,5}, {6,5,6,5,6}, {3,4,4,4,3}, {6,5,5,5,6}, {7,4,6,4,7},
    {7,4,6,4,4}, {3,4,5,5,3}, {5,5,7,5,5}, {7,2,2,2,7}, {1,1,1,5,2},
    {5,5,6,5,5}, {4,4,4,4,7}, {5,7,7,5,5}, {5,7,7,7,5}, {2,5,5,5,2},
    {6,5,6,4,4}, {2,5,5,7,3}, {6,5,6,5,5}, {3,4,2,1,6}, {7,2,2,2,2},
    {5,5,5,5,7}, {5,5,5,5,2}, {5,5,7,7,5}, {5,5,2,5,5}, {5,5,2,2,2},
    {7,1,2,4,7},
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,3,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7},
    {0,0,0,0,0}, {0,2,0,2,0}, {0,0,0,0,2}, {0,0,7,0,0}, {0,0,0,0,7},
    {1,1,2,4,4}, {4,2,1,2,4}, {0,2,7,2,0}, {5,1,2,4,5}, {3,1,2,0,2},
    {2,2,2,0,2}, {5,7,5,7,5},
};

_Static_assert(sizeof(kGlyphs) / sizeof(kGlyphs[0]) == sizeof(kGlyphChars) - 1,
               "glyph table must match glyph character table");

void flite_canvas_clear(FliteCanvas* canvas) {
    if(canvas != NULL) memset(canvas->pixels, 0, sizeof(canvas->pixels));
}

void flite_canvas_pixel(FliteCanvas* canvas, int x, int y, bool on) {
    if(canvas == NULL || x < 0 || x >= FLITE_SCREEN_W || y < 0 || y >= FLITE_SCREEN_H) return;
    const size_t index = (size_t)(y / 8) * FLITE_SCREEN_W + (size_t)x;
    const uint8_t mask = (uint8_t)(1u << (y % 8));
    if(on) canvas->pixels[index] |= mask;
    else canvas->pixels[index] &= (uint8_t)~mask;
}

bool flite_canvas_get_pixel(const FliteCanvas* canvas, int x, int y) {
    if(canvas == NULL || x < 0 || x >= FLITE_SCREEN_W || y < 0 || y >= FLITE_SCREEN_H) return false;
    const size_t index = (size_t)(y / 8) * FLITE_SCREEN_W + (size_t)x;
    return (canvas->pixels[index] & (1u << (y % 8))) != 0;
}

void flite_canvas_rect(FliteCanvas* canvas, int x, int y, int w, int h, bool filled) {
    if(canvas == NULL || w <= 0 || h <= 0) return;
    if(filled) {
        for(int row = y; row < y + h; row++) {
            for(int col = x; col < x + w; col++) flite_canvas_pixel(canvas, col, row, true);
        }
        return;
    }
    for(int col = x; col < x + w; col++) {
        flite_canvas_pixel(canvas, col, y, true);
        flite_canvas_pixel(canvas, col, y + h - 1, true);
    }
    for(int row = y; row < y + h; row++) {
        flite_canvas_pixel(canvas, x, row, true);
        flite_canvas_pixel(canvas, x + w - 1, row, true);
    }
}

void flite_canvas_line(FliteCanvas* canvas, int x0, int y0, int x1, int y1, bool on) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for(;;) {
        flite_canvas_pixel(canvas, x0, y0, on);
        if(x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 < dx) { err += dx; y0 += sy; }
    }
}

static const uint8_t* glyph_for(char c) {
    for(size_t i = 0; i < sizeof(kGlyphChars) - 1; i++) {
        if(kGlyphChars[i] == c) return kGlyphs[i];
    }
    return kGlyphs[36]; /* space */
}

void flite_canvas_text(FliteCanvas* canvas, int x, int y, const char* text, bool invert) {
    if(canvas == NULL || text == NULL) return;
    int cursor = x;
    for(const char* p = text; *p != '\0'; p++) {
        char c = *p;
        if(c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        const uint8_t* glyph = glyph_for(c);
        for(int row = 0; row < 5; row++) {
            for(int col = 0; col < 3; col++) {
                if((glyph[row] & (1u << (2 - col))) != 0) {
                    flite_canvas_rect(canvas, cursor + col * 2, y + row * 2, 2, 2, !invert);
                }
            }
        }
        cursor += 8;
    }
}

size_t flite_canvas_text_width(const char* text) {
    if(text == NULL) return 0;
    size_t n = strlen(text);
    return n == 0 ? 0 : n * 8 - 2;
}

void flite_canvas_header(FliteCanvas* canvas, const char* title) {
    flite_canvas_rect(canvas, 0, 0, FLITE_SCREEN_W, 11, true);
    flite_canvas_text(canvas, 2, 2, title, true);
    flite_canvas_line(canvas, 0, 10, FLITE_SCREEN_W - 1, 10, false);
}

void flite_canvas_battery(FliteCanvas* canvas, int x, int y, int percent) {
    if(percent < 0) percent = 0;
    if(percent > 100) percent = 100;
    flite_canvas_rect(canvas, x, y, 16, 8, false);
    flite_canvas_rect(canvas, x + 16, y + 2, 2, 4, true);
    int fill = (percent * 12) / 100;
    flite_canvas_rect(canvas, x + 2, y + 2, fill, 4, true);
}
