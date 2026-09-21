/*
 * @file main_page.c
 * @brief 主界面管理任务，负责UI状态切换和页面控制
 * @details 本文件实现了系统的主界面任务，包括：
 *          1. UI状态管理（主页面、菜单、摄像头等）
 *          2. 页面创建和切换逻辑
 *          3. 按键处理和状态转换
 */
#include "headfile.h"
#include "audio_player.h"  // 添加音频播放器头文件
#include "file_iterator.h" // 添加文件迭代器头文件
#include "wifi_task.h" // 用于访问全局时间变量
#include "ui/mainmenu.h"    // 添加主菜单头文件

static const char *TAG = "main.page";


/**
 * @brief 更新时钟显示函数
 * @details 从时间管理模块获取当前时间并更新UI显示
 */
static void update_time_display(void)
{
    // 不再检查是否在主页面，让时间在任何页面都能更新
    
    // 双重检查：确保global_time_valid为true且年份不是1970年
    if (global_time_valid && global_timeinfo.tm_year >= (2020 - 1900))
    {
        // 调用UI更新函数显示时间
        update_mainpage_time_display(&global_timeinfo);
    }
    // 如果时间无效且WiFi已连接，可以考虑触发一次时间同步
    else if (get_wifi_status() == 1) {
        ESP_LOGI(TAG, "Time invalid when updating display, triggering NTP sync");
        // 这里不能直接调用obtain_time()，因为它可能在其他任务中运行
        // 我们只能记录日志，依赖continuous_time_sync任务来处理同步
    }
}

// 声明在main.c中定义的函数
extern ScrollingMenuItem *create_main_menu_items(void);

/* 全局任务和事件句柄 */
static TaskHandle_t menu_task_handle = NULL; // 菜单任务句柄
extern EventGroupHandle_t my_event_group;    // 系统事件组

/* 全局UI状态管理 */
ui_state_t current_ui_state = UI_STATE_MAIN_PAGE; // 当前UI状态，初始为主页面

/* 各页面屏幕对象 */
lv_obj_t *mainpage_screen = NULL;        // 主页面屏幕
lv_obj_t *menu_screen = NULL;            // 菜单屏幕
lv_obj_t *camera_screen = NULL;          // 摄像头屏幕
lv_obj_t *brightness_screen = NULL;      // 亮度设置屏幕
lv_obj_t *screen_dir_screen = NULL;      // 屏幕方向设置屏幕
lv_obj_t *info_display_screen = NULL;    // 信息显示屏幕
lv_obj_t *sdcard_screen = NULL;          // SD卡屏幕
lv_obj_t *ball_screen = NULL;            // 小球屏幕
lv_obj_t *audio_screen = NULL;           // 音乐播放屏幕

/**
 * @brief 主页面任务函数
 * @param pvParameters 任务参数（未使用）
 * @details 该函数是系统的主界面任务，负责：
 *          1. 初始化主页面和菜单
 *          2. 处理按键输入，进行页面切换
 *          3. 管理UI状态转换逻辑
 *          4. 实时更新时钟显示
 */
