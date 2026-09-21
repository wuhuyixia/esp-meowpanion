/**
 * @file headfile.h
 * @brief 项目统一头文件管理
 * @author ESP32-S3 Project
 * @date 2023
 * @note 本文件集中管理所有头文件包含，按功能模块分类，便于维护
 */

#ifndef HEADFILE_H
#define HEADFILE_H

/*============================================================================
 * 标准库头文件
 *============================================================================*/
#include <stdio.h>  // 标准输入输出函数
#include <stdint.h> // 标准整数类型定义
#include <string.h> // 字符串操作函数
#include <math.h>   // 数学计算函数
#include <stdlib.h> // 标准库通用工具函数

/*============================================================================
 * ESP-IDF 系统头文件
 *============================================================================*/
// 系统基础
#include "esp_err.h"    // ESP32错误类型定义
#include "esp_log.h"    // 日志系统
#include "esp_check.h"  // 错误检查宏
#include "esp_system.h" // 系统功能接口
#include "esp_vfs.h"    // 虚拟文件系统
#include "nvs_flash.h"  // 非易失性存储
#include "esp_event.h"  // 事件系统
#include "esp_wifi.h"   // WiFi功能

// 文件系统
#include "esp_spiffs.h"        // SPIFFS文件系统
#include "esp_vfs_fat.h"       // FAT文件系统
#include "sdmmc_cmd.h"         // SD/MMC命令接口
#include "driver/sdmmc_host.h" // SD/MMC主机驱动

// 驱动相关
#include "driver/i2c_master.h" // 新版 I2C 主机驱动
#include "driver/spi_master.h" // SPI主机驱动
#include "driver/ledc.h"       // LED PWM控制器驱动
#include "driver/i2s_std.h"    // I2S标准驱动

// 实时操作系统
#include "freertos/FreeRTOS.h"     // FreeRTOS核心
#include "freertos/task.h"         // FreeRTOS任务管理
#include "freertos/event_groups.h" // FreeRTOS事件组

// LCD显示相关
#include "esp_lcd_types.h"        // LCD类型定义
#include "esp_lcd_panel_io.h"     // LCD面板IO接口
#include "esp_lcd_panel_vendor.h" // LCD面板供应商驱动
#include "esp_lcd_panel_ops.h"    // LCD面板操作接口
#include "esp_lcd_touch_ft5x06.h" // FT5x06触摸屏驱动

// 音频编解码器接口
#include "esp_codec_dev.h"          // 音频编解码器设备接口
#include "esp_codec_dev_defaults.h" // 音频编解码器默认配置
#include "esp_lvgl_port.h"          // ESP32 LVGL端口
#include "audio_player.h"           // 音频播放器
#include "file_iterator.h"          // 文件迭代器

/*============================================================================
 * LVGL 图形库头文件
 *============================================================================*/
#include "lvgl.h" // LVGL图形库核心

/*============================================================================
 * 项目自定义头文件
 *============================================================================*/
// 系统配置
#include "defines.h"      // 项目定义和常量 (已包含，确保整个项目能正确访问宏定义)
#include "esp32_s3_szp.h" // ESP32-S3板级支持包

// 板级支持包(BSP)
#include "bsp/aspiffs.h" // SPIFFS文件系统BSP
#include "bsp/key.h"     // 按键BSP
#include "bsp/codec.h"   // 音频编解码器BSP

// 用户界面
#include "ui/load_anim.h"      // 加载动画
#include "ui/mainmenu.h"       // 主界面头文件
#include "ui/link_list_menu.h" // 链接列表菜单

// 任务
#include "task/power_music.h"    // 开机音乐任务
#include "task/main_page.h"      // 主界面任务
#include "task/camera_task.h"    // 摄像头任务
#include "task/wifi_task.h"      // WiFi任务
#include "task/setting_action.h" // 设置操作任务
#include "task/ball_task.h"      // 球体任务
#include "task/music_task.h"     // 音乐任务
#include "task/weather_task.h"   // 天气任务
#include "task/sdcard_task.h"         // SD卡任务
#endif /* HEADFILE_H */
