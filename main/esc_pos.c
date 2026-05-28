#include <string.h>
#include "esc_pos.h"

void esc_pos_init(uint8_t *buf, size_t *len)
{
    buf[*len] = 0x1B;
    buf[*len + 1] = 0x40; // ESC @
    *len += 2;
}

void esc_pos_align(uint8_t *buf, size_t *len, esc_pos_align_t align)
{
    buf[*len] = 0x1B;
    buf[*len + 1] = 0x61; // ESC a
    buf[*len + 2] = (uint8_t)align;
    *len += 3;
}

void esc_pos_style(uint8_t *buf, size_t *len, uint8_t style)
{
    buf[*len] = 0x1B;
    buf[*len + 1] = 0x21; // ESC !
    buf[*len + 2] = style;
    *len += 3;
}

void esc_pos_text(uint8_t *buf, size_t *len, const char *text)
{
    size_t slen = strlen(text);
    memcpy(&buf[*len], text, slen);
    *len += slen;
}

void esc_pos_feed(uint8_t *buf, size_t *len, uint8_t lines)
{
    for (uint8_t i = 0; i < lines; i++) {
        buf[*len] = 0x0A; // LF
        (*len)++;
    }
}

void esc_pos_cut(uint8_t *buf, size_t *len)
{
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x56; // GS V
    buf[*len + 2] = 0x42; // m = 66 (feed paper and cut)
    buf[*len + 3] = 0x00; // n = 0 dots
    *len += 4;
}

void esc_pos_barcode(uint8_t *buf, size_t *len, const char *data)
{
    // Set barcode height (GS h n)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x68;
    buf[*len + 2] = 80;   // 80 dots high
    *len += 3;

    // Set barcode width (GS w n)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x77;
    buf[*len + 2] = 3;    // Width level 3
    *len += 3;

    // Set HRI characters position (GS H n)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x48;
    buf[*len + 2] = 2;    // Print below the barcode
    *len += 3;

    // Print barcode (GS k m d1...dk NUL)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x6B;
    buf[*len + 2] = 4;    // m = 4: CODE39
    *len += 3;

    size_t dlen = strlen(data);
    memcpy(&buf[*len], data, dlen);
    *len += dlen;

    buf[*len] = 0x00;     // NUL terminator
    (*len)++;
}

void esc_pos_select_code_page(uint8_t *buf, size_t *len, uint8_t page)
{
    buf[*len] = 0x1B;
    buf[*len + 1] = 0x74; // ESC t
    buf[*len + 2] = page;
    *len += 3;
}

void esc_pos_thai_text(uint8_t *buf, size_t *len, const char *utf8_text)
{
    size_t i = 0;
    while (utf8_text[i] != '\0') {
        uint8_t b1 = (uint8_t)utf8_text[i];
        if (b1 < 0x80) {
            buf[*len] = b1;
            (*len)++;
            i++;
        } else if (b1 == 0xE0 && (uint8_t)utf8_text[i+1] == 0xB8) {
            uint8_t b3 = (uint8_t)utf8_text[i+2];
            buf[*len] = b3 + 0x20;
            (*len)++;
            i += 3;
        } else if (b1 == 0xE0 && (uint8_t)utf8_text[i+1] == 0xB9) {
            uint8_t b3 = (uint8_t)utf8_text[i+2];
            buf[*len] = b3 + 0x60;
            (*len)++;
            i += 3;
        } else {
            i++; // Skip unsupported multibyte sequences
        }
    }
}

void esc_pos_qrcode(uint8_t *buf, size_t *len, const char *data)
{
    size_t dlen = strlen(data);
    
    // 1. Set QR Code Cell Size (GS ( k 3 0 49 67 size)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x28;
    buf[*len + 2] = 0x6B;
    buf[*len + 3] = 0x03;
    buf[*len + 4] = 0x00;
    buf[*len + 5] = 49;  // cn = 49 (QR Code)
    buf[*len + 6] = 67;  // fn = 67 (cell size)
    buf[*len + 7] = 6;   // size = 6 dots (range 1-16)
    *len += 8;

    // 2. Set Error Correction Level (GS ( k 3 0 49 69 level)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x28;
    buf[*len + 2] = 0x6B;
    buf[*len + 3] = 0x03;
    buf[*len + 4] = 0x00;
    buf[*len + 5] = 49;  // cn = 49
    buf[*len + 6] = 69;  // fn = 69 (error correction)
    buf[*len + 7] = 48;  // level = 48 (level L)
    *len += 8;

    // 3. Store QR Code Data (GS ( k pL pH 49 80 48 d1...dk)
    uint16_t param_len = dlen + 3;
    uint8_t pL = param_len & 0xFF;
    uint8_t pH = (param_len >> 8) & 0xFF;

    buf[*len] = 0x1D;
    buf[*len + 1] = 0x28;
    buf[*len + 2] = 0x6B;
    buf[*len + 3] = pL;
    buf[*len + 4] = pH;
    buf[*len + 5] = 49;  // cn = 49
    buf[*len + 6] = 80;  // fn = 80 (store data)
    buf[*len + 7] = 48;  // m = 48
    *len += 8;

    memcpy(&buf[*len], data, dlen);
    *len += dlen;

    // 4. Print QR Code Symbol (GS ( k 3 0 49 81 48)
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x28;
    buf[*len + 2] = 0x6B;
    buf[*len + 3] = 0x03;
    buf[*len + 4] = 0x00;
    buf[*len + 5] = 49;  // cn = 49
    buf[*len + 6] = 81;  // fn = 81 (print symbol)
    buf[*len + 7] = 48;  // m = 48
    *len += 8;
}

void esc_pos_print_bitmap(uint8_t *buf, size_t *len, uint16_t width, uint16_t height, const uint8_t *bitmap_data)
{
    uint16_t width_bytes = (width + 7) / 8;
    
    // Command: GS v 0 m xL xH yL yH
    buf[*len] = 0x1D;
    buf[*len + 1] = 0x76; // v
    buf[*len + 2] = 0x30; // 0
    buf[*len + 3] = 0x00; // m = 0 (Normal size)
    
    buf[*len + 4] = width_bytes & 0xFF; // xL
    buf[*len + 5] = (width_bytes >> 8) & 0xFF; // xH
    
    buf[*len + 6] = height & 0xFF; // yL
    buf[*len + 7] = (height >> 8) & 0xFF; // yH
    
    *len += 8;
    
    size_t data_len = width_bytes * height;
    memcpy(&buf[*len], bitmap_data, data_len);
    *len += data_len;
}