void main_page_task(void *pvParameters)
{
    // 等待开机音乐播放完成
    xEventGroupWaitBits(my_event_group, START_MUSIC_COMPLETED, pdFALSE, pdFALSE, portMAX_DELAY);
    delete_loading_anim(); // 删除加载动画
    /* 创建主页面屏幕 */
    if (lvgl_port_lock(100)) // 获取LVGL互斥锁，保证UI操作的线程安全
    {
        mainpage_screen = lv_obj_create(NULL); // 创建主页面屏幕对象
        if (mainpage_screen != NULL)
        {
            lv_scr_load(mainpage_screen); // 加载主页面屏幕
            lv_main_page();               // 在主页面屏幕上创建主页面内容
        }
        lvgl_port_unlock(); // 释放LVGL互斥锁
    }

    /* 创建菜单屏幕 */
    if (lvgl_port_lock(100)) // 获取LVGL互斥锁
    {
        menu_screen = lv_obj_create(NULL); // 创建菜单屏幕对象
        if (menu_screen != NULL)
        {
            // 切换到菜单屏幕
            lv_scr_load(menu_screen);
            // 在菜单屏幕上创建菜单
            menu_head = create_main_menu_items(); // 调用main.c中定义的函数创建菜单项

            gdlb_create_menu(menu_head);      // 创建菜单容器和基础结构
            gdlb_operate_menu(menu_head);     // 操作菜单，创建菜单项UI对象
            // 切换回主页面屏幕
            lv_scr_load(mainpage_screen);
        }
        lvgl_port_unlock(); // 释放LVGL互斥锁
    }
    ESP_LOGI(TAG, "here is main page");
    /* 主循环 - 处理按键输入和UI状态切换 */
    while (1)
    {
        if (is_key_pressed()) // 检测按键是否被按下
        {

            if (current_ui_state == UI_STATE_MAIN_PAGE)
            {
                /* 从主页面切换到菜单页面 */
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen); // 加载菜单屏幕
                        lvgl_port_unlock();
                    }
                }
                current_ui_state = UI_STATE_MENU; // 更新UI状态
                ESP_LOGI(TAG, " main page TO MENU"); // 记录状态切换日志
            }
            else if (current_ui_state == UI_STATE_MENU)
            {
                /* 从菜单页面切换回主页面 */
                if (mainpage_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(mainpage_screen); // 加载主页面屏幕
                        lvgl_port_unlock();
                    }
                }

                current_ui_state = UI_STATE_MAIN_PAGE; // 更新UI状态
                ESP_LOGI(TAG, " main page TO MAIN"); // 记录状态切换日志
            }
            else if (current_ui_state == UI_STATE_CAMERA)
            {
                /* 从摄像头页面切换回菜单页面 */
                printf("Main page task: Camera key pressed, returning to menu\n");

                // 停止摄像头显示
                camera_active = false;

                // 等待摄像头任务结束
                if (camera_task_handle != NULL)
                {
                    vTaskDelete(camera_task_handle);
                    camera_task_handle = NULL;
                }

                // 等待一段时间确保摄像头任务完全停止
                vTaskDelay(pdMS_TO_TICKS(100));

                // 切换回菜单界面
                current_ui_state = UI_STATE_MENU; // 更新UI状态
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, " CAMERA TO MENU"); // 记录状态切换日志
                }
            }
            else if (current_ui_state == UI_STATE_BRIGHTNESS)
            {
                // 当前是亮度设置页面，切换到菜单
                printf("Main page task: Brightness key pressed, returning to menu\n");

                // 切换回菜单界面
                current_ui_state = UI_STATE_MENU;
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, " BRIGHTNESS TO MENU");
                }
            }
            else if (current_ui_state == UI_STATE_SCREEN_DIR)
            {
                // 当前是屏幕反向设置页面，切换到菜单
                printf("Main page task: Screen direction key pressed, returning to menu\n");

                // 切换回菜单界面
                current_ui_state = UI_STATE_MENU;
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, " SCREEN_DIR TO MENU");
                }
            }
            else if (current_ui_state == UI_STATE_INFO_DISPLAY)
            {
                // 当前是信息显示页面，切换到菜单
                printf("Main page task: Info display key pressed, returning to menu\n");

                // 切换回菜单界面
                current_ui_state = UI_STATE_MENU;
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, " INFO_DISPLAY TO MENU");
                }
            }
            else if (current_ui_state == UI_STATE_SDCARD)
            {
                // 当前是SD卡页面，在根目录下返回菜单，其他目录下由屏幕返回按键处理
                // file_path_info已经在main_page.h中声明为外部变量
                if (file_path_info.path_index == 0)
                {
                    // 在根目录下，直接返回菜单
                    printf("Main page task: SD card key pressed in root directory, returning to menu\n");

                    // 卸载SD卡
                    bsp_sdcard_unmount();

                    // 切换回菜单界面
                    current_ui_state = UI_STATE_MENU;
                    if (menu_screen != NULL)
                    {
                        if (lvgl_port_lock(100))
                        {
                            lv_scr_load(menu_screen);
                            lvgl_port_unlock();
                        }
                        ESP_LOGI(TAG, " SDCARD TO MENU");
                    }
                }
            }
            else if (current_ui_state == UI_STATE_BALL)
            {
                // 当前是小球页面，按键返回菜单
                printf("Main page task: Ball page key pressed, returning to menu\n");

                // 保存UI状态变更前的日志
                ESP_LOGI(TAG, "BALL TO MENU - 开始切换");
                
                // 先停止小球任务（ball_task_stop只负责停止任务和清理资源）
                extern void ball_task_stop(void);
                ball_task_stop();

                // 确保menu_screen存在并切换
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        // 先保存ball_screen指针
                        lv_obj_t *temp_ball_screen = ball_screen;

                        // 最重要：先切换屏幕，确保显示切换成功
                        lv_scr_load(menu_screen);
                        ESP_LOGI(TAG, "屏幕已切换到菜单页面");

                        // 然后切换UI状态
                        // 注意：UI状态切换必须在main_page中进行
                        current_ui_state = UI_STATE_MENU;

                        // 最后释放ball_screen资源
                        if (temp_ball_screen != NULL)
                        {
                            lv_obj_del(temp_ball_screen);
                            ball_screen = NULL;
                            ESP_LOGI(TAG, "ball_screen已释放");
                        }

                        lvgl_port_unlock();
                    }
                    else
                    {
                        ESP_LOGE(TAG, "获取LVGL锁失败");
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "menu_screen不存在");
                }
                ESP_LOGI(TAG, "BALL TO MENU - 切换完成");
            }
            else if (current_ui_state == UI_STATE_AUDIO)
            {
                // 当前是音乐播放器页面，按键返回菜单
                printf("Main page task: Audio player key pressed, returning to menu\n");

                // 停止音乐播放并清理资源
                audio_player_stop();
                audio_player_delete();

                // 关闭音频功放
                extern void pa_en(uint8_t en);
                pa_en(0);

                // 清理file_iterator资源
                extern void *file_iterator;
                if (file_iterator != NULL)
                {
                    // 重置file_iterator指针
                    file_iterator = NULL;
                    ESP_LOGI(TAG, "音乐播放器: file_iterator指针已重置");
                }

                // 重置audio_screen指针
                extern lv_obj_t *audio_screen;
                if (audio_screen != NULL)
                {
                    audio_screen = NULL;
                    ESP_LOGI(TAG, "音乐播放器: audio_screen已重置");
                }

                // 重置音乐列表
                extern lv_obj_t *music_list;
                if (music_list != NULL)
                {
                    music_list = NULL;
                    ESP_LOGI(TAG, "音乐播放器: music_list已重置");
                }

                // 文件系统保持挂载状态，无需重置标志
                // 这样可以避免重复挂载操作，提高性能

                // 删除音乐播放器界面
                // 因为现在使用audio_screen而不是icon_in_obj，这里不需要显式删除
                // 屏幕切换会由系统自动处理
                ESP_LOGI(TAG, "切换到菜单界面");

                // 切换回菜单界面
                current_ui_state = UI_STATE_MENU;
                if (menu_screen != NULL)
                {
                    if (lvgl_port_lock(100))
                    {
                        lv_scr_load(menu_screen);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, "AUDIO TO MENU");
                }
            }
        }
        // 添加时间计数变量，每100ms检查一次按键，每10次（1秒）更新一次时间
        static uint8_t time_update_counter = 0;
        time_update_counter++;
        
        // 每10次循环（1秒）更新一次时间显示
        if (time_update_counter >= 10) {
            update_time_display();
            time_update_counter = 0;
        }
        
        // 降低延时到100ms，提高按键检测频率，减少按键响应延迟
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}