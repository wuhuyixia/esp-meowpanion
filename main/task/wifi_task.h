#pragma once

#include "esp_wifi.h"
#include <time.h> // 为了struct tm类型
#include "defines.h" // 包含统一的宏定义文件



// 全局时间变量，用于在其他地方访问当前时间
extern struct tm global_timeinfo;
extern bool global_time_valid; // 标记时间是否有效

// 函数声明
esp_err_t wifi_sta_init(void);
uint8_t get_wifi_status(void);
void sntp_task(void *pvParameters);
void wifi_monitor_task(void *pvParameters);
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data);
