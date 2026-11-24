#include "flite_manager.h"

#include <string.h>

FliteManager* flite_manager_instance = NULL;

void flite_manager_init(FliteManager* manager, FliteApp** apps, size_t count, FliteCanvas* canvas) {
    if(manager == NULL) return;
    manager->apps = apps;
    manager->count = count;
    manager->active = NULL;
    manager->canvas = canvas;
    manager->dirty = true;
    memset(&manager->info, 0, sizeof(manager->info));
    strncpy(manager->info.mode, "AP", sizeof(manager->info.mode) - 1);
    strncpy(manager->info.ssid, "FlipperLite", sizeof(manager->info.ssid) - 1);
    strncpy(manager->info.ip, "192.168.4.1", sizeof(manager->info.ip) - 1);
    flite_manager_instance = manager;
}

bool flite_manager_start(FliteManager* manager, const char* id) {
    if(manager == NULL || id == NULL) return false;
    for(size_t i = 0; i < manager->count; i++) {
        FliteApp* app = manager->apps[i];
        if(app != NULL && app->id != NULL && strcmp(app->id, id) == 0) {
            manager->active = app;
            manager->dirty = true;
            return true;
        }
    }
    return false;
}

void flite_manager_dispatch(FliteManager* manager, const FliteEvent* event) {
    if(manager == NULL || event == NULL) return;
    if(event->type == FliteEventAppStart) {
        flite_manager_start(manager, event->text);
    } else if(manager->active != NULL) {
        flite_app_dispatch(manager->active, event);
    }
    manager->dirty = true;
}

void flite_manager_render(FliteManager* manager) {
    if(manager == NULL || manager->canvas == NULL) return;
    flite_canvas_clear(manager->canvas);
    if(manager->active != NULL) flite_app_render(manager->active, manager->canvas);
    manager->dirty = false;
}

const char* flite_manager_active_id(const FliteManager* manager) {
    if(manager == NULL || manager->active == NULL) return "none";
    return manager->active->id;
}

FliteManager* flite_manager(void) {
    return flite_manager_instance;
}
