#include <stdio.h>
#include <string.h>

#include "flite_apps.h"
#include "flite_canvas.h"
#include "flite_manager.h"
#include "flite_protocol.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        checks++;                                                                \
        if(!(cond)) {                                                            \
            failures++;                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                        \
    } while(0)

static void test_canvas(void) {
    FliteCanvas canvas;
    flite_canvas_clear(&canvas);
    for(size_t i = 0; i < sizeof(canvas.pixels); i++) CHECK(canvas.pixels[i] == 0);

    CHECK(flite_canvas_text_width("ABC") == 22);
    CHECK(flite_canvas_text_width("") == 0);

    flite_canvas_pixel(&canvas, 10, 9, true);
    CHECK(flite_canvas_get_pixel(&canvas, 10, 9));
    flite_canvas_pixel(&canvas, 10, 9, false);
    CHECK(!flite_canvas_get_pixel(&canvas, 10, 9));
    flite_canvas_pixel(&canvas, -1, 0, true); /* no crash on out-of-bounds */
    flite_canvas_pixel(&canvas, 128, 0, true);

    flite_canvas_rect(&canvas, 2, 2, 10, 6, true);
    CHECK(flite_canvas_get_pixel(&canvas, 2, 2));
    CHECK(flite_canvas_get_pixel(&canvas, 11, 7));
    CHECK(!flite_canvas_get_pixel(&canvas, 12, 7));

    flite_canvas_clear(&canvas);
    flite_canvas_text(&canvas, 0, 0, "HELLO", false);
    bool any_pixel = false;
    for(size_t i = 0; i < sizeof(canvas.pixels); i++) any_pixel |= canvas.pixels[i] != 0;
    CHECK(any_pixel);

    flite_canvas_clear(&canvas);
    flite_canvas_header(&canvas, "MENU");
    CHECK(flite_canvas_get_pixel(&canvas, 0, 0));
    CHECK(flite_canvas_get_pixel(&canvas, 127, 9));
    CHECK(!flite_canvas_get_pixel(&canvas, 0, 11));

    flite_canvas_battery(&canvas, 110, 2, 50);
    CHECK(flite_canvas_get_pixel(&canvas, 110, 2));
    CHECK(flite_canvas_get_pixel(&canvas, 126, 4));
}

static void test_protocol(void) {
    FliteEvent event;
    memset(&event, 0, sizeof(event));
    event.type = FliteEventInput;
    event.input.key = FliteKeyOk;
    event.input.type = FliteInputShort;
    event.input.ts_ms = 1234;

    char buf[256];
    CHECK(flite_event_to_json(&event, buf, sizeof(buf)));
    CHECK(strstr(buf, "\"type\":\"input\"") != NULL);
    CHECK(strstr(buf, "\"key\":\"ok\"") != NULL);
    CHECK(strstr(buf, "\"action\":\"short\"") != NULL);

    FliteInputEvent input;
    CHECK(flite_parse_input_json("{\"key\":\"down\",\"action\":\"long\"}", &input));
    CHECK(input.key == FliteKeyDown);
    CHECK(input.type == FliteInputLong);
    CHECK(!flite_parse_input_json("{\"key\":\"nope\",\"action\":\"short\"}", &input));
    CHECK(!flite_parse_input_json("not json", &input));

    char id[16];
    CHECK(flite_parse_id_json("{\"id\":\"badge\"}", id, sizeof(id)));
    CHECK(strcmp(id, "badge") == 0);
    CHECK(!flite_parse_id_json("{\"id\":\"too-long-for-buffer\"}", id, sizeof(id)));

    char ssid[32], pass[32];
    CHECK(flite_parse_wifi_json("{\"ssid\":\"MYNET\",\"password\":\"secret\"}",
                                ssid, sizeof(ssid), pass, sizeof(pass)));
    CHECK(strcmp(ssid, "MYNET") == 0 && strcmp(pass, "secret") == 0);

    CHECK(strcmp(flite_key_name(FliteKeyBack), "back") == 0);
    CHECK(flite_key_from_name("up", &input.key) && input.key == FliteKeyUp);
    CHECK(flite_base64_encode((const uint8_t*)"f", 1, buf, sizeof(buf)) == 4);
    CHECK(strcmp(buf, "Zg==") == 0);
    CHECK(flite_base64_encode((const uint8_t*)"fo", 2, buf, sizeof(buf)) == 4);
    CHECK(strcmp(buf, "Zm8=") == 0);
    CHECK(flite_base64_encode((const uint8_t*)"foo", 3, buf, sizeof(buf)) == 4);
    CHECK(strcmp(buf, "Zm9v") == 0);
}

static void test_manager(void) {
    static FliteCanvas canvas;
    static FliteManager manager;
    flite_apps_init(&canvas);
    flite_manager_init(&manager, flite_apps, flite_apps_count, &canvas);

    CHECK(!flite_manager_start(&manager, "missing"));
    CHECK(flite_manager_start(&manager, "menu"));
    CHECK(strcmp(flite_manager_active_id(&manager), "menu") == 0);

    FliteEvent event;
    memset(&event, 0, sizeof(event));
    event.type = FliteEventInput;
    event.input.type = FliteInputShort;
    event.input.key = FliteKeyDown;
    flite_manager_dispatch(&manager, &event); /* move selection to Badge */
    event.input.key = FliteKeyOk;
    flite_manager_dispatch(&manager, &event); /* launch it */
    CHECK(strcmp(flite_manager_active_id(&manager), "badge") == 0);
    CHECK(manager.active->current_scene == 0);

    flite_manager_render(&manager);
    bool any_pixel = false;
    for(size_t i = 0; i < sizeof(canvas.pixels); i++) any_pixel |= canvas.pixels[i] != 0;
    CHECK(any_pixel);
    CHECK(!manager.dirty);

    event.type = FliteEventAppStart;
    strncpy(event.text, "wifi", sizeof(event.text));
    flite_manager_dispatch(&manager, &event);
    CHECK(strcmp(flite_manager_active_id(&manager), "wifi") == 0);
}

int main(void) {
    test_canvas();
    test_protocol();
    test_manager();
    printf("host tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
