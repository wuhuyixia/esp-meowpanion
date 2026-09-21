/*
 * @file main.c
 * @brief ESP32-S3系统入口程序
 * @details 本文件是ESP32-S3系统的主入口，主要负责：
 *          1. 系统初始化（NVS、I2C、LVGL、音频编解码器等）
 *          2. 创建系统关键任务（开机音乐、主页面等）
 *          3. 初始化系统资源（事件组、互斥锁等）
 *          4. 提供菜单项创建等公共功能
 */
#include "headfile.h"
#include "ota_app.h"
#include "ota_web.h"

static const char *TAG = "main";
EventGroupHandle_t my_event_group;
SemaphoreHandle_t audio_mutex;

ScrollingMenuItem *create_main_menu_items(void)
{
    ScrollingMenuItem *menu_head = gdlb_create_item(LV_SYMBOL_AUDIO, action1, 1, NULL);     // 串口收发
    menu_head->next = gdlb_create_item(LV_SYMBOL_SD_CARD, action2, 1, NULL);                // 读取挂载sd卡
    menu_head->next->next = gdlb_create_item(LV_SYMBOL_IMAGE, action3, 1, NULL);            // 摄像头页面
    menu_head->next->next->next = gdlb_create_item(LV_SYMBOL_SETTINGS, NULL, 1, NULL);      // 设置页面
    menu_head->next->next->next->next = gdlb_create_item(LV_SYMBOL_STOP, action5, 1, NULL); // 停止页面

    menu_head->next->next->next->sub = gdlb_create_item(LV_SYMBOL_CHARGE, action4_1, 2, menu_head);              // 设置亮度页面
    menu_head->next->next->next->sub->next = gdlb_create_item(LV_SYMBOL_EYE_OPEN, action4_2, 2, menu_head);      // 开发信息页面
    menu_head->next->next->next->sub->next->next = gdlb_create_item(LV_SYMBOL_SHUFFLE, action4_3, 2, menu_head); // 设置屏幕反向页面

    return menu_head;
}

/**
 * @brief 应用程序入口函数
 * @details ESP32-S3系统启动后执行的第一个用户函数，主要完成：
 *          1. 非易失性存储(NVS)初始化
 *          2. 硬件外设初始化（I2C、IO扩展、LVGL、音频编解码器等）
 *          3. 系统资源创建（事件组、互斥锁）
 *          4. 关键任务创建（开机音乐、主页面）
 */
void app_main(void)
{
    /* 初始化非易失性存储(NVS) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* 如果NVS存储空间已满或版本不兼容，则擦除后重新初始化 */
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    /*
     * 先初始化 OTA 应用层：创建写入锁和状态锁。
     * 后续 ota_mark_valid()、ota_web_start() 以及 HTTP 上传处理器都会
     * 依赖这里创建的资源，因此必须早于网络和 Web 服务器启动。
     */
    ESP_ERROR_CHECK(ota_app_init());
    ESP_ERROR_CHECK(wifi_sta_init());
    /* 硬件外设初始化 */
    ESP_ERROR_CHECK(bsp_i2c_init());   // I2C初始化，用于与外设通信
    pca9557_init();   // IO扩展芯片初始化，扩展IO端口
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口，用于UI显示
    bsp_codec_init(); // 音频初始化，配置音频编解码器
    qmi8658_init();   // 初始化加速度传感器
    loading_anim();   // 显示启动加载动画
    app_key_init();   // 初始化按键处理
   
    my_event_group = xEventGroupCreate(); // 创建事件组，用于任务间通信

    /* 创建音频互斥锁，确保双核资源访问安全 */
    audio_mutex = xSemaphoreCreateMutex();
   
    /* 创建系统关键任务 */
    // 创建开机音乐任务，优先级5，分配到CPU核心1
    xTaskCreatePinnedToCore(power_music_task, "power_music_task", 4 * 768, NULL, 5, NULL, 1);
    
    // 无论WiFi是否连接，都先创建主页面任务，确保UI能正常显示
    xTaskCreatePinnedToCore(main_page_task, "main_page_task", 4 * 1024, NULL, 6, NULL, 0);
    
     
    // 只在WiFi连接成功时创建SNTP任务
    if (get_wifi_status()) {
        xTaskCreatePinnedToCore(sntp_task, "sntp_task", 4*1152, NULL, 4, NULL, 1);
        xTaskCreatePinnedToCore(weather_task, "weather_task", 4*1152, NULL, 4, NULL, 1);
    } else {
        ESP_LOGI(TAG, "WiFi not connected, will start SNTP task and weather task when WiFi connects");
        // 创建一个任务来监控WiFi状态，当WiFi连接后再启动SNTP任务
        xTaskCreatePinnedToCore(wifi_monitor_task, "wifi_monitor_task", 2048, NULL, 3, NULL, 1);
    }

    /*
     * 确认待验证的新固件：
     *
     * OTA 写入成功后，Bootloader 会在下一次启动时从新分区运行。由于
     * sdkconfig 开启了 CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE，新固件
     * 必须在完成基本初始化后调用 ota_mark_valid()，否则它仍可能被视为
     * 未确认镜像，并在异常重启时回退到旧分区。
     *
     * 这里不等待 Wi-Fi、天气等业务服务：网络服务暂时失败不应影响已经
     * 能够正常启动的固件被确认。
     */
    ota_mark_valid();

    /*
     * 最后启动 Web OTA 服务。这样做有两个好处：
     * 1. 页面只有在核心应用已经初始化后才可访问，避免升级过程中 UI/
     *    音频/摄像头等资源还未准备好；
     * 2. 设备联网后可通过 http://<设备IP>/ 访问升级页面，上传当前工程
     *    生成的 .bin 文件。服务监听端口由 ota_web.c 固定为 80。
     */
    ESP_ERROR_CHECK(ota_web_start());
}
