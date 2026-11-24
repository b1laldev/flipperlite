#ifndef FLITE_MANAGER_H
#define FLITE_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "flite_canvas.h"
#include "flite_scene.h"
#include "flite_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FliteApp** apps;
    size_t count;
    FliteApp* active;
    FliteCanvas* canvas;
    FliteDeviceInfo info;
    bool dirty;
} FliteManager;

extern FliteManager* flite_manager_instance;

void flite_manager_init(FliteManager* manager, FliteApp** apps, size_t count, FliteCanvas* canvas);
bool flite_manager_start(FliteManager* manager, const char* id);
void flite_manager_dispatch(FliteManager* manager, const FliteEvent* event);
void flite_manager_render(FliteManager* manager);
const char* flite_manager_active_id(const FliteManager* manager);
FliteManager* flite_manager(void);

#ifdef __cplusplus
}
#endif

#endif
