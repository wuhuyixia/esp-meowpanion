#pragma once

#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/semphr.h"
#include "defines.h" // 包含统一的宏定义文件


// 天气字符最大为"雷阵雨伴有冰雹" 共计21, 某尾加\0 -> 22
typedef struct weather
{
    char text_day[FORECAST_DAY][22];           // 白天天气（文本）
    char text_night[FORECAST_DAY][22];         // 晚上天气（文本）
    uint8_t code_day[FORECAST_DAY];         // 白天天气（代码）
    uint8_t code_night[FORECAST_DAY];       // 晚上天气（代码）
    int8_t  degree_high[FORECAST_DAY];      // 最高气温
    int8_t  degree_low[FORECAST_DAY];       // 最低气温
    uint8_t humidity[FORECAST_DAY];         // 湿度 
    char city[22];
}weather_t;
extern weather_t weather;

void weather_task(void *param);
