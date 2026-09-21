/*
 * @file camera_task.c
 * @brief 摄像头任务实现
 * @details 本文件实现了ESP32-S3摄像头功能，主要包括：
 *          1. 初始化摄像头硬件和屏幕显示
 *          2. 采集摄像头图像数据
 *          3. 在LCD屏幕上实时显示摄像头画面
 *          4. 提供摄像头功能的启动和退出机制
 *          5. 实现摄像头功能的UI界面
 */
#include "headfile.h"

static const char *TAG = "camera_task";

TaskHandle_t camera_task_handle;
bool camera_active = false;

lv_obj_t *img_camera = NULL;
lv_img_dsc_t img_camera_dsc;
lv_img_dsc_t img_camera_dsc = {
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 320,
    .header.h = 240,
    .data_size = 240 * 320 * 2,
};

// 摄像头处理任务
/**
 * @brief 摄像头数据处理任务
 * @details 循环采集摄像头图像数据并在LCD上显示，处理按键事件实现退出功能
 * @param arg 任务参数（未使用）
 */
static void task_process_camera(void *arg)
{
    while (camera_active)
    {
        camera_fb_t *frame = esp_camera_fb_get();
        img_camera_dsc.data = frame->buf;

        // 添加锁保护LVGL操作
        if (lvgl_port_lock(100))
        {
            lv_img_set_src(img_camera, &img_camera_dsc);
            lvgl_port_unlock();
        }
        esp_camera_fb_return(frame);
    }

    // 摄像头任务结束，清理资源
    esp_camera_deinit(); // 取消初始化摄像头
    lvgl_port_lock(0);
    if (camera_screen != NULL)
    {
        lv_obj_del(camera_screen); // 删除摄像头画布
        camera_screen = NULL;
    }
    lvgl_port_unlock();
    dvp_pwdn(1); // 摄像头进入掉电模式
    vTaskDelete(NULL);
}
/**
 * @brief 初始化摄像头屏幕
 * @details 创建摄像头显示界面，包括背景和退出按钮
 */
static void init_camera_screen(void)
{
    // 添加锁保护LVGL操作
    if (lvgl_port_lock(100))
    {
        static lv_style_t style;
        // 创建摄像头屏幕

        lv_style_init(&style);
        lv_style_set_radius(&style, 10);
        lv_style_set_bg_opa(&style, LV_OPA_COVER);
        lv_style_set_bg_color(&style, lv_color_hex(0xcccccc));
        lv_style_set_border_width(&style, 0);
        lv_style_set_pad_all(&style, 0);
        lv_style_set_width(&style, 320);
        lv_style_set_height(&style, 240);

        camera_screen = lv_obj_create(NULL);
        lv_obj_add_style(camera_screen, &style, 0);

        // 创建摄像头图像对象
        img_camera = lv_img_create(camera_screen);
        lv_obj_set_pos(img_camera, 0, 0);
        lv_obj_set_size(img_camera, 320, 240); // 根据您的屏幕尺寸调整

        lvgl_port_unlock();
    }
}
/**
 * @brief 摄像头功能入口函数
 * @details 初始化摄像头功能，创建摄像头任务，设置UI状态并加载摄像头屏幕
 */
void action3(void)
{
    // 切换到摄像头界面
    current_ui_state = UI_STATE_CAMERA;
    printf("Action 3 UI_STATE_CAMERA 被执行\n");

    init_camera_screen();
    // 确保摄像头只初始化一次
    static bool camera_initialized = false;
    if (!camera_initialized)
    {
        bsp_camera_init();
        camera_initialized = true;
    }

    // 添加锁保护LVGL操作
    if (lvgl_port_lock(100))
    {
        // 加载摄像头屏幕
        lv_scr_load(camera_screen);
        lvgl_port_unlock();
    }

    // 设置摄像头活动标志
    camera_active = true;

    // 创建摄像头处理任务
    xTaskCreatePinnedToCore(task_process_camera, "task_process_camera", 4 * 1024, NULL, 5, &camera_task_handle, 1);
}
