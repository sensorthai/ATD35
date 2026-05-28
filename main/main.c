/**
 * @file main.c
 * @brief ATD3.5-S3 Alcohol Breathalyzer Test — Time Setting + Consent Flow
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "display.h"
#include "ui_screens.h"
#include "usb_printer.h"
#include "esc_pos.h"
#include "ds1307.h"
#include "card_reader.h"

static const char *TAG = "main";
static QueueHandle_t printer_event_queue = NULL;

/* ID from card reader (mutable, updated when card is swiped) */
static char s_id_number[16] = "-------------";

static void printer_event_cb(printer_event_t event)
{
    if (printer_event_queue) {
        xQueueSend(printer_event_queue, &event, 0);
    }
}

/* ========================================================================== */
/*  RTC Time Management                                                        */
/* ========================================================================== */
void on_time_confirmed(struct tm *t)
{
    ESP_LOGI(TAG, "Time confirmed: %02d:%02d %02d/%02d/%04d",
             t->tm_hour, t->tm_min, t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);

    /* Set system time via settimeofday */
    time_t epoch = mktime(t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    /* Save to DS1307 RTC (battery backed) */
    if (ds1307_is_present()) {
        ds1307_set_time(t);
    }

    /* Set timezone to Bangkok (ICT = UTC+7) */
    setenv("TZ", "ICT-7", 1);
    tzset();

    /* Switch to scan license screen (3 sec splash, then main) */
    ui_show_scan_screen();
}

/* ========================================================================== */
/*  Print Slip Tasks                                                           */
/* ========================================================================== */

/* Format A: ยินยอมเป่า */
static void print_format_a_task(void *arg)
{
    ESP_LOGI(TAG, "Printing Format A (consent)...");
    ui_show_temporary_message("Printing...", lv_palette_main(LV_PALETTE_AMBER), 5000);

    uint8_t *buf = malloc(4096);
    if (!buf) {
        ui_show_temporary_message("Mem Error!", lv_palette_main(LV_PALETTE_RED), 2000);
        vTaskDelete(NULL);
        return;
    }
    size_t len = 0;

    /* Get current time */
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    char time_str[16], date_str[40];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);

    esc_pos_init(buf, &len);
    esc_pos_select_code_page(buf, &len, 26);
    esc_pos_align(buf, &len, ALIGN_LEFT);
    esc_pos_style(buf, &len, STYLE_NORMAL);

    esc_pos_thai_text(buf, &len, "รายงานผลการตรวจวัดปริมาณแอลกอฮอล์");
    esc_pos_feed(buf, &len, 2);

    esc_pos_thai_text(buf, &len, "เลขบัตรประชาชนผู้ถูกตรวจวัด");
    esc_pos_feed(buf, &len, 1);
    esc_pos_text(buf, &len, s_id_number);
    esc_pos_feed(buf, &len, 2);

    esc_pos_thai_text(buf, &len, "เวลา : ");
    esc_pos_text(buf, &len, time_str);
    esc_pos_feed(buf, &len, 1);

    esc_pos_thai_text(buf, &len, "วันที่ : ");
    esc_pos_text(buf, &len, date_str);
    esc_pos_feed(buf, &len, 1);

    esc_pos_feed(buf, &len, 3);
    esc_pos_cut(buf, &len);

    esp_err_t ret = printer_send_data(buf, len);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Format A printed OK");
        ui_show_temporary_message("Success!", lv_palette_main(LV_PALETTE_GREEN), 2000);
    } else {
        ESP_LOGE(TAG, "Print failed: %s", esp_err_to_name(ret));
        ui_show_temporary_message("Print Failed!", lv_palette_main(LV_PALETTE_RED), 2000);
    }

    free(buf);
    vTaskDelete(NULL);
}

