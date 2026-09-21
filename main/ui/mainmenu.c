#include "headfile.h"
#include "time.h" // 为了struct tm类型
#include "task/weather_task.h" // 为了天气数据结构和函数
static anim_state_t current_anim_state = ANIM_STATE_IDLE;
 

lv_obj_t *mainpage_weather_desc_label = NULL;
lv_obj_t *mainpage_temperature_range_label = NULL;
lv_obj_t *mainpage_tomorrow_weather_label = NULL;
lv_obj_t *mainpage_tomorrow_temp_label = NULL;
lv_obj_t *mainpage_city_label = NULL;
lv_obj_t *mainpage_tomorrow_label = NULL;
lv_obj_t *mainpage_today_label = NULL;
LV_FONT_DECLARE(tiktok24);

LV_FONT_DECLARE(tiktok32);
LV_FONT_DECLARE(tiktok48);
LV_FONT_DECLARE(tiktok63);
/* Global UI objects */

lv_obj_t *mainpage_weather = NULL;
lv_obj_t *mainpage_temperature_label = NULL;
lv_obj_t *mainpage_year = NULL;
lv_obj_t *mainpage_year_label = NULL;
lv_obj_t *mainpage_weekday_label = NULL;
lv_obj_t *mainpage_year_text_label = NULL;
lv_obj_t *mainpage_day_label = NULL;
lv_obj_t *mainpage_day_text_label = NULL;
lv_obj_t *mainpage_month_text_label = NULL;
lv_obj_t *mainpage_month_label = NULL;
lv_obj_t *mainpage_point = NULL;
lv_obj_t *mainpage_time = NULL;
lv_obj_t *mainpage_hour_label = NULL;
lv_obj_t *mainpage_minute_label = NULL;
lv_obj_t *mainpage_colon1_label = NULL;
lv_obj_t *mainpage_second_label = NULL;
lv_obj_t *mainpage_colon2_label = NULL;
// 通用平移动画函数
// 参数说明：
// obj - 要应用动画的对象
// start_x - 起始X坐标
// end_x - 结束X坐标
// duration - 动画持续时间(毫秒)
// 带回调的平移动画函数
static void create_slide_animation_with_cb(lv_obj_t *obj, int32_t start_x, int32_t end_x, uint32_t duration, void (*cb)(void))
{
    if (lvgl_port_lock(100))
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, obj);
        lv_anim_set_values(&a, start_x, end_x);
        lv_anim_set_time(&a, duration);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);

        // 设置完成回调
        if (cb)
        {
            lv_anim_set_ready_cb(&a, (lv_anim_ready_cb_t)cb);
        }

        lv_anim_start(&a);
        lvgl_port_unlock();
    }
}
static void first_animation_complete_cb(void)
{
    // 第一个动画完成，设置状态为第二和第三个动画执行中
    current_anim_state = ANIM_STATE_SECOND_THIRD;
    // 处理状态转换
    handle_animation_state();
}

static void second_animation_complete_cb(void)
{
    // 第二个动画完成，不立即改变状态，等待第三个动画也完成
    if (current_anim_state == ANIM_STATE_WAIT_THIRD)
    {
        // 第三个动画已经完成，可以执行第四个动画
        current_anim_state = ANIM_STATE_FOURTH;
        handle_animation_state();
    }
    else
    {
        // 标记为等待第三个动画完成
        current_anim_state = ANIM_STATE_WAIT_SECOND;
    }
}

static void third_animation_complete_cb(void)
{
    // 第三个动画完成，不立即改变状态，等待第二个动画也完成
    if (current_anim_state == ANIM_STATE_WAIT_SECOND)
    {
        // 第二个动画已经完成，可以执行第四个动画
        current_anim_state = ANIM_STATE_FOURTH;
        handle_animation_state();
    }
    else
    {
        // 标记为等待第二个动画完成
        current_anim_state = ANIM_STATE_WAIT_THIRD;
    }
}

