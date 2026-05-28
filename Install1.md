# Install1.md — วิธีการต่อเชื่อม Hardware

## โปรเจค ATD3.5-S3 Alcohol Breathalyzer Test
บอร์ด: ArtronShop ATD3.5-S3 (ESP32-S3 + LCD 3.5" ST7796S + Touch FT6336U)
วันที่: 28/05/2026

---

## ภาพรวมการต่อเชื่อม

```
ATD3.5-S3 (ESP32-S3)
├── LCD 3.5" ST7796S .............. SPI (ในตัว)
├── Touch FT6336U ................. I2C_NUM_0 (ในตัว)
├── DS1307 RTC .................... I2C_NUM_0 (ต่อเพิ่ม)
├── Magnetic Card Reader .......... UART2 (ต่อเพิ่ม)
└── USB Thermal Printer ........... USB OTG (ต่อเพิ่ม)
```

---

## 1. DS1307 Real-Time Clock (RTC)

โมดูล RTC มีแบตเตอรี่ CR2032 สำรองเวลาเมื่อดับไฟ
ใช้ I2C bus เดียวกับ Touch Controller (address ไม่ชนกัน)

### การต่อสาย

| DS1307 Pin | ATD3.5-S3 Pin | GPIO | หมายเหตุ |
|---|---|---|---|
| SDA | SDA | **GPIO 15** | I2C Data (ร่วมกับ Touch) |
| SCL | SCL | **GPIO 16** | I2C Clock (ร่วมกับ Touch) |
| VCC | 3.3V หรือ 5V | - | DS1307 รองรับทั้ง 3.3V/5V |
| GND | GND | - | - |

### I2C Address

| อุปกรณ์ | Address | หมายเหตุ |
|---|---|---|
| FT6336U (Touch) | `0x38` | ติดมากับบอร์ด |
| DS1307 (RTC) | `0x68` | ต่อเพิ่ม — ไม่ชนกัน ✅ |

### ข้อควรระวัง
- ตรวจสอบว่ามีแบตเตอรี่ CR2032 ใส่ในโมดูล DS1307
- I2C bus ใช้ internal pull-up ที่ ESP32 เปิดอยู่แล้ว
- ความเร็ว I2C: 400 kHz

---

## 2. Magnetic Card Reader (เครื่องอ่านใบขับขี่)

เครื่องอ่านบัตรแม่เหล็ก (Magnetic Stripe Reader) ส่งข้อมูลผ่าน UART

### การต่อสาย

| Card Reader Pin | ATD3.5-S3 Pin | GPIO | หมายเหตุ |
|---|---|---|---|
| TX (Data Out) | RXD | **GPIO 44** | UART2 RX — รับข้อมูลจากเครื่องอ่าน |
| VCC | 5V | - | ใช้ไฟ 5V |
| GND | GND | - | - |

### UART Configuration

| Parameter | Value |
|---|---|
| UART Port | `UART_NUM_2` |
| Baud Rate | `9600` |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

### ข้อมูลที่อ่านได้จากใบขับขี่
- **Track 1:** ชื่อ-นามสกุล (prefix, firstname, lastname)
- **Track 2:** เลขบัตรประชาชน 13 หลัก, วันหมดอายุ, วันเกิด

---

## 3. USB Thermal Printer (เครื่องพิมพ์ Slip)

เครื่องพิมพ์ใบเสร็จ/Slip ต่อผ่าน USB OTG ของ ESP32-S3

### การต่อสาย

| Printer | ATD3.5-S3 | หมายเหตุ |
|---|---|---|
| USB Type-A | USB OTG Port | ต่อตรง หรือผ่าน OTG adapter |

### ข้อควรระวัง
- ESP32-S3 ทำหน้าที่เป็น USB Host
- Printer ต้องรองรับ ESC/POS protocol
- Code Page: CP874 (Thai)
- ใช้ USB Bulk Transfer

---

## 4. LCD + Touch (ติดมากับบอร์ด)

### LCD — ST7796S (SPI)

| Function | GPIO |
|---|---|
| SCLK | GPIO 12 |
| MOSI | GPIO 11 |
| MISO | GPIO 13 |
| CS | GPIO 10 |
| DC | GPIO 21 |
| RST | GPIO 14 |
| Backlight | GPIO 3 |

### Touch — FT6336U (I2C)

| Function | GPIO |
|---|---|
| SDA | GPIO 15 |
| SCL | GPIO 16 |

### Display

| Parameter | Value |
|---|---|
| Resolution | 480 x 320 (Landscape) |
| Interface | SPI, 40 MHz |
| Color | 16-bit RGB565 |
| Orientation | Landscape (swap_xy = true) |

---

## สรุป GPIO ที่ใช้ทั้งหมด

| GPIO | Function | อุปกรณ์ |
|---|---|---|
| 3 | Backlight | LCD |
| 10 | SPI CS | LCD |
| 11 | SPI MOSI | LCD |
| 12 | SPI SCLK | LCD |
| 13 | SPI MISO | LCD |
| 14 | LCD RST | LCD |
| 15 | I2C SDA | Touch + DS1307 |
| 16 | I2C SCL | Touch + DS1307 |
| 21 | SPI DC | LCD |
| 44 | UART2 RX | Card Reader |
| USB | USB OTG | Printer |
