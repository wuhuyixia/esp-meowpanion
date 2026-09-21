#pragma once

#include <time.h>

// 动画状态枚举
typedef enum
{
    ANIM_STATE_IDLE,         // 空闲状态
    ANIM_STATE_FIRST,        // 第一个动画执行中
    ANIM_STATE_SECOND_THIRD, // 第二和第三个动画同时执行中
    ANIM_STATE_WAIT_SECOND,
    ANIM_STATE_WAIT_THIRD,
    ANIM_STATE_FOURTH,   // 第四个动画执行中
    ANIM_STATE_COMPLETED // 所有动画完成
} anim_state_t;

// 天气显示相关的全局变量
extern lv_obj_t *mainpage_weather;
extern lv_obj_t *mainpage_weather_desc_label;
extern lv_obj_t *mainpage_temperature_range_label;
extern lv_obj_t *mainpage_tomorrow_weather_label;
extern lv_obj_t *mainpage_tomorrow_temp_label;
extern lv_obj_t *mainpage_city_label;

void handle_animation_state(void);
void lv_main_page(void);

/**
 * @brief 更新主页面时间显示
 * @param timeinfo 时间信息结构体指针
 */
void update_mainpage_time_display(const struct tm *timeinfo);

/**
 * @brief 更新主页面天气显示
 * @param today_weather 今天的天气描述
 * @param today_temp 今天的温度范围
 * @param tomorrow_weather 明天的天气描述
 * @param tomorrow_temp 明天的温度范围
 * @param city 城市名称
 */
void update_mainpage_weather_display(const char *today_weather, const char *today_temp, 
                                    const char *tomorrow_weather, const char *tomorrow_temp, 
                                    const char *city);

                                    