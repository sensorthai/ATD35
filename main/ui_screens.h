/**
 * @file ui_screens.h
 * @brief Alcohol Breathalyzer UI for ATD3.5-S3 (480x320 Landscape)
 */
#pragma once
#include "lvgl.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Screen 1: Time Setting Screen */
void ui_time_screen_create(void);
void ui_show_time_screen(void);

/* Screen 2: Scan License Splash */
void ui_scan_screen_create(void);
void ui_show_scan_screen(void);

/* Screen 2: Main Screen (ID + Consent/Refuse) */
void ui_main_screen_create(void);
void ui_show_main_screen(void);

/* Shared utilities */
void ui_update_printer_status(bool connected);
void ui_show_temporary_message(const char *msg, lv_color_t color, uint32_t duration_ms);
void ui_update_id_number(const char *id);

/* Callbacks from main.c */
extern void on_time_confirmed(struct tm *t);
extern void on_consent_pressed(void);
extern void on_refuse_pressed(void);

#ifdef __cplusplus
}
#endif
