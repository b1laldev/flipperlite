#ifndef FLITE_SCENE_H
#define FLITE_SCENE_H

#include <stdbool.h>
#include <stddef.h>

#include "flite_canvas.h"
#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FliteApp FliteApp;

typedef struct {
    const char* name;
    void (*on_enter)(FliteApp* app);
    bool (*on_event)(FliteApp* app, const FliteEvent* event);
    void (*on_render)(FliteApp* app, FliteCanvas* canvas);
    void (*on_exit)(FliteApp* app);
} FliteScene;

struct FliteApp {
    const char* id;
    const char* name;
    void* ctx;
    const FliteScene* const* scenes;
    size_t scene_count;
    size_t current_scene;
};

void flite_app_set_scene(FliteApp* app, size_t index);
void flite_app_dispatch(FliteApp* app, const FliteEvent* event);
void flite_app_render(FliteApp* app, FliteCanvas* canvas);

#ifdef __cplusplus
}
#endif

#endif
