/**
 * @file ds1307.h
 * @brief DS1307 RTC driver over I2C (shared bus with touch)
 */
#pragma once
#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS1307_I2C_ADDR  0x68
#define DS1307_I2C_PORT  I2C_NUM_0

/**
 * @brief Read current time from DS1307 RTC.
 * @param t Pointer to struct tm to store the result
 * @return ESP_OK on success
 */
esp_err_t ds1307_get_time(struct tm *t);

/**
 * @brief Write time to DS1307 RTC.
 * @param t Pointer to struct tm with the time to set
 * @return ESP_OK on success
 */
esp_err_t ds1307_set_time(const struct tm *t);

/**
 * @brief Check if DS1307 is present on the I2C bus.
 * @return true if device responds at 0x68
 */
bool ds1307_is_present(void);

#ifdef __cplusplus
}
#endif
