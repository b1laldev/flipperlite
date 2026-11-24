#ifndef FLITE_APPS_H
#define FLITE_APPS_H

#include "flite_canvas.h"
#include "flite_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FliteApp flite_app_menu;
extern FliteApp flite_app_badge;
extern FliteApp flite_app_wifi;
extern FliteApp flite_app_settings;

extern FliteApp* flite_apps[];
extern const size_t flite_apps_count;

void flite_apps_init(FliteCanvas* canvas);

#ifdef __cplusplus
}
#endif

#endif
