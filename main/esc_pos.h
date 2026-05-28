#ifndef ESC_POS_H
#define ESC_POS_H

#include <stdint.h>
#include <stddef.h>

// Alignment options
typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2
} esc_pos_align_t;

// Text styles (bitmask for ESC !)
#define STYLE_NORMAL      0x00
#define STYLE_BOLD        0x08
#define STYLE_DBL_HEIGHT  0x10
#define STYLE_DBL_WIDTH   0x20
#define STYLE_DOUBLE_HW   0x30
#define STYLE_UNDERLINE   0x80

// ESC/POS functions
void esc_pos_init(uint8_t *buf, size_t *len);
void esc_pos_align(uint8_t *buf, size_t *len, esc_pos_align_t align);
void esc_pos_style(uint8_t *buf, size_t *len, uint8_t style);
void esc_pos_text(uint8_t *buf, size_t *len, const char *text);
void esc_pos_feed(uint8_t *buf, size_t *len, uint8_t lines);
void esc_pos_cut(uint8_t *buf, size_t *len);
void esc_pos_barcode(uint8_t *buf, size_t *len, const char *data);
void esc_pos_select_code_page(uint8_t *buf, size_t *len, uint8_t page);
void esc_pos_thai_text(uint8_t *buf, size_t *len, const char *utf8_text);
void esc_pos_qrcode(uint8_t *buf, size_t *len, const char *data);
void esc_pos_print_bitmap(uint8_t *buf, size_t *len, uint16_t width, uint16_t height, const uint8_t *bitmap_data);

#endif // ESC_POS_H
