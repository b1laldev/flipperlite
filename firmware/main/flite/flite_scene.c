#include "flite_scene.h"

void flite_app_set_scene(FliteApp* app, size_t index) {
    if(app == NULL || index >= app->scene_count || index == app->current_scene) return;
    if(app->scenes[app->current_scene] != NULL && app->scenes[app->current_scene]->on_exit != NULL) {
        app->scenes[app->current_scene]->on_exit(app);
    }
    app->current_scene = index;
    if(app->scenes[index] != NULL && app->scenes[index]->on_enter != NULL) {
        app->scenes[index]->on_enter(app);
    }
}

void flite_app_dispatch(FliteApp* app, const FliteEvent* event) {
    if(app == NULL || event == NULL || app->scene_count == 0) return;
    const FliteScene* scene = app->scenes[app->current_scene];
    if(scene != NULL && scene->on_event != NULL && scene->on_event(app, event)) return;
    /* Default: long/back returns to the first scene of the app. */
    if(event->type == FliteEventInput && event->input.key == FliteKeyBack) {
        flite_app_set_scene(app, 0);
    }
}

void flite_app_render(FliteApp* app, FliteCanvas* canvas) {
    if(app == NULL || canvas == NULL || app->scene_count == 0) return;
    const FliteScene* scene = app->scenes[app->current_scene];
    if(scene != NULL && scene->on_render != NULL) scene->on_render(app, canvas);
}