static void fourth_animation_complete_cb(void)
{
    // 第四个动画完成，设置状态为完成
    current_anim_state = ANIM_STATE_COMPLETED;
    handle_animation_state();
}
// 处理动画状态的函数
void handle_animation_state(void)
{
    if (lvgl_port_lock(100))
    {
        switch (current_anim_state)
        {
        case ANIM_STATE_IDLE:
            // 启动第一个动画
            create_slide_animation_with_cb(mainpage_time, -320, -78, 500, first_animation_complete_cb);
            current_anim_state = ANIM_STATE_FIRST;
            break;

        case ANIM_STATE_FIRST:
            // 第一个动画执行中，等待回调
            break;

        case ANIM_STATE_SECOND_THIRD:
            // 同时启动第二和第三个动画
            create_slide_animation_with_cb(mainpage_year, -320, -50, 500, second_animation_complete_cb);
            create_slide_animation_with_cb(mainpage_point, 350, 255, 500, third_animation_complete_cb);
            break;

        case ANIM_STATE_WAIT_SECOND:
            // 等待第二个动画完成
            break;

        case ANIM_STATE_WAIT_THIRD:
            // 等待第三个动画完成
            break;

        case ANIM_STATE_FOURTH:
            // 启动第四个动画
            create_slide_animation_with_cb(mainpage_weather, 500, 0, 500, fourth_animation_complete_cb);
            break;

        case ANIM_STATE_COMPLETED:
            // 所有动画完成，可以在这里执行完成后的操作
            break;
        }
        lvgl_port_unlock();
    }
}
// 启动动画状态机
void start_animation_state_machine(void)
{
    if (lvgl_port_lock(100))
    {
        // 重置状态
        current_anim_state = ANIM_STATE_IDLE;
        // 处理初始状态
        handle_animation_state();
        lvgl_port_unlock();
    }
}
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
                                    const char *city)
{
    if (lvgl_port_lock(100))
    {
        // 更新今天的天气描述
        if (mainpage_weather_desc_label != NULL && today_weather != NULL)
        {
            lv_label_set_text(mainpage_weather_desc_label, today_weather);
        }
        
        // 更新今天的温度范围
        if (mainpage_temperature_range_label != NULL && today_temp != NULL)
        {
            lv_label_set_text(mainpage_temperature_range_label, today_temp);
        }
        
        // 更新明天的天气描述
        if (mainpage_tomorrow_weather_label != NULL && tomorrow_weather != NULL)
        {
            lv_label_set_text(mainpage_tomorrow_weather_label, tomorrow_weather);
        }
        
        // 更新明天的温度范围
        if (mainpage_tomorrow_temp_label != NULL && tomorrow_temp != NULL)
        {
            lv_label_set_text(mainpage_tomorrow_temp_label, tomorrow_temp);
        }
        
        // 更新城市名称
        if (mainpage_city_label != NULL && city != NULL)
        {
            lv_label_set_text(mainpage_city_label, city);
        }
        
        lvgl_port_unlock();
    }
}

/**
 * @brief 更新主页面时间显示
 * @param timeinfo 时间信息结构体指针
 */
void update_mainpage_time_display(const struct tm *timeinfo)
{
    if (timeinfo == NULL)
        return;

    char time_buf[16]; // 增大缓冲区大小以避免格式截断警告

    if (lvgl_port_lock(100)) // 获取LVGL锁
    {
        // 更新小时显示
        snprintf(time_buf, sizeof(time_buf), "%02d\n", timeinfo->tm_hour);
        if (mainpage_hour_label)
        {
            lv_label_set_text(mainpage_hour_label, time_buf);
        }

        // 更新分钟显示
        snprintf(time_buf, sizeof(time_buf), "%02d", timeinfo->tm_min);
        if (mainpage_minute_label)
        {
            lv_label_set_text(mainpage_minute_label, time_buf);
        }

        // 更新秒显示
        snprintf(time_buf, sizeof(time_buf), "%02d", timeinfo->tm_sec);
        if (mainpage_second_label)
        {
            lv_label_set_text(mainpage_second_label, time_buf);
        }

        // 更新年份显示
        snprintf(time_buf, sizeof(time_buf), "%04d\n", timeinfo->tm_year + 1900);
        if (mainpage_year_label)
        {
            lv_label_set_text(mainpage_year_label, time_buf);
        }

        // 更新月份显示
        snprintf(time_buf, sizeof(time_buf), "%02d\n", timeinfo->tm_mon + 1);
        if (mainpage_month_label)
        {
            lv_label_set_text(mainpage_month_label, time_buf);
        }

        // 更新日期显示
        snprintf(time_buf, sizeof(time_buf), "%d\n\n", timeinfo->tm_mday);
        if (mainpage_day_label)
        {
            lv_label_set_text(mainpage_day_label, time_buf);
        }

        // 更新星期显示
        const char *weekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
        if (mainpage_weekday_label && timeinfo->tm_wday >= 0 && timeinfo->tm_wday < 7)
        {
            lv_label_set_text(mainpage_weekday_label, weekdays[timeinfo->tm_wday]);
        }

        lvgl_port_unlock(); // 释放LVGL锁
    }
}

