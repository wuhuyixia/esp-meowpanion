#pragma once

// 定义文件路径信息结构体
typedef struct
{
    uint8_t path_index;  // 在第几级目录
    char path_now[512];  // 当前文件路径
    char path_back[512]; // 上级文件路径
} file_path_info_t;

extern file_path_info_t file_path_info;


// 定义UI状态枚举
typedef enum
{
    UI_STATE_MAIN_PAGE,    // 主页面状态
    UI_STATE_MENU,         // 菜单状态
    UI_STATE_CAMERA,       // 摄像头状态
    UI_STATE_BRIGHTNESS,   // 亮度设置状态
    UI_STATE_SCREEN_DIR,   // 屏幕方向设置状态
    UI_STATE_INFO_DISPLAY, // 信息显示状态
    UI_STATE_SDCARD,       // SD卡状态
    UI_STATE_BALL,         // 小球页面状态
    UI_STATE_AUDIO         // 音乐页面状态
} ui_state_t;

extern lv_obj_t *mainpage_screen;
extern lv_obj_t *menu_screen;
extern lv_obj_t *camera_screen;
extern lv_obj_t *brightness_screen;
extern lv_obj_t *screen_dir_screen;
extern lv_obj_t *info_display_screen;
extern lv_obj_t *sdcard_screen;
extern lv_obj_t *ball_screen;
extern lv_obj_t *audio_screen;

extern ui_state_t current_ui_state;
void main_page_task(void *pvParameters);