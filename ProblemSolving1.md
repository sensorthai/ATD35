# ProblemSolving1.md — สรุปปัญหาและวิธีแก้ไข

## โปรเจค ATD3.5-S3 Alcohol Breathalyzer Test
วันที่: 28/05/2026

---

## 1. ปัญหา: Print Slip Format ไม่ตรงตามต้องการ

**อาการ:** Slip พิมพ์ออกมาเป็น format เก่า (หมายเลขบัตรประชาชน + keypad)

**สาเหตุ:** โค้ดเดิมออกแบบเป็นระบบ keypad พิมพ์เลข ไม่ใช่ระบบตรวจวัดแอลกอฮอล์

**วิธีแก้ไข:**
- เขียน `print_slip_task` ใหม่ใน `main.c` ให้มี 2 format:
  - **Format A** (ยินยอมเป่า) — รายงานผล + เลขบัตร + เวลา/วันที่
  - **Format B** (ไม่ยินยอม) — เพิ่มข้อความ `**** ไม่ยินยอมเป่า ****`
- ใช้ `ALIGN_LEFT` + `STYLE_NORMAL` ทั้งหมดตามที่ต้องการ

---

## 2. ปัญหา: Compiler Error — `format-truncation` (Werror)

**อาการ:**
```
error: '%04d' directive output may be truncated writing between 4 and 11 bytes
into a region of size between 0 and 10 [-Werror=format-truncation=]
```

**สาเหตุ:** Buffer `date_str[16]` เล็กเกินไปสำหรับ `snprintf("%02d/%02d/%04d")` — compiler คำนวณ worst-case ของ `tm_year` (int range) แล้วพบว่า buffer อาจไม่พอ

**วิธีแก้ไข:**
- เพิ่มขนาด buffer จาก `char date_str[16]` เป็น `char date_str[40]`
- เพิ่มขนาด clock display buffer จาก `char buf[32]` เป็น `char buf[80]`
- ต้องแก้ทุกที่ที่มี `snprintf` กับ `%04d` (มี 3 จุด: format_a, format_b, clock_update)

---

## 3. ปัญหา: Compiler Error — `unused variable 'TAG'`

**อาการ:**
```
error: 'TAG' defined but not used [-Wunused-variable]
```

**สาเหตุ:** ลบโค้ด `ESP_LOGI(TAG, ...)` ออกจาก `ui_screens.c` แต่ยังมี `static const char *TAG` ค้างอยู่

**วิธีแก้ไข:**
- ลบบรรทัด `static const char *TAG = "ui_screens";` ออก

---

## 4. ปัญหา: Compiler Error — `unknown type name 'bool'`

**อาการ:**
```
error: unknown type name 'bool'
ds1307.h:34:1: error: unknown type name 'bool'
```

**สาเหตุ:** ไฟล์ `ds1307.h` ใช้ type `bool` แต่ไม่ได้ include `<stdbool.h>`

**วิธีแก้ไข:**
- เพิ่ม `#include <stdbool.h>` ใน `ds1307.h`

---

## 5. ปัญหา: เวลาหายเมื่อปิดเครื่อง

**อาการ:** ทุกครั้งที่เปิดเครื่องต้องตั้งเวลาใหม่ เพราะ ESP32-S3 internal RTC ไม่มีแบตเตอรี่สำรอง

**สาเหตุ:** ESP32-S3 internal RTC จะรีเซ็ตเป็น epoch 0 เมื่อดับไฟ

**วิธีแก้ไข:**
- เพิ่มโมดูล **DS1307 RTC** (มีแบตเตอรี่ CR2032 สำรอง)
- สร้าง driver `ds1307.c` / `ds1307.h`
- ตอน boot: อ่านเวลาจาก DS1307 → sync เข้า ESP32 system clock
- ตอน confirm เวลา: บันทึกกลับลง DS1307
- ใช้ I2C bus เดียวกับ Touch (GPIO 15/16) address ไม่ชนกัน

---

## 6. ปัญหา: เลขบัตรประชาชน Hardcode

**อาการ:** เลขบัตรแสดงค่าคงที่ `1521253654587` ไม่สามารถเปลี่ยนได้

**สาเหตุ:** ยังไม่มี card reader

**วิธีแก้ไข:**
- เพิ่ม Magnetic Card Reader ผ่าน UART2 (GPIO 44)
- สร้าง driver `card_reader.c` / `card_reader.h`
- เมื่อรูดใบขับขี่ → parse Track2 → ดึงเลขบัตรประชาชน 13 หลัก
- อัพเดทจอแสดงผล + ใช้ในการพิมพ์ Slip อัตโนมัติ