void lv_main_page(void)
{
    if (lvgl_port_lock(100))
    {
        // Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_bg_opa(mainpage_screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mainpage_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(mainpage_screen, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(mainpage_screen, LV_OBJ_FLAG_SCROLLABLE);

        // Write codes screen_time - 时间容器，移到前面创建
        mainpage_time = lv_obj_create(mainpage_screen);
        lv_obj_set_pos(mainpage_time, -400, 0); //-400是为了初始化 后续动画会移动到正确位置
        lv_obj_set_size(mainpage_time, 396, 79);
        lv_obj_set_scrollbar_mode(mainpage_time, LV_SCROLLBAR_MODE_OFF);

        // Write style for screen_time, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_time, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(mainpage_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(mainpage_time, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(mainpage_time, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_time, 19, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mainpage_time, lv_color_hex(0xB194D1), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_hour_label - 小时标签
        mainpage_hour_label = lv_label_create(mainpage_time);
        lv_label_set_text(mainpage_hour_label, "20");
        lv_label_set_long_mode(mainpage_hour_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_hour_label, 85, 10);
        lv_obj_set_size(mainpage_hour_label, 112, 35);

        // Write style for screen_hour_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_hour_label, lv_color_hex(0xD8AB41), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_hour_label, &tiktok63, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_hour_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_hour_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Set flex layout for vertical alignment
        lv_obj_set_flex_align(mainpage_hour_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_hour_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_minute_label - 分钟标签
        mainpage_minute_label = lv_label_create(mainpage_time);
        lv_label_set_text(mainpage_minute_label, "42");
        lv_label_set_long_mode(mainpage_minute_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_minute_label, 195 ,10);
        lv_obj_set_size(mainpage_minute_label, 100, 35);

        // Write style for screen_minute_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_minute_label, lv_color_hex(0x1868DC), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_minute_label, &tiktok63, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_minute_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_minute_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Set flex layout for vertical alignment
        lv_obj_set_flex_align(mainpage_minute_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_minute_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_colon1_label - 第一个冒号
        mainpage_colon1_label = lv_label_create(mainpage_time);
        lv_label_set_text(mainpage_colon1_label, ":");
        lv_label_set_long_mode(mainpage_colon1_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_colon1_label, 170, 10);
        lv_obj_set_size(mainpage_colon1_label, 40, 35);

        // Write style for screen_colon1_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_colon1_label, lv_color_hex(0x07C3E1), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_colon1_label, &tiktok63, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_colon1_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_colon1_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Set flex layout for vertical alignment
        lv_obj_set_flex_align(mainpage_colon1_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_colon1_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_second_label - 秒标签
        mainpage_second_label = lv_label_create(mainpage_time);
        lv_label_set_text(mainpage_second_label, "47");
        lv_label_set_long_mode(mainpage_second_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_second_label, 315, 10);
        lv_obj_set_size(mainpage_second_label, 70, 35);

        // Write style for screen_second_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_second_label, lv_color_hex(0x3288DC), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_second_label, &tiktok48, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_second_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_second_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Set flex layout for vertical alignment
        lv_obj_set_flex_align(mainpage_second_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_second_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_colon2_label - 第二个冒号
        mainpage_colon2_label = lv_label_create(mainpage_time);
        lv_label_set_text(mainpage_colon2_label, ":");
        lv_label_set_long_mode(mainpage_colon2_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_colon2_label, 280, 10);
        lv_obj_set_size(mainpage_colon2_label, 35, 35);

        // Write style for screen_colon2_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_colon2_label, lv_color_hex(0x07C3E1), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_colon2_label, &tiktok63, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_colon2_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_colon2_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Set flex layout for vertical alignment
        lv_obj_set_flex_align(mainpage_colon2_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_colon2_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_weather
        mainpage_weather = lv_obj_create(mainpage_screen);
        lv_obj_set_pos(mainpage_weather, 400, 164); // 400是 为了 初始设置之后看不到 不影响后续动画
        lv_obj_set_size(mainpage_weather, 451, 77);
        lv_obj_set_scrollbar_mode(mainpage_weather, LV_SCROLLBAR_MODE_OFF);

        // Write style for screen_weather, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_weather, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(mainpage_weather, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(mainpage_weather, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(mainpage_weather, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_weather, 19, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_weather, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mainpage_weather, lv_color_hex(0x0DECEF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_weather, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_weather, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_weather, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_weather, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_weather, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 添加竖向显示的"明天"标签（左侧）
        mainpage_tomorrow_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_tomorrow_label, "明天"); // 使用换行符实现竖向显示
        lv_obj_set_pos(mainpage_tomorrow_label, 180, 8);
        lv_obj_set_size(mainpage_tomorrow_label, 40, 65);
        lv_obj_set_style_text_color(mainpage_tomorrow_label, lv_color_hex(0xFF0080), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_tomorrow_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_tomorrow_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_tomorrow_label, 5, LV_PART_MAIN | LV_STATE_DEFAULT); // 设置行间距

        // 添加"多云"标签（明天右侧）
        mainpage_tomorrow_weather_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_tomorrow_weather_label, "多云");
        lv_obj_set_pos(mainpage_tomorrow_weather_label, 190, 8);
        lv_obj_set_size(mainpage_tomorrow_weather_label, 120, 35);
        lv_obj_set_style_text_color(mainpage_tomorrow_weather_label, lv_color_hex(0x2169EB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_tomorrow_weather_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_tomorrow_weather_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 添加"30°/12°"温度标签（多云下方）
        mainpage_tomorrow_temp_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_tomorrow_temp_label, "30°/12°");
        lv_obj_set_pos(mainpage_tomorrow_temp_label, 210, 40);
        lv_obj_set_size(mainpage_tomorrow_temp_label, 100, 30);
        lv_obj_set_style_text_color(mainpage_tomorrow_temp_label, lv_color_hex(0x2169EB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_tomorrow_temp_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_tomorrow_temp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 添加竖向显示的"今天"标签
        mainpage_today_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_today_label, "今天"); // 使用换行符实现竖向显示
        lv_obj_set_pos(mainpage_today_label, 15, 8);
        lv_obj_set_size(mainpage_today_label, 40, 65);
        lv_obj_set_style_text_color(mainpage_today_label, lv_color_hex(0xFF0080), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_today_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_today_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_today_label, 5, LV_PART_MAIN | LV_STATE_DEFAULT); // 设置行间距

        // 添加"阴转晴"标签
        mainpage_weather_desc_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_weather_desc_label, "阴转晴");
        lv_obj_set_pos(mainpage_weather_desc_label, 35,8);
        lv_obj_set_size(mainpage_weather_desc_label, 120, 35);
        lv_obj_set_style_text_color(mainpage_weather_desc_label, lv_color_hex(0x2169EB), LV_PART_MAIN | LV_STATE_DEFAULT); // 粉红色文字
        lv_obj_set_style_text_font(mainpage_weather_desc_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_weather_desc_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 添加"28/17"温度标签
        mainpage_temperature_range_label = lv_label_create(mainpage_weather);
        lv_label_set_text(mainpage_temperature_range_label, "28°/17°");
        lv_obj_set_pos(mainpage_temperature_range_label, 45, 40);
        lv_obj_set_size(mainpage_temperature_range_label, 100, 30);
        lv_obj_set_style_text_color(mainpage_temperature_range_label, lv_color_hex(0x2169EB), LV_PART_MAIN | LV_STATE_DEFAULT); // 黑色文字
        lv_obj_set_style_text_font(mainpage_temperature_range_label, &tiktok24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_temperature_range_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        

        // Write codes screen_year
        mainpage_year = lv_obj_create(mainpage_screen);
        lv_obj_set_pos(mainpage_year, -400, 82);
        lv_obj_set_size(mainpage_year, 305, 79);
        lv_obj_set_scrollbar_mode(mainpage_year, LV_SCROLLBAR_MODE_OFF);

        // 在UI创建时直接设置默认时间为20:42:47，包含完整的年月日和星期信息
        struct tm default_time = {0};
        default_time.tm_year = 123; // 2023 - 1900
        default_time.tm_mon = 7;    // 8月 (0-11)
        default_time.tm_mday = 31;  // 31日
        default_time.tm_hour = 20;
        default_time.tm_min = 42;
        default_time.tm_sec = 47;
        default_time.tm_wday = 4; // 周四 (0-6, 0是周日)
        update_mainpage_time_display(&default_time);

        // Write style for screen_year, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_year, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(mainpage_year, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(mainpage_year, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(mainpage_year, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_year, 19, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_year, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mainpage_year, lv_color_hex(0xffe4b5), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_year, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_year, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_year, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_year, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_year, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_year_label
        mainpage_year_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_year_label, "2023\n");
        lv_label_set_long_mode(mainpage_year_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_year_label, 75, 3);
        lv_obj_set_size(mainpage_year_label, 96, 29);

        // Write style for screen_year_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_year_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_year_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_year_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_year_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_year_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_weekday_label
        mainpage_weekday_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_weekday_label, "周四");

        lv_label_set_long_mode(mainpage_weekday_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_weekday_label, 217, 21);
        lv_obj_set_size(mainpage_weekday_label, 70, 35);

        // Write style for screen_weekday_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_weekday_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_weekday_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_weekday_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_weekday_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_weekday_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_year_text_label
        mainpage_year_text_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_year_text_label, "年");
        lv_label_set_long_mode(mainpage_year_text_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_year_text_label, 155, 3);
        lv_obj_set_size(mainpage_year_text_label, 52, 35);

        // Write style for screen_year_text_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_year_text_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_year_text_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_year_text_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_year_text_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_year_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_day_label
        mainpage_day_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_day_label, "31\n\n");
        lv_label_set_long_mode(mainpage_day_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_day_label, 140, 37);
        lv_obj_set_size(mainpage_day_label, 49, 40);

        // Write style for screen_day_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_day_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_day_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_day_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_day_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_day_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_day_text_label
        mainpage_day_text_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_day_text_label, "日");
        lv_label_set_long_mode(mainpage_day_text_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_day_text_label, 171, 38);
        lv_obj_set_size(mainpage_day_text_label, 52, 35);

        // Write style for screen_day_text_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_day_text_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_day_text_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_day_text_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_day_text_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_day_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_month_text_label
        mainpage_month_text_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_month_text_label, "月");
        lv_label_set_long_mode(mainpage_month_text_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_month_text_label, 109, 38);
        lv_obj_set_size(mainpage_month_text_label, 34, 35);

        // Write style for screen_month_text_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_month_text_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_month_text_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_month_text_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_month_text_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_month_text_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_month_label
        mainpage_month_label = lv_label_create(mainpage_year);
        lv_label_set_text(mainpage_month_label, "8\n");
        lv_label_set_long_mode(mainpage_month_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_month_label, 75, 37);
        lv_obj_set_size(mainpage_month_label, 60, 36);

        // Write style for screen_month_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_month_label, lv_color_hex(0x72B0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_month_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_month_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_month_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_month_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_point
        mainpage_point = lv_obj_create(mainpage_screen);
        lv_obj_set_pos(mainpage_point, 400, 81); // 400是 为了 初始设置之后看不到 不影响后续动画
        lv_obj_set_size(mainpage_point, 275, 79);
        lv_obj_set_scrollbar_mode(mainpage_point, LV_SCROLLBAR_MODE_OFF);

        // Write style for screen_point, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_point, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(mainpage_point, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(mainpage_point, lv_color_hex(0x02A9EB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(mainpage_point, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_point, 19, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_point, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(mainpage_point, lv_color_hex(0x02A9EB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_point, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_point, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_point, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_point, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_point, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 在湛江文字上方添加HOME图标
        lv_obj_t *mainpage_home_icon = lv_label_create(mainpage_point);
        lv_label_set_text(mainpage_home_icon, LV_SYMBOL_HOME);
        lv_obj_set_pos(mainpage_home_icon, 12, 3);

        lv_obj_set_style_text_color(mainpage_home_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_home_icon, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_home_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Write codes screen_city_label
        mainpage_city_label = lv_label_create(mainpage_point);
        lv_label_set_text(mainpage_city_label, "二");
        lv_label_set_long_mode(mainpage_city_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(mainpage_city_label, -10, 35);
        lv_obj_set_size(mainpage_city_label, 80, 35);

        // Write style for screen_city_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
        lv_obj_set_style_border_width(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(mainpage_city_label, lv_color_hex(0xFFA500), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(mainpage_city_label, &tiktok32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(mainpage_city_label, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_line_space(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(mainpage_city_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(mainpage_city_label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 时间相关UI元素已在函数开始位置创建
        // 动画完成回调函数

        // 直接使用全局时间变量初始化时间显示
        if (global_time_valid)
        {
            update_mainpage_time_display(&global_timeinfo);
        }
        else
        {
            // 如果时间无效，设置默认时间为20:42:47
            struct tm default_time = {0};
            default_time.tm_hour = 20;
            default_time.tm_min = 42;
            default_time.tm_sec = 47;
            update_mainpage_time_display(&default_time);
        }

        start_animation_state_machine();
        lv_scr_load(mainpage_screen);
        // 使用通用平移动画函数：mainpage_time从左边平移到右边
        lvgl_port_unlock();
    }
}

