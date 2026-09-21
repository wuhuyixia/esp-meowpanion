/*
 * @file key.c
 * @brief 硬件按键处理模块实现
 * @details 本文件实现了ESP32-S3硬件按键的初始化、状态检测和事件处理功能，主要包括：
 *          1. 按键硬件初始化
 *          2. 按键状态检测与消抖
 *          3. 按键事件判断与处理
 *          4. 按键应用任务创建
 */
#include "headfile.h"



// 按键状态变量
static volatile key_state_t key_state = KEY_STATE_IDLE;
static volatile uint32_t last_key_time = 0;
static volatile bool debounced_key_pressed = false;

// 按键中断服务函数
static void IRAM_ATTR key_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 消抖处理
    if ((current_time - last_key_time) > DEBOUNCE_TIME_MS) {
        // 读取当前电平状态
        int level = gpio_get_level(KEY_GPIO);
        
        if (level == 0) { // 按键按下
            key_state = KEY_STATE_PRESSED;
            debounced_key_pressed = true;
        } else if (level == 1 && debounced_key_pressed) { // 按键释放且之前已经确认按下
            key_state = KEY_STATE_RELEASED;
            debounced_key_pressed = false;
        }
        
        last_key_time = current_time;
    }
}

// 初始化按键
/**
 * @brief 按键初始化函数
 * @details 配置按键GPIO引脚，设置为输入模式，并启用内部上拉电阻
 * @return 无返回值
 */
void key_init(void)
{
    // GPIO配置结构体
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // 下降沿中断
        .mode = GPIO_MODE_INPUT,        // 输入模式
        .pin_bit_mask = 1 << KEY_GPIO,  // 选择GPIO0
        .pull_down_en = 0,              // 禁能内部下拉
        .pull_up_en = 1                 // 使能内部上拉
    };
    
    // 根据上面的配置设置GPIO
    gpio_config(&io_conf);
    
    // 安装GPIO中断服务
    gpio_install_isr_service(0);
    
    // 添加中断处理程序
    gpio_isr_handler_add(KEY_GPIO, key_isr_handler, (void*) KEY_GPIO);
}

// 获取按键状态
/**
 * @brief 获取按键当前状态
 * @details 读取按键GPIO电平，进行消抖处理，并更新按键状态
 * @return 当前按键状态（KEY_STATE_IDLE/KEY_STATE_PRESSED/KEY_STATE_RELEASED）
 */
key_state_t get_key_state(void)
{
    // 直接返回当前状态，不再自动转换状态
    // 状态转换应该由中断处理程序和is_key_pressed/is_key_released函数处理
    return key_state;
}

// 检查按键是否被按下
/**
 * @brief 检查按键是否被按下
 * @details 判断按键是否从空闲状态切换到按下状态
 * @return 如果按键被按下返回true，否则返回false
 */
bool is_key_pressed(void)
{
    // 保存当前状态用于返回
    bool result = (get_key_state() == KEY_STATE_PRESSED);
    
    if (result) {
        // 重置状态，避免重复检测
        key_state = KEY_STATE_IDLE;
        debounced_key_pressed = false; // 确保消抖标志也被重置
    }
    
    return result;
}

// 检查按键是否被释放
/**
 * @brief 检查按键是否被释放
 * @details 判断按键是否从按下状态切换到释放状态
 * @return 如果按键被释放返回true，否则返回false
 */
bool is_key_released(void)
{
    if (get_key_state() == KEY_STATE_RELEASED) {
        // 重置状态，避免重复检测
        key_state = KEY_STATE_IDLE;
        return true;
    }
    return false;
}

// 在应用程序中初始化按键
/**
 * @brief 应用按键初始化
 * @details 初始化按键硬件并创建按键处理任务
 * @return 无返回值
 */
void app_key_init(void)
{
    key_init();
    ESP_LOGI("KEY", "Key initialized on GPIO %d", KEY_GPIO);
}

// 在应用程序的任务中检查按键状态
/**
 * @brief 按键应用任务
 * @details 在任务中循环检查按键状态，处理按键事件
 * @param pvParameters 任务参数（未使用）
 * @return 无返回值
 */
void app_key_task(void *pvParameters)
{
    while (1) {
        // 检查按键是否被按下
        if (is_key_pressed()) {
            ESP_LOGI("KEY", "Key pressed!");
            // 在这里添加按键按下后的处理代码
        }
        
        // 检查按键是否被释放
        if (is_key_released()) {
            ESP_LOGI("KEY", "Key released!");
            // 在这里添加按键释放后的处理代码
        }
        
        // 延时一段时间，避免过度占用CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ... existing code ...