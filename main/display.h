/**
 * @file display.h
 * @brief Display and touch init for ArtronShop ATD3.5-S3 (ST7796S + FT6336U)
 */
#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// LCD SPI Pins on ATD3.5-S3
#define LCD_SPI_HOST    SPI2_HOST
#define LCD_PIXEL_CLK   (40 * 1000 * 1000)
#define LCD_BK_LIGHT    GPIO_NUM_3
#define LCD_RST         GPIO_NUM_14
#define LCD_DC          GPIO_NUM_21
#define LCD_CS          GPIO_NUM_10
#define LCD_SCLK        GPIO_NUM_12
#define LCD_MOSI        GPIO_NUM_11
#define LCD_MISO        GPIO_NUM_13

// Touch I2C Pins on ATD3.5-S3
#define TOUCH_I2C_PORT  I2C_NUM_0
#define TOUCH_SDA       GPIO_NUM_15
#define TOUCH_SCL       GPIO_NUM_16
#define TOUCH_I2C_CLK   (400 * 1000)

// Screen Resolution
#define LCD_H_RES       320
#define LCD_V_RES       480

esp_err_t display_init(void);
void display_lock(void);
void display_unlock(void);

#ifdef __cplusplus
}
#endif
