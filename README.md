# 喵伴

ESP32-S3 LVGL Project with Kawaii Face Component.

## Features

- ESP-IDF based project
- LVGL graphics library
- LCD display support
- Touch screen support
- Kawaii Face component (0015/lvgl_kawaii_face)

## Dependencies

- lvgl/lvgl: ~8.3.0
- espressif/esp_lvgl_port: ~1.4.0
- espressif/esp_lcd_touch_ft5x06: ~1.0.6
- 0015/lvgl_kawaii_face: ^1.0.0

## Build

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash
```
