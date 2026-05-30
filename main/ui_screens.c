/**
 * @file ui_screens.c
 * @brief Alcohol Breathalyzer UI — Time Setting + Consent Flow (480x320 Landscape)
 */
#include "ui_screens.h"
#include "display.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>



/* Custom Thai fonts */
LV_FONT_DECLARE(font_thai_16);
LV_FONT_DECLARE(font_thai_24);

/* Modern Color Palette */
#define C_BG            lv_color_hex(0x090D16)
#define C_CARD          lv_color_hex(0x131A26)
#define C_BORDER        lv_color_hex(0x27354A)
#define C_AMBER         lv_color_hex(0xF59E0B)
#define C_GREEN         lv_color_hex(0x10B981)
#define C_RED           lv_color_hex(0xEF4444)
#define C_TEXT          lv_color_hex(0xF8FAFC)
#define C_DIM           lv_color_hex(0x64748B)
#define C_BLUE          lv_color_hex(0x3B82F6)

/* ========================================================================== */
/*  Screen 1: Time Setting                                                     */
/* ========================================================================== */
static lv_obj_t *scr_time = NULL;

/* Time edit fields */
static int s_hour = 12, s_min = 0;
static int s_day = 1, s_month = 1, s_year = 2026;

static lv_obj_t *lbl_time_display = NULL;
static lv_obj_t *lbl_date_display = NULL;

static void refresh_time_display(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d : %02d", s_hour, s_min);
    lv_label_set_text(lbl_time_display, buf);

    snprintf(buf, sizeof(buf), "%02d / %02d / %04d", s_day, s_month, s_year);
    lv_label_set_text(lbl_date_display, buf);
}

static void btn_time_event(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);
    // action: 1=hour+, 2=hour-, 3=min+, 4=min-
    //         5=day+, 6=day-, 7=month+, 8=month-, 9=year+, 10=year-
    switch (action) {
        case 1: s_hour = (s_hour + 1) % 24; break;
        case 2: s_hour = (s_hour + 23) % 24; break;
        case 3: s_min = (s_min + 1) % 60; break;
        case 4: s_min = (s_min + 59) % 60; break;
        case 5: s_day = (s_day % 31) + 1; break;
        case 6: s_day = s_day <= 1 ? 31 : s_day - 1; break;
        case 7: s_month = (s_month % 12) + 1; break;
        case 8: s_month = s_month <= 1 ? 12 : s_month - 1; break;
        case 9: s_year++; break;
        case 10: if (s_year > 2024) s_year--; break;
    }
    refresh_time_display();
}

static void btn_confirm_time(lv_event_t *e)
{
    struct tm t = {
        .tm_hour = s_hour,
        .tm_min = s_min,
        .tm_sec = 0,
        .tm_mday = s_day,
        .tm_mon = s_month - 1,  // 0-based
        .tm_year = s_year - 1900,
    };
    on_time_confirmed(&t);
}

/* Helper: create +/- button pair vertically */
static void create_adj_buttons(lv_obj_t *parent, int x, int y,
                                int action_plus, int action_minus,
                                int btn_w, int btn_h)
{
    lv_obj_t *btn_up = lv_btn_create(parent);
    lv_obj_set_size(btn_up, btn_w, btn_h);
    lv_obj_set_pos(btn_up, x, y);
    lv_obj_set_style_bg_color(btn_up, C_BLUE, 0);
    lv_obj_set_style_radius(btn_up, 8, 0);
    lv_obj_add_event_cb(btn_up, btn_time_event, LV_EVENT_CLICKED, (void *)(intptr_t)action_plus);
    lv_obj_t *lbl_up = lv_label_create(btn_up);
    lv_label_set_text(lbl_up, LV_SYMBOL_UP);
    lv_obj_center(lbl_up);

    lv_obj_t *btn_dn = lv_btn_create(parent);
    lv_obj_set_size(btn_dn, btn_w, btn_h);
    lv_obj_set_pos(btn_dn, x, y + btn_h + 60);
    lv_obj_set_style_bg_color(btn_dn, C_BLUE, 0);
    lv_obj_set_style_radius(btn_dn, 8, 0);
    lv_obj_add_event_cb(btn_dn, btn_time_event, LV_EVENT_CLICKED, (void *)(intptr_t)action_minus);
    lv_obj_t *lbl_dn = lv_label_create(btn_dn);
    lv_label_set_text(lbl_dn, LV_SYMBOL_DOWN);
    lv_obj_center(lbl_dn);
}

