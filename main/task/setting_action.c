/*
 * @file setting_action.c
 * @brief 系统设置功能实现
 * @details 本文件实现了ESP32-S3系统的设置功能，主要包括：
 *          1. 屏幕亮度调节功能
 *          2. 屏幕方向控制（正向/反向）
 *          3. 系统信息显示
 *          4. 各设置项的UI界面创建和事件处理
 */
#include "headfile.h"

// 外部屏幕对象

extern lv_obj_t *mainpage_screen;
extern lv_obj_t *menu_screen;
extern lv_obj_t *camera_screen;
extern lv_obj_t *brightness_screen;
extern lv_obj_t *screen_dir_screen;
extern lv_obj_t *info_display_screen; // 信息显示屏幕

// UI state variable
extern ui_state_t current_ui_state;

/**
 * @brief 亮度滑块事件回调函数
 * @details 响应亮度调节滑块的变化，实时更新屏幕亮度
 * @param e LVGL事件对象
 */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int brightness = lv_slider_get_value(slider);
    bsp_display_brightness_set(brightness); // 设置屏幕亮度
}
/**
 * @brief 屏幕方向切换事件回调函数
 * @details 响应屏幕方向开关的变化，控制屏幕旋转方向（正向/反向180度）
 * @param e LVGL事件对象
 */
static void screen_dir_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    lv_disp_t *disp = lv_disp_get_default();
    lv_obj_t *status_label = (lv_obj_t *)lv_obj_get_user_data(sw);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED))
    {
        // 开关开启，设置屏幕反向显示（180度旋转）
        lv_disp_set_rotation(disp, LV_DISP_ROT_180);
        lv_label_set_text((lv_obj_t *)status_label, "Reversed");
        printf("Screen direction set to reverse (180 degrees)\n");
    }
    else
    {
        // 开关关闭，设置屏幕正向显示（0度）
        lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);
        lv_label_set_text((lv_obj_t *)status_label, "Normal");
        printf("Screen direction set to normal (0 degrees)\n");
    }
}
/**
 * @brief 亮度设置功能入口
 * @details 创建亮度调节界面，包含滑块控制器，实现屏幕亮度的实时调节
 */
void action4_1(void)
{
    /* 创建亮度设置界面（如果尚未创建） */
    if (brightness_screen == NULL)
    {
        brightness_screen = lv_obj_create(NULL); // 创建亮度设置屏幕

        // 创建亮度调节滑块
        lv_obj_t *slider = lv_slider_create(brightness_screen);
        lv_obj_set_width(slider, 200);
        lv_obj_set_pos(slider, 50, 100);
        lv_slider_set_range(slider, 0, 100);  // 设置亮度范围0-100
        lv_slider_set_value(slider, 50, LV_ANIM_OFF); // 默认亮度50%
        lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // 添加事件回调

        // 创建滑块标签
        lv_obj_t *label = lv_label_create(brightness_screen);
        lv_label_set_text(label, "Brightness");
        lv_obj_set_pos(label, 50, 80);

        /* 初始化显示器亮度 */
        bsp_display_brightness_init();
        bsp_display_brightness_set(50); // 设置初始亮度为50%
    }

    /* 加载亮度设置屏幕 */
    lv_scr_load(brightness_screen);

    /* 更新UI状态 */
    current_ui_state = UI_STATE_BRIGHTNESS;
}
/**
 * @brief 系统信息显示功能入口
 * @details 创建系统信息显示界面，展示项目标题、描述、功能特性和版本号
 */
void action4_2(void)
{
    /* 创建信息显示界面（如果尚未创建） */
    if (info_display_screen == NULL)
    {
        info_display_screen = lv_obj_create(NULL); // 创建信息显示屏幕

        // 设置屏幕背景为黑色
        lv_obj_set_style_bg_color(info_display_screen, lv_color_black(), 0);

        // 创建标题标签
        lv_obj_t *title_label = lv_label_create(info_display_screen);
        lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
        lv_label_set_text(title_label, "ESP32-S3 Smart Display");
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

        // 创建项目描述标签
        lv_obj_t *desc_label = lv_label_create(info_display_screen);
        lv_obj_set_style_text_color(desc_label, lv_color_white(), 0);
        lv_label_set_text(desc_label, "Multi-functional smart display system");
        lv_obj_align(desc_label, LV_ALIGN_TOP_MID, 0, 50);

        // 创建功能特性标签
        lv_obj_t *features_label = lv_label_create(info_display_screen);
        lv_obj_set_style_text_color(features_label, lv_color_white(), 0);
        lv_label_set_text(features_label, "Features: GUI, Display, Audio, WiFi");
        lv_obj_align(features_label, LV_ALIGN_CENTER, 0, 0);

        // 创建版本标签
        lv_obj_t *version_label = lv_label_create(info_display_screen);
        lv_label_set_text(version_label, "Version: V1.4.0");
        lv_obj_align(version_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    }

    /* 加载信息显示屏幕 */
    lv_scr_load(info_display_screen);

    /* 更新UI状态 */
    current_ui_state = UI_STATE_INFO_DISPLAY;
}

/**
 * @brief 屏幕方向控制功能入口
 * @details 创建屏幕方向控制界面，提供开关控件实现屏幕方向的正向/反向切换
 */
void action4_3(void)
{
    /* 创建屏幕方向控制界面（如果尚未创建） */
    if (screen_dir_screen == NULL)
    {
        screen_dir_screen = lv_obj_create(NULL); // 创建屏幕方向控制屏幕

        // 创建标题标签
        lv_obj_t *title_label = lv_label_create(screen_dir_screen);
        lv_label_set_text(title_label, "Screen Direction Control");
        lv_obj_set_pos(title_label, 50, 30);

        // 创建描述标签
        lv_obj_t *desc_label = lv_label_create(screen_dir_screen);
        lv_label_set_text(desc_label, "Toggle switch to rotate screen");
        lv_obj_set_pos(desc_label, 50, 60);

        // 创建屏幕方向控制开关
        lv_obj_t *sw = lv_switch_create(screen_dir_screen);
        lv_obj_set_pos(sw, 50, 100);
        lv_obj_add_event_cb(sw, screen_dir_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // 添加事件回调

        // 创建状态标签（显示当前屏幕方向）
        lv_obj_t *status_label = lv_label_create(screen_dir_screen);
        lv_label_set_text(status_label, "Normal"); // 默认显示正向
        lv_obj_set_pos(status_label, 120, 100);

        // 将状态标签存储在开关的用户数据中，以便回调函数更新
        lv_obj_set_user_data(sw, status_label);

        // 创建操作说明标签
        lv_obj_t *instruction_label = lv_label_create(screen_dir_screen);
        lv_label_set_text(instruction_label, "Press physical button to return to menu");
        lv_obj_set_pos(instruction_label, 50, 150);
    }

    /* 加载屏幕方向控制屏幕 */
    lv_scr_load(screen_dir_screen);

    /* 更新UI状态 */
    current_ui_state = UI_STATE_SCREEN_DIR;
}
