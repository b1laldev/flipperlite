#include "flite_apps.h"

#include <stdio.h>
#include <string.h>

#include "flite_manager.h"

typedef struct {
    size_t selected;
} MenuCtx;

typedef struct {
    bool reveal;
} BadgeCtx;

typedef struct {
    bool demo_mode;
} SettingsCtx;

static MenuCtx menu_ctx;
static BadgeCtx badge_ctx;
static SettingsCtx settings_ctx;

static void clip(char* dst, size_t len, const char* src) {
    if(len == 0) return;
    strncpy(dst, src, len - 1);
    dst[len - 1] = '\0';
}

/* ---------------- Menu ---------------- */

static bool menu_event(FliteApp* app, const FliteEvent* event) {
    MenuCtx* ctx = (MenuCtx*)app->ctx;
    if(event->type != FliteEventInput) return false;
    if(event->input.key == FliteKeyUp || event->input.key == FliteKeyLeft) {
        ctx->selected = (ctx->selected + flite_apps_count - 1) % flite_apps_count;
        return true;
    }
    if(event->input.key == FliteKeyDown || event->input.key == FliteKeyRight) {
        ctx->selected = (ctx->selected + 1) % flite_apps_count;
        return true;
    }
    if(event->input.key == FliteKeyOk) {
        flite_manager_start(flite_manager(), flite_apps[ctx->selected]->id);
        return true;
    }
    return false;
}

static void menu_render(FliteApp* app, FliteCanvas* canvas) {
    MenuCtx* ctx = (MenuCtx*)app->ctx;
    flite_canvas_header(canvas, "APPS");
    for(size_t i = 0; i < flite_apps_count; i++) {
        char line[32];
        snprintf(line, sizeof(line), "%s %s", i == ctx->selected ? ">" : " ", flite_apps[i]->name);
        flite_canvas_text(canvas, 4, 14 + (int)i * 12, line, false);
    }
}

/* ---------------- Badge ---------------- */

static bool badge_event(FliteApp* app, const FliteEvent* event) {
    BadgeCtx* ctx = (BadgeCtx*)app->ctx;
    if(event->type == FliteEventInput && event->input.key == FliteKeyOk && app->current_scene == 0) {
        ctx->reveal = !ctx->reveal;
        return true;
    }
    return false;
}

static void badge_render(FliteApp* app, FliteCanvas* canvas) {
    BadgeCtx* ctx = (BadgeCtx*)app->ctx;
    if(app->current_scene == 0) {
        flite_canvas_header(canvas, "BADGE DEMO");
        flite_canvas_text(canvas, 4, 16, "OWN NETWORK ONLY", false);
        flite_canvas_text(canvas, 4, 28, "SSID LAB-WIFI-TEST", false);
        char pin[24];
        snprintf(pin, sizeof(pin), "PIN %s", ctx->reveal ? "12345670" : "********");
        flite_canvas_text(canvas, 4, 40, pin, false);
        flite_canvas_text(canvas, 4, 52, "OK: SHOW/HIDE PIN", false);
    } else {
        flite_canvas_header(canvas, "ABOUT");
        flite_canvas_text(canvas, 4, 18, "FLIPPERLITE", false);
        flite_canvas_text(canvas, 4, 30, "ESP32 PORT", false);
        flite_canvas_text(canvas, 4, 42, "V" FLITE_VERSION, false);
    }
}

/* ---------------- WiFi ---------------- */

static void wifi_render(FliteApp* app, FliteCanvas* canvas) {
    (void)app;
    FliteDeviceInfo* info = &flite_manager()->info;
    char line[24];
    char ssid[16];
    flite_canvas_header(canvas, "WIFI STATUS");
    snprintf(line, sizeof(line), "MODE %s", info->mode);
    flite_canvas_text(canvas, 4, 16, line, false);
    clip(ssid, sizeof(ssid), info->ssid);
    snprintf(line, sizeof(line), "SSID %s", ssid);
    flite_canvas_text(canvas, 4, 28, line, false);
    snprintf(line, sizeof(line), "IP %s", info->ip);
    flite_canvas_text(canvas, 4, 40, line, false);
    flite_canvas_text(canvas, 4, 52, "BRIDGE HTTP 8080", false);
}