void ui_time_screen_create(void)
{
    /* Read current system time as default */
    time_t now;
    time(&now);
    struct tm *tm_now = localtime(&now);
    if (tm_now->tm_year > 100) { // valid time
        s_hour = tm_now->tm_hour;
        s_min  = tm_now->tm_min;
        s_day  = tm_now->tm_mday;
        s_month = tm_now->tm_mon + 1;
        s_year = tm_now->tm_year + 1900;
    }

    scr_time = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_time, C_BG, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(scr_time);
    lv_obj_set_style_text_font(title, &font_thai_24, 0);
    lv_obj_set_style_text_color(title, C_AMBER, 0);
    lv_label_set_text(title, "\xE0\xB8\x95\xE0\xB8\xB1\xE0\xB9\x89\xE0\xB8\x87\xE0\xB8\x84\xE0\xB9\x88\xE0\xB8\xB2\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\x99\xE0\xB9\x80\xE0\xB8\xA7\xE0\xB8\xA5\xE0\xB8\xB2");
    // "ตั้งค่าวันเวลา"
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* --- TIME section (top half) --- */
    lv_obj_t *time_card = lv_obj_create(scr_time);
    lv_obj_set_size(time_card, 300, 100);
    lv_obj_align(time_card, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(time_card, C_CARD, 0);
    lv_obj_set_style_border_color(time_card, C_BORDER, 0);
    lv_obj_set_style_border_width(time_card, 1, 0);
    lv_obj_set_style_radius(time_card, 12, 0);
    lv_obj_clear_flag(time_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Time display label */
    lbl_time_display = lv_label_create(time_card);
    lv_obj_set_style_text_font(lbl_time_display, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_time_display, C_TEXT, 0);
    lv_obj_align(lbl_time_display, LV_ALIGN_CENTER, 0, 0);

    /* +/- buttons for hour and minute */
    create_adj_buttons(time_card, 30, -5, 1, 2, 50, 30);   // hour +/-
    create_adj_buttons(time_card, 210, -5, 3, 4, 50, 30);  // min +/-

    /* --- DATE section (middle) --- */
    lv_obj_t *date_card = lv_obj_create(scr_time);
    lv_obj_set_size(date_card, 300, 100);
    lv_obj_align(date_card, LV_ALIGN_TOP_MID, 0, 165);
    lv_obj_set_style_bg_color(date_card, C_CARD, 0);
    lv_obj_set_style_border_color(date_card, C_BORDER, 0);
    lv_obj_set_style_border_width(date_card, 1, 0);
    lv_obj_set_style_radius(date_card, 12, 0);
    lv_obj_clear_flag(date_card, LV_OBJ_FLAG_SCROLLABLE);

    /* Date display label */
    lbl_date_display = lv_label_create(date_card);
    lv_obj_set_style_text_font(lbl_date_display, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_date_display, C_TEXT, 0);
    lv_obj_align(lbl_date_display, LV_ALIGN_CENTER, 0, 0);

    /* +/- buttons for day, month, year */
    create_adj_buttons(date_card, 5, -5, 5, 6, 45, 30);     // day +/-
    create_adj_buttons(date_card, 115, -5, 7, 8, 45, 30);   // month +/-
    create_adj_buttons(date_card, 230, -5, 9, 10, 45, 30);  // year +/-

    refresh_time_display();

    /* --- CONFIRM button --- */
    lv_obj_t *btn_confirm = lv_btn_create(scr_time);
    lv_obj_set_size(btn_confirm, 200, 50);
    lv_obj_align(btn_confirm, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(btn_confirm, C_GREEN, 0);
    lv_obj_set_style_radius(btn_confirm, 12, 0);
    lv_obj_add_event_cb(btn_confirm, btn_confirm_time, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_confirm = lv_label_create(btn_confirm);
    lv_obj_set_style_text_font(lbl_confirm, &font_thai_24, 0);
    lv_obj_set_style_text_color(lbl_confirm, C_TEXT, 0);
    lv_label_set_text(lbl_confirm, "Confirm");
    lv_obj_center(lbl_confirm);
}

void ui_show_time_screen(void)
{
    display_lock();
    lv_scr_load(scr_time);
    display_unlock();
}

/* ========================================================================== */
/*  Screen 2: "กรุณารูดใบขับขี่" Splash (3 seconds)                            */
/* ========================================================================== */
static lv_obj_t *scr_scan = NULL;
static lv_timer_t *scan_timer = NULL;

static void scan_timer_cb(lv_timer_t *timer)
{
    if (scan_timer) {
        lv_timer_del(scan_timer);
        scan_timer = NULL;
    }
    ui_show_main_screen();
}

void ui_scan_screen_create(void)
{
    scr_scan = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_scan, C_BG, 0);

    /* Card container */
    lv_obj_t *card = lv_obj_create(scr_scan);
    lv_obj_set_size(card, 300, 160);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, C_CARD, 0);
    lv_obj_set_style_border_color(card, C_AMBER, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    lv_obj_t *icon = lv_label_create(card);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(icon, C_AMBER, 0);
    lv_label_set_text(icon, LV_SYMBOL_SD_CARD);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -30);

    /* Thai text: "กรุณารูดใบขับขี่" */
    lv_obj_t *lbl = lv_label_create(card);
    lv_obj_set_style_text_font(lbl, &font_thai_24, 0);
    lv_obj_set_style_text_color(lbl, C_TEXT, 0);
    lv_label_set_text(lbl, "\xE0\xB8\x81\xE0\xB8\xA3\xE0\xB8\xB8\xE0\xB8\x93\xE0\xB8\xB2\xE0\xB8\xA3\xE0\xB8\xB9\xE0\xB8\x94\xE0\xB9\x83\xE0\xB8\x9A\xE0\xB8\x82\xE0\xB8\xB1\xE0\xB8\x9A\xE0\xB8\x82\xE0\xB8\xB5\xE0\xB9\x88");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 20);
}

void ui_show_scan_screen(void)
{
    display_lock();
    lv_scr_load(scr_scan);
    /* Auto-transition to main screen after 3 seconds */
    scan_timer = lv_timer_create(scan_timer_cb, 3000, NULL);
    lv_timer_set_repeat_count(scan_timer, 1);
    display_unlock();
}

/* ========================================================================== */
/*  Screen 3: Main Screen — ID Display + Consent/Refuse                        */
/* ========================================================================== */
static lv_obj_t *scr_main = NULL;
static lv_obj_t *lbl_id_number = NULL;
static lv_obj_t *lbl_name = NULL;
static lv_obj_t *lbl_datetime = NULL;
static lv_obj_t *status_dot = NULL;
static lv_obj_t *lbl_status = NULL;
static lv_obj_t *lbl_msg = NULL;

/* Temporary message timer */
static lv_timer_t *msg_timer = NULL;

static void clear_msg_timer_cb(lv_timer_t *timer)
{
    lv_label_set_text(lbl_msg, "");
    if (msg_timer) {
        lv_timer_del(msg_timer);
        msg_timer = NULL;
    }
}

void ui_show_temporary_message(const char *msg, lv_color_t color, uint32_t duration_ms)
{
    display_lock();
    if (lbl_msg) {
        lv_label_set_text(lbl_msg, msg);
        lv_obj_set_style_text_color(lbl_msg, color, 0);
    }
    if (msg_timer) {
        lv_timer_del(msg_timer);
    }
    msg_timer = lv_timer_create(clear_msg_timer_cb, duration_ms, NULL);
    display_unlock();
}

/* Update clock on main screen */
static lv_timer_t *clock_timer = NULL;

static void clock_update_cb(lv_timer_t *timer)
{
    if (!lbl_datetime) return;
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    char buf[80];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d  %02d/%02d/%04d",
             t->tm_hour, t->tm_min, t->tm_sec,
             t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    lv_label_set_text(lbl_datetime, buf);
}

static void btn_consent_event(lv_event_t *e)
{
    on_consent_pressed();
}

static void btn_refuse_event(lv_event_t *e)
{
    on_refuse_pressed();
}

void ui_main_screen_create(void)
{
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, C_BG, 0);

    /* --- Top: Title --- */
    lv_obj_t *title = lv_label_create(scr_main);
    lv_obj_set_style_text_font(title, &font_thai_16, 0);
    lv_obj_set_style_text_color(title, C_AMBER, 0);
    // "ตรวจวัดปริมาณแอลกอฮอล์"
    lv_label_set_text(title, "\xE0\xB8\x95\xE0\xB8\xA3\xE0\xB8\xA7\xE0\xB8\x88\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\x94\xE0\xB8\x9B\xE0\xB8\xA3\xE0\xB8\xB4\xE0\xB8\xA1\xE0\xB8\xB2\xE0\xB8\x93\xE0\xB9\x81\xE0\xB8\xAD\xE0\xB8\xA5\xE0\xB8\x81\xE0\xB8\xAD\xE0\xB8\xAE\xE0\xB8\xAD\xE0\xB8\xA5\xE0\xB9\x8C");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    /* --- ID Card Section --- */
    lv_obj_t *id_card = lv_obj_create(scr_main);
    lv_obj_set_size(id_card, 450, 120);
    lv_obj_align(id_card, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(id_card, C_CARD, 0);
    lv_obj_set_style_border_color(id_card, C_AMBER, 0);
    lv_obj_set_style_border_width(id_card, 2, 0);
    lv_obj_set_style_radius(id_card, 12, 0);
    lv_obj_set_style_pad_all(id_card, 10, 0);
    lv_obj_clear_flag(id_card, LV_OBJ_FLAG_SCROLLABLE);

    /* ID label */
    lv_obj_t *lbl_id_title = lv_label_create(id_card);
    lv_obj_set_style_text_font(lbl_id_title, &font_thai_16, 0);
    lv_obj_set_style_text_color(lbl_id_title, C_DIM, 0);
    // "เลขบัตรประชาชน"
    lv_label_set_text(lbl_id_title, "\xE0\xB9\x80\xE0\xB8\xA5\xE0\xB8\x82\xE0\xB8\x9A\xE0\xB8\xB1\xE0\xB8\x95\xE0\xB8\xA3\xE0\xB8\x9B\xE0\xB8\xA3\xE0\xB8\xB0\xE0\xB8\x8A\xE0\xB8\xB2\xE0\xB8\x8A\xE0\xB8\x99");
    lv_obj_align(lbl_id_title, LV_ALIGN_TOP_MID, 0, -2);

    /* ID number (large) */
    lbl_id_number = lv_label_create(id_card);
    lv_obj_set_style_text_font(lbl_id_number, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_id_number, C_TEXT, 0);
    lv_label_set_text(lbl_id_number, "1521253654587");
    lv_obj_align(lbl_id_number, LV_ALIGN_TOP_MID, 0, 22);

    /* Name label */
    lbl_name = lv_label_create(id_card);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_name, C_AMBER, 0);
    lv_label_set_text(lbl_name, "AKARADEJ PONGPANIT");
    lv_obj_align(lbl_name, LV_ALIGN_TOP_MID, 0, 65);

    /* --- Date/Time display --- */
    lbl_datetime = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_datetime, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_datetime, C_TEXT, 0);
    lv_label_set_text(lbl_datetime, "--:--:--  --/--/----");
    lv_obj_align(lbl_datetime, LV_ALIGN_TOP_RIGHT, -15, 8);

    /* --- Two consent buttons --- */
    // "ยินยอมตรวจวัด" button (green)
    lv_obj_t *btn_consent = lv_btn_create(scr_main);
    lv_obj_set_size(btn_consent, 240, 55);
    lv_obj_align(btn_consent, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(btn_consent, C_GREEN, 0);
    lv_obj_set_style_radius(btn_consent, 12, 0);
    lv_obj_add_event_cb(btn_consent, btn_consent_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_consent = lv_label_create(btn_consent);
    lv_obj_set_style_text_font(lbl_consent, &font_thai_24, 0);
    lv_obj_set_style_text_color(lbl_consent, C_TEXT, 0);
    // "ยินยอมตรวจวัด"
    lv_label_set_text(lbl_consent, "\xE0\xB8\xA2\xE0\xB8\xB4\xE0\xB8\x99\xE0\xB8\xA2\xE0\xB8\xAD\xE0\xB8\xA1\xE0\xB8\x95\xE0\xB8\xA3\xE0\xB8\x88\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\x94");
    lv_obj_center(lbl_consent);

    // "ไม่ยินยอมตรวจวัด" button (red)
    lv_obj_t *btn_refuse = lv_btn_create(scr_main);
    lv_obj_set_size(btn_refuse, 240, 55);
    lv_obj_align(btn_refuse, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_bg_color(btn_refuse, C_RED, 0);
    lv_obj_set_style_radius(btn_refuse, 12, 0);
    lv_obj_add_event_cb(btn_refuse, btn_refuse_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_refuse = lv_label_create(btn_refuse);
    lv_obj_set_style_text_font(lbl_refuse, &font_thai_24, 0);
    lv_obj_set_style_text_color(lbl_refuse, C_TEXT, 0);
    // "ไม่ยินยอมตรวจวัด"
    lv_label_set_text(lbl_refuse, "\xE0\xB9\x84\xE0\xB8\xA1\xE0\xB9\x88\xE0\xB8\xA2\xE0\xB8\xB4\xE0\xB8\x99\xE0\xB8\xA2\xE0\xB8\xAD\xE0\xB8\xA1\xE0\xB8\x95\xE0\xB8\xA3\xE0\xB8\x88\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\x94");
    lv_obj_center(lbl_refuse);

    /* --- Message label --- */
    lbl_msg = lv_label_create(scr_main);
    lv_label_set_text(lbl_msg, "");
    lv_obj_set_style_text_font(lbl_msg, &font_thai_16, 0);
    lv_obj_align(lbl_msg, LV_ALIGN_BOTTOM_MID, 0, -40);

    /* --- Printer status --- */
    status_dot = lv_obj_create(scr_main);
    lv_obj_set_size(status_dot, 10, 10);
    lv_obj_set_style_bg_color(status_dot, C_RED, 0);
    lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(status_dot, 0, 0);
    lv_obj_align(status_dot, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    lbl_status = lv_label_create(scr_main);
    // "เครื่องพิมพ์ไม่พร้อม"
    lv_label_set_text(lbl_status, "\xE0\xB9\x80\xE0\xB8\x84\xE0\xB8\xA3\xE0\xB8\xB7\xE0\xB9\x88\xE0\xB8\xAD\xE0\xB8\x87\xE0\xB8\x9E\xE0\xB8\xB4\xE0\xB8\xA1\xE0\xB8\x9E\xE0\xB9\x8C\xE0\xB9\x84\xE0\xB8\xA1\xE0\xB9\x88\xE0\xB8\x9E\xE0\xB8\xA3\xE0\xB9\x89\xE0\xB8\xAD\xE0\xB8\xA1");
    lv_obj_set_style_text_color(lbl_status, C_RED, 0);
    lv_obj_set_style_text_font(lbl_status, &font_thai_16, 0);
    lv_obj_align_to(lbl_status, status_dot, LV_ALIGN_OUT_RIGHT_MID, 8, -1);

    /* Start clock update timer (1 second) */
    clock_timer = lv_timer_create(clock_update_cb, 1000, NULL);
}

void ui_show_main_screen(void)
{
    display_lock();
    lv_scr_load(scr_main);
    clock_update_cb(NULL); // Immediate clock refresh
    display_unlock();
}

void ui_update_printer_status(bool connected)
{
    display_lock();
    if (status_dot && lbl_status) {
        if (connected) {
            lv_obj_set_style_bg_color(status_dot, C_GREEN, 0);
            // "เครื่องพิมพ์พร้อมแล้ว"
            lv_label_set_text(lbl_status, "\xE0\xB9\x80\xE0\xB8\x84\xE0\xB8\xA3\xE0\xB8\xB7\xE0\xB9\x88\xE0\xB8\xAD\xE0\xB8\x87\xE0\xB8\x9E\xE0\xB8\xB4\xE0\xB8\xA1\xE0\xB8\x9E\xE0\xB9\x8C\xE0\xB8\x9E\xE0\xB8\xA3\xE0\xB9\x89\xE0\xB8\xAD\xE0\xB8\xA1\xE0\xB9\x81\xE0\xB8\xA5\xE0\xB9\x89\xE0\xB8\xA7");
            lv_obj_set_style_text_color(lbl_status, C_GREEN, 0);
        } else {
            lv_obj_set_style_bg_color(status_dot, C_RED, 0);
            // "เครื่องพิมพ์ไม่พร้อม"
            lv_label_set_text(lbl_status, "\xE0\xB9\x80\xE0\xB8\x84\xE0\xB8\xA3\xE0\xB8\xB7\xE0\xB9\x88\xE0\xB8\xAD\xE0\xB8\x87\xE0\xB8\x9E\xE0\xB8\xB4\xE0\xB8\xA1\xE0\xB8\x9E\xE0\xB9\x8C\xE0\xB9\x84\xE0\xB8\xA1\xE0\xB9\x88\xE0\xB8\x9E\xE0\xB8\xA3\xE0\xB9\x89\xE0\xB8\xAD\xE0\xB8\xA1");
            lv_obj_set_style_text_color(lbl_status, C_RED, 0);
        }
    }
    display_unlock();
}

void ui_update_id_number(const char *id)
{
    display_lock();
    if (lbl_id_number) {
        lv_label_set_text(lbl_id_number, id);
    }
    display_unlock();
}

void ui_update_name(const char *name)
{
    display_lock();
    if (lbl_name) {
        lv_label_set_text(lbl_name, name);
    }
    display_unlock();
}
