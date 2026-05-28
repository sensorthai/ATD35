/**
 * @file ds1307.c
 * @brief DS1307 RTC driver — BCD register read/write over legacy I2C
 */
#include "ds1307.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "ds1307";

/* BCD helpers */
static uint8_t bcd_to_dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static uint8_t dec_to_bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

bool ds1307_is_present(void)
{
    /* Try reading 1 byte from register 0x00 */
    uint8_t reg = 0x00;
    uint8_t data = 0;
    esp_err_t err = i2c_master_write_read_device(DS1307_I2C_PORT, DS1307_I2C_ADDR,
                                                  &reg, 1, &data, 1, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "DS1307 detected at 0x%02X", DS1307_I2C_ADDR);
        return true;
    }
    ESP_LOGW(TAG, "DS1307 not found at 0x%02X: %s", DS1307_I2C_ADDR, esp_err_to_name(err));
    return false;
}

esp_err_t ds1307_get_time(struct tm *t)
{
    uint8_t reg = 0x00;
    uint8_t data[7] = {0};

    esp_err_t err = i2c_master_write_read_device(DS1307_I2C_PORT, DS1307_I2C_ADDR,
                                                  &reg, 1, data, 7, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read DS1307: %s", esp_err_to_name(err));
        return err;
    }

    /* DS1307 registers:
     * [0] Seconds (bit7=CH), [1] Minutes, [2] Hours (bit6=12/24),
     * [3] Day-of-week, [4] Date, [5] Month, [6] Year (00-99) */
    t->tm_sec  = bcd_to_dec(data[0] & 0x7F);
    t->tm_min  = bcd_to_dec(data[1] & 0x7F);
    t->tm_hour = bcd_to_dec(data[2] & 0x3F); // 24-hour mode
    t->tm_mday = bcd_to_dec(data[4] & 0x3F);
    t->tm_mon  = bcd_to_dec(data[5] & 0x1F) - 1; // 0-based
    t->tm_year = bcd_to_dec(data[6]) + 100;        // years since 1900 (2000+)
    t->tm_wday = bcd_to_dec(data[3] & 0x07);
    t->tm_isdst = -1;

    ESP_LOGI(TAG, "Read: %02d:%02d:%02d %02d/%02d/%04d",
             t->tm_hour, t->tm_min, t->tm_sec,
             t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    return ESP_OK;
}

esp_err_t ds1307_set_time(const struct tm *t)
{
    uint8_t data[8];
    data[0] = 0x00; // Start register address
    data[1] = dec_to_bcd(t->tm_sec) & 0x7F;  // Clear CH bit (enable oscillator)
    data[2] = dec_to_bcd(t->tm_min);
    data[3] = dec_to_bcd(t->tm_hour) & 0x3F; // 24-hour mode
    data[4] = dec_to_bcd(t->tm_wday > 0 ? t->tm_wday : 1);
    data[5] = dec_to_bcd(t->tm_mday);
    data[6] = dec_to_bcd(t->tm_mon + 1);      // 1-based
    data[7] = dec_to_bcd((t->tm_year + 1900) % 100);  // 2-digit year

    esp_err_t err = i2c_master_write_to_device(DS1307_I2C_PORT, DS1307_I2C_ADDR,
                                                data, 8, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write DS1307: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Set: %02d:%02d:%02d %02d/%02d/%04d",
             t->tm_hour, t->tm_min, t->tm_sec,
             t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    return ESP_OK;
}