/* ---------------- Settings ---------------- */

static bool settings_event(FliteApp* app, const FliteEvent* event) {
    SettingsCtx* ctx = (SettingsCtx*)app->ctx;
    if(event->type != FliteEventInput) return false;
    if(event->input.key == FliteKeyUp || event->input.key == FliteKeyDown) {
        flite_app_set_scene(app, app->current_scene == 0 ? 1 : 0);
        return true;
    }
    if(event->input.key == FliteKeyOk && app->current_scene == 0) {
        ctx->demo_mode = !ctx->demo_mode;
        return true;
    }
    return false;
}

static void settings_render(FliteApp* app, FliteCanvas* canvas) {
    SettingsCtx* ctx = (SettingsCtx*)app->ctx;
    if(app->current_scene == 0) {
        flite_canvas_header(canvas, "SETTINGS");
        char line[24];
        snprintf(line, sizeof(line), "DEMO %s", ctx->demo_mode ? "ON" : "OFF");
        flite_canvas_text(canvas, 4, 18, line, false);
        flite_canvas_text(canvas, 4, 32, "UP/DOWN SELECT", false);
        flite_canvas_text(canvas, 4, 46, "OK TOGGLE", false);
    } else {
        flite_canvas_header(canvas, "ABOUT");
        flite_canvas_text(canvas, 4, 20, "GPL-3.0 UPSTREAM", false);
        flite_canvas_text(canvas, 4, 34, "MIT THIS PORT", false);
    }
    flite_canvas_battery(canvas, 110, 2, 75);
}

/* ---------------- Scene tables and registry ---------------- */

static const FliteScene menu_scene_list = {"list", NULL, menu_event, menu_render, NULL};
static const FliteScene badge_scene_card = {"card", NULL, badge_event, badge_render, NULL};
static const FliteScene badge_scene_about = {"about", NULL, NULL, badge_render, NULL};
static const FliteScene wifi_scene_status = {"status", NULL, NULL, wifi_render, NULL};
static const FliteScene settings_scene_list = {"list", NULL, settings_event, settings_render, NULL};
static const FliteScene settings_scene_about = {"about", NULL, NULL, settings_render, NULL};

static const FliteScene* const menu_scene_ptrs[] = {&menu_scene_list};
static const FliteScene* const badge_scene_ptrs[] = {&badge_scene_card, &badge_scene_about};
static const FliteScene* const wifi_scene_ptrs[] = {&wifi_scene_status};
static const FliteScene* const settings_scene_ptrs[] = {&settings_scene_list, &settings_scene_about};

FliteApp flite_app_menu = {"menu", "Apps", &menu_ctx, menu_scene_ptrs, 1, 0};
FliteApp flite_app_badge = {"badge", "Badge", &badge_ctx, badge_scene_ptrs, 2, 0};
FliteApp flite_app_wifi = {"wifi", "WiFi", NULL, wifi_scene_ptrs, 1, 0};
FliteApp flite_app_settings = {"settings", "Settings", &settings_ctx, settings_scene_ptrs, 2, 0};

FliteApp* flite_apps[] = {
    &flite_app_menu, &flite_app_badge, &flite_app_wifi, &flite_app_settings,
};
const size_t flite_apps_count = sizeof(flite_apps) / sizeof(flite_apps[0]);

void flite_apps_init(FliteCanvas* canvas) {
    (void)canvas;
    memset(&menu_ctx, 0, sizeof(menu_ctx));
    memset(&badge_ctx, 0, sizeof(badge_ctx));
    memset(&settings_ctx, 0, sizeof(settings_ctx));
    flite_app_menu.current_scene = 0;
    flite_app_badge.current_scene = 0;
    flite_app_wifi.current_scene = 0;
    flite_app_settings.current_scene = 0;
}