/* Format B: ไม่ยินยอม */
static void print_format_b_task(void *arg)
{
    ESP_LOGI(TAG, "Printing Format B (refuse)...");
    ui_show_temporary_message("Printing...", lv_palette_main(LV_PALETTE_AMBER), 5000);

    uint8_t *buf = malloc(4096);
    if (!buf) {
        ui_show_temporary_message("Mem Error!", lv_palette_main(LV_PALETTE_RED), 2000);
        vTaskDelete(NULL);
        return;
    }
    size_t len = 0;

    /* Get current time */
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    char time_str[16], date_str[40];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);

    esc_pos_init(buf, &len);
    esc_pos_select_code_page(buf, &len, 26);
    esc_pos_align(buf, &len, ALIGN_LEFT);
    esc_pos_style(buf, &len, STYLE_NORMAL);

    esc_pos_thai_text(buf, &len, "รายงานผลการตรวจวัดปริมาณแอลกอฮอล์");
    esc_pos_feed(buf, &len, 1);

    esc_pos_thai_text(buf, &len, "เลขบัตรประชาชนผู้ถูกตรวจวัด");
    esc_pos_feed(buf, &len, 1);
    esc_pos_text(buf, &len, s_id_number);
    esc_pos_feed(buf, &len, 2);

    esc_pos_thai_text(buf, &len, "ผู้ถูกตรวจวัด ");
    esc_pos_text(buf, &len, "**** ");
    esc_pos_thai_text(buf, &len, "ไม่ยินยอมเป่า");
    esc_pos_text(buf, &len, " ****");
    esc_pos_feed(buf, &len, 2);

    esc_pos_thai_text(buf, &len, "เวลา : ");
    esc_pos_text(buf, &len, time_str);
    esc_pos_feed(buf, &len, 1);

    esc_pos_thai_text(buf, &len, "วันที่ : ");
    esc_pos_text(buf, &len, date_str);
    esc_pos_feed(buf, &len, 1);

    esc_pos_feed(buf, &len, 3);
    esc_pos_cut(buf, &len);

    esp_err_t ret = printer_send_data(buf, len);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Format B printed OK");
        ui_show_temporary_message("Success!", lv_palette_main(LV_PALETTE_GREEN), 2000);
    } else {
        ESP_LOGE(TAG, "Print failed: %s", esp_err_to_name(ret));
        ui_show_temporary_message("Print Failed!", lv_palette_main(LV_PALETTE_RED), 2000);
    }

    free(buf);
    vTaskDelete(NULL);
}

/* ========================================================================== */
/*  Callbacks                                                                  */
/* ========================================================================== */
static void on_card_read(const license_data *lic)
{
    ESP_LOGI(TAG, "Card read: ID=%s", lic->card_id);
    strncpy(s_id_number, lic->card_id, sizeof(s_id_number) - 1);
    s_id_number[sizeof(s_id_number) - 1] = '\0';

    /* Update UI with new ID */
    ui_update_id_number(s_id_number);
    ui_show_main_screen();
}

void on_consent_pressed(void)
{
    ESP_LOGI(TAG, "Consent (ยินยอมเป่า) pressed");
    if (!g_printer_dev.is_opened) {
        ui_show_temporary_message("No Printer!", lv_palette_main(LV_PALETTE_RED), 2500);
        return;
    }
    xTaskCreate(print_format_a_task, "print_a", 4096, NULL, 4, NULL);
}

void on_refuse_pressed(void)
{
    ESP_LOGI(TAG, "Refuse (ไม่ยินยอม) pressed");
    if (!g_printer_dev.is_opened) {
        ui_show_temporary_message("No Printer!", lv_palette_main(LV_PALETTE_RED), 2500);
        return;
    }
    xTaskCreate(print_format_b_task, "print_b", 4096, NULL, 4, NULL);
}

/* ========================================================================== */
/*  Main Application Entry                                                     */
/* ========================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "    ATD3.5-S3 Alcohol Breathalyzer Test    ");
    ESP_LOGI(TAG, "===========================================");

    /* 1. NVS init */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. Set timezone */
    setenv("TZ", "ICT-7", 1);
    tzset();

    /* 3. Event queue */
    printer_event_queue = xQueueCreate(4, sizeof(printer_event_t));

    /* 4. Display init (also initializes I2C for touch) */
    ESP_ERROR_CHECK(display_init());

    /* 5. Read time from DS1307 RTC if available */
    if (ds1307_is_present()) {
        struct tm rtc_time;
        if (ds1307_get_time(&rtc_time) == ESP_OK) {
            time_t epoch = mktime(&rtc_time);
            struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
            settimeofday(&tv, NULL);
            ESP_LOGI(TAG, "System time synced from DS1307 RTC");
        }
    } else {
        ESP_LOGW(TAG, "DS1307 not found — using manual time setting");
    }

    /* 6. Create UI screens */
    ui_time_screen_create();
    ui_scan_screen_create();
    ui_main_screen_create();

    /* 7. Show time setting screen first */
    ui_show_time_screen();

    /* 8. Card reader init */
    card_reader_init();
    card_reader_register_cb(on_card_read);
    card_reader_start();

    /* 7. USB printer init */
    printer_register_event_cb(printer_event_cb);
    ESP_ERROR_CHECK(printer_host_init());

    /* 8. Event monitor loop */
    printer_event_t event;
    bool last_known_status = false;
    while (1) {
        if (xQueueReceive(printer_event_queue, &event, pdMS_TO_TICKS(2000))) {
            if (event == PRINTER_EVENT_CONNECTED) {
                ESP_LOGI(TAG, "USB printer connected!");
                last_known_status = true;
                ui_update_printer_status(true);
            } else if (event == PRINTER_EVENT_DISCONNECTED) {
                ESP_LOGI(TAG, "USB printer removed.");
                last_known_status = false;
                ui_update_printer_status(false);
            }
        }

        bool current_status = g_printer_dev.is_opened;
        if (current_status != last_known_status) {
            last_known_status = current_status;
            ui_update_printer_status(current_status);
        }
    }
}
