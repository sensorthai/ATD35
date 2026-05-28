# 📋 ATD3.5-S3 Touchscreen ID Card Printer — Product Features

## ภาพรวมโปรเจกต์

ระบบรับหมายเลขบัตรประชาชน 13 หลัก ผ่านจอสัมผัสขนาด 3.5 นิ้ว
และสั่งพิมพ์ใบเสร็จออกเครื่องพิมพ์ USB ผ่านพอร์ต OTG ของ ESP32-S3

---

## 🖥️ Hardware Platform

| รายการ | รายละเอียด |
|---|---|
| **Board** | ArtronShop ATD3.5-S3 (ESP32-S3 QFN56, rev v0.2) |
| **LCD** | ST7796S 3.5" TFT 480×320 SPI |
| **Touch** | FT6336U Capacitive I2C |
| **RAM** | Embedded PSRAM 8MB |
| **Flash** | 4MB SPI Flash |
| **USB** | Native USB OTG (Host mode) |
| **Framework** | ESP-IDF v5.2 |
| **GUI** | LVGL 8.3 via esp_lvgl_port |

---

## ✨ Features

### 1. จอสัมผัส Touchscreen Keypad (480×320 Landscape)

- **Glassmorphic Dark Theme** — ดีไซน์โทนสีเข้ม (Space Blue) พร้อม card panel แบบ premium
- **Amber Accent** — เส้นขอบซ้ายสีส้มเน้นความสวยงาม
- **Keypad 0-9** พร้อมปุ่ม **CLR** (ลบ) และ **ENTER** (ยืนยัน/สั่งพิมพ์)
- **Gradient Buttons** — ปุ่มไล่สี vertical gradient พร้อม pressed effect
- **แสดงผลตัวเลขแบบ Real-time** — กดเลขแล้วแสดงทันทีบน display card
- **รองรับ 13 หลัก** — เต็มรูปแบบหมายเลขบัตรประชาชนไทย
- **ข้อความเตือน** — แสดง temporary message พร้อม auto-dismiss timer

### 2. ภาษาไทยบนจอ LVGL 🇹🇭

- **Custom Thai Font** — แปลงจาก Google Fonts "Sarabun Bold" ด้วย `lv_font_conv`
- **2 ขนาด:**
  - `font_thai_24` (24px) — หัวข้อ "**บัตรประชาชน**" สีขาวหนา
  - `font_thai_16` (16px) — สถานะเครื่องพิมพ์
- **ครอบคลุม Unicode Thai Block** — U+0E00–U+0E7F + ASCII U+0020–U+007F
- **Rendering 4bpp** — Anti-aliasing คมชัดบนจอ TFT

### 3. สถานะเครื่องพิมพ์แบบ Real-time

- **สีเขียว + "เครื่องพิมพ์พร้อมแล้ว"** — เมื่อเครื่องพิมพ์ USB เชื่อมต่อ
- **สีแดง + "เครื่องพิมพ์ไม่พร้อม"** — เมื่อเครื่องพิมพ์ถูกถอดออก
- **Status Dot Indicator** — จุดสีกลมแสดงสถานะแบบ visual
- **Dual Monitoring:**
  - USB Hot-plug Event Callback (ตอบสนองทันทีเมื่อเสียบ/ถอด)
  - Periodic Polling ทุก 2 วินาที (sync UI กับ hardware state)

### 4. USB OTG Host Printer

- **Native USB Host** — ใช้ ESP32-S3 USB OTG เชื่อมต่อเครื่องพิมพ์โดยตรง
- **USB Bulk Transfer** — ส่งข้อมูลผ่าน bulk endpoints
- **Hot-plug Support** — เสียบ/ถอดเครื่องพิมพ์ได้ตลอดเวลา
- **Thread-safe** — ใช้ Mutex ป้องกัน concurrent access

### 5. ESC/POS Receipt Printing

- **ภาษาไทยบนใบเสร็จ** — ใช้ Code Page CP874 (Thai)
- **รูปแบบใบเสร็จ:**
  ```
  ================================
     หมายเลขบัตรประชาชน
  ================================
  --------------------------------
        1234567890123
  --------------------------------
  ระบบพิมพ์ผ่านจอสัมผัส ATD3.5-S3
         ยินดีต้อนรับค่ะ
  ================================
  ```
- **Double Height + Bold** — หัวข้อและตัวเลขขนาดใหญ่ชัดเจน
- **Auto Cut** — ตัดกระดาษอัตโนมัติหลังพิมพ์
- **Background Print Task** — พิมพ์ใน FreeRTOS task แยก ไม่ block UI

### 6. Display & Touch Calibration

- **Landscape Rotation** — จอหมุน 90° จาก portrait เป็น landscape (480×320)
- **Touch Coordinate Mapping** — สอดคล้องกับ ArtronShop reference library
- **Transformation Logic:**
  - `swap_xy = true` (สลับแกนสำหรับ landscape)
  - `mirror_x = true` (กลับแกน Y หลัง swap)
  - `mirror_y = false`

---

## 📁 โครงสร้างไฟล์

```
ATD35/
├── main/
│   ├── main.c              — Entry point, event loop, print job
│   ├── display.c           — LCD + Touch + LVGL initialization
│   ├── display.h           — Pin definitions and display API
│   ├── ui_screens.c        — Keypad UI, Thai labels, status display
│   ├── ui_screens.h        — UI API declarations
│   ├── usb_printer.c       — USB Host printer driver
│   ├── usb_printer.h       — Printer types and API
│   ├── esc_pos.c           — ESC/POS command builder
│   ├── esc_pos.h           — ESC/POS API
│   ├── font_thai_16.c      — Thai font 16px (Sarabun Bold)
│   ├── font_thai_24.c      — Thai font 24px (Sarabun Bold)
│   ├── Sarabun-Bold.ttf    — Source TTF font file
│   └── CMakeLists.txt      — Build configuration
├── CMakeLists.txt           — Project-level CMake
├── sdkconfig                — ESP-IDF configuration
└── managed_components/      — ESP Component Manager dependencies
    ├── espressif__esp_lcd_st7796/
    ├── espressif__esp_lcd_touch_ft5x06/
    ├── espressif__esp_lvgl_port/
    └── lvgl__lvgl/
```

---

## 🔧 Build & Flash

```powershell
# Setup ESP-IDF environment
& C:\Antigravity\ESP32\esp-idf\export.ps1

# Build
idf.py build

# Flash to board
idf.py -p COM6 flash

# Monitor serial output
idf.py -p COM6 monitor
```

---

## 📊 Binary Size

| Component | Size |
|---|---|
| **Bootloader** | 21 KB |
| **Application** | ~644 KB (80% partition free) |
| **Partition** | 3 KB |
| **Total Flash Used** | ~668 KB / 3 MB |
