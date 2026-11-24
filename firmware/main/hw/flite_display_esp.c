#include "flite_display_esp.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)

static const char* TAG = "flite/oled";
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t oled_dev;
static FliteCanvas canvas;
static uint8_t frame[1 + OLED_WIDTH * OLED_PAGES];

void flite_display_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_FLITE_OLED_SDA,
        .scl_io_num = CONFIG_FLITE_OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_FLITE_OLED_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_config, &oled_dev));

    const uint8_t init[] = {
        0x00,       /* control: command stream */
        0xAE,       /* display off */
        0xD5, 0x80, /* clock divide */
        0xA8, 0x3F, /* 64 rows */
        0xD3, 0x00, /* no offset */
        0x40,       /* start line 0 */
        0x8D, 0x14, /* charge pump on */
        0x20, 0x00, /* horizontal addressing */
        0xA1,       /* segment remap */
        0xC8,       /* reverse scan */
        0xDA, 0x12, /* COM pins */
        0x81, 0xCF, /* contrast */
        0xA4,       /* RAM to display */
        0xA6,       /* normal video */
        0xAF,       /* display on */
    };
    ESP_ERROR_CHECK(i2c_master_transmit(oled_dev, init, sizeof(init), 200));
    flite_canvas_clear(&canvas);
    flite_display_refresh();
    ESP_LOGI(TAG, "SSD1306 ready at 0x%02X", CONFIG_FLITE_OLED_ADDR);
}

FliteCanvas* flite_display_canvas(void) {
    return &canvas;
}

void flite_display_refresh(void) {
    const uint8_t setup[] = {0x00, 0x21, 0x00, OLED_WIDTH - 1, 0x22, 0x00, OLED_PAGES - 1};
    if(i2c_master_transmit(oled_dev, setup, sizeof(setup), 100) != ESP_OK) return;

    frame[0] = 0x40; /* control: data stream */
    for(int page = 0; page < OLED_PAGES; page++) {
        for(int x = 0; x < OLED_WIDTH; x++) {
            uint8_t byte = 0;
            for(int bit = 0; bit < 8; bit++) {
                if(flite_canvas_get_pixel(&canvas, x, page * 8 + bit)) byte |= (uint8_t)(1u << bit);
            }
            frame[1 + page * OLED_WIDTH + x] = byte;
        }
    }
    i2c_master_transmit(oled_dev, frame, sizeof(frame), 200);
}
