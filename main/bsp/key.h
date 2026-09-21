#pragma once
// 按键状态枚举
typedef enum {
    KEY_STATE_IDLE,      // 空闲状态
    KEY_STATE_PRESSED,   // 按键按下
    KEY_STATE_RELEASED   // 按键释放
} key_state_t;


void key_init(void);
key_state_t get_key_state(void);
bool is_key_released(void);
bool is_key_pressed(void);
void app_key_init(void);
void app_key_task(void *pvParameters);