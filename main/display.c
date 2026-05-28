/**
 * @file display.c
 * @brief LCD (ST7796S SPI) + Touch (FT6336U I2C) + LVGL init for ATD3.5-S3
 */
#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lvgl_port.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

static const char *TAG = "display";
static lv_disp_t *s_disp = NULL;

static void backlight_on(void)
{
    gpio_config_t cfg = { .mode = GPIO_MODE_OUTPUT,
                          .pin_bit_mask = 1ULL << LCD_BK_LIGHT };
    gpio_config(&cfg);
    gpio_set_level(LCD_BK_LIGHT, 1);
}

esp_err_t display_init(void)
{
    backlight_on();

    /* SPI bus */
    spi_bus_config_t bus = {
        .sclk_io_num = LCD_SCLK, .mosi_io_num = LCD_MOSI,
        .miso_io_num = LCD_MISO, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    /* LCD panel IO */
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_DC, .cs_gpio_num = LCD_CS,
        .pclk_hz = LCD_PIXEL_CLK, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .spi_mode = 0, .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &io));

    /* LCD panel */
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST, .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    // Screen is in landscape layout with correct mirroring
    bool swap_xy = true;
    bool mirror_x = false;
    bool mirror_y = false;

    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, swap_xy));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, mirror_x, mirror_y));

    /* Touch I2C */
    i2c_config_t i2c = {
        .mode = I2C_MODE_MASTER, .sda_io_num = TOUCH_SDA, .scl_io_num = TOUCH_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TOUCH_I2C_CLK,
    };
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_I2C_PORT, &i2c));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    // x_max/y_max = RAW touch panel resolution (portrait), NOT post-swap display size.
    // The esp_lcd_touch driver applies: mirror FIRST, swap SECOND.
    // FT6336U raw: X=0..319 (short), Y=0..479 (long)
    esp_lcd_touch_config_t tp_cfg = { .x_max = LCD_H_RES,   // 320 (raw X)
                                       .y_max = LCD_V_RES,   // 480 (raw Y)
                                       .rst_gpio_num = -1, .int_gpio_num = -1 };
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_cfg.scl_speed_hz = 0; // Legacy I2C driver requires this to be 0
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_I2C_PORT,
                                              &tp_io_cfg, &tp_io));
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &touch));

    // Driver order: 1) mirror_x(x_max) 2) mirror_y(y_max) 3) swap_xy
    // ArtronShop ref rotation 0: final_x = raw_y, final_y = 320 - raw_x
    // With mirror_x=true: x' = 320 - raw_x. Then swap: final_x=raw_y, final_y=320-raw_x ✓
    esp_lcd_touch_set_swap_xy(touch, true);
    esp_lcd_touch_set_mirror_x(touch, true);
    esp_lcd_touch_set_mirror_y(touch, false);

    /* LVGL */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    uint16_t hres = swap_xy ? LCD_V_RES : LCD_H_RES;
    uint16_t vres = swap_xy ? LCD_H_RES : LCD_V_RES;

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io, .panel_handle = panel, .buffer_size = LCD_H_RES * 50,
        .double_buffer = true, .hres = hres, .vres = vres,
        .rotation = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        }
    };
    s_disp = lvgl_port_add_disp(&disp_cfg);

    const lvgl_port_touch_cfg_t touch_cfg = { .disp = s_disp, .handle = touch };
    lvgl_port_add_touch(&touch_cfg);

    /* Dark theme */
    lv_theme_t *th = lv_theme_default_init(s_disp,
        lv_palette_main(LV_PALETTE_AMBER), lv_palette_main(LV_PALETTE_ORANGE),
        true, LV_FONT_DEFAULT);
    lv_disp_set_theme(s_disp, th);

    ESP_LOGI(TAG, "Display + touch + LVGL initialized successfully");
    return ESP_OK;
}

void display_lock(void)   { lvgl_port_lock(0); }
void display_unlock(void) { lvgl_port_unlock(); }
