/*
 * @file music_task.c
 * @brief 音乐播放功能实现
 * @details 本文件实现了系统的音乐播放功能，包括：
 *          1. 音频播放器初始化和控制
 *          2. 音乐文件列表管理
 *          3. 播放/暂停、音量控制等操作
 *          4. 音乐播放界面的创建和更新
 */
#include "headfile.h"
LV_FONT_DECLARE(tiktok20);
LV_FONT_DECLARE(tiktok32);
static const char *TAG = "main.page";

audio_player_config_t player_config = {0}; // 音频播放器配置结构体
uint8_t g_sys_volume = VOLUME_DEFAULT; // 系统音量，初始值使用默认音量
file_iterator_instance_t *file_iterator = NULL; // 文件迭代器实例，用于遍历音乐文件

extern lv_obj_t *audio_screen; // 音频播放界面屏幕对象
// 添加音频互斥锁的外部声明（用于开机音乐）
extern SemaphoreHandle_t audio_mutex;
// 创建播放器专用的互斥锁（用于音乐播放器功能）
SemaphoreHandle_t player_mutex = NULL;

// 文件系统挂载状态标志
static bool spiffs_mounted = false;

/* 音频播放界面UI控件 */
lv_obj_t *music_list;         // 音乐文件列表对象
lv_obj_t *label_play_pause;   // 播放/暂停按钮文本标签
lv_obj_t *btn_play_pause;     // 播放/暂停按钮
lv_obj_t *volume_slider;      // 音量滑块

lv_obj_t *music_title_label;  // 音乐标题标签
lv_obj_t *btn_music_back;     // 音乐返回按钮

extern ui_state_t current_ui_state; // 当前UI状态
// 函数声明
void music_event_handler(lv_event_t * e);
void mp3_player_init(void);
static void music_ui(void);


/**
 * @brief 音乐播放功能入口函数
 * @details 当用户从菜单中选择音乐播放功能时调用此函数，主要完成：
 *          1. 初始化音乐播放器
 *          2. 创建并显示音乐播放界面
 *          3. 更新UI状态为音乐播放页面
 */
void action1(void)
{
    printf("Action action1 被执行，初始化音乐播放器\n");
    
    // 调用音乐事件处理函数初始化播放器
    music_event_handler(NULL);
    
    // 设置当前UI状态为音乐页面并切换屏幕（使用lv_port_lock保护）
    lvgl_port_lock(0);

    current_ui_state = UI_STATE_AUDIO; // 更新UI状态为音频播放状态
    
    // 加载音频屏幕
    if (audio_screen != NULL) {
        lv_scr_load(audio_screen); // 切换到音频播放屏幕
        ESP_LOGI(TAG, "已切换到音乐播放器屏幕");
    } else {
        ESP_LOGE(TAG, "audio_screen为NULL，无法切换屏幕");
    }
    
    lvgl_port_unlock(); // 释放LVGL互斥锁
    
    ESP_LOGI(TAG, "音乐播放器已启动，UI状态设置为AUDIO");
}


/**
 * @brief 根据索引播放音乐文件
 * @param index 要播放的音乐文件索引
 * @details 此函数通过索引查找对应的音乐文件路径，并开始播放该文件。
 *          使用互斥锁保护播放器资源访问，确保线程安全。
 */
static void play_index(int index)
{
    ESP_LOGI(TAG, "play_index(%d)", index);
    
    // 获取播放器互斥锁，确保播放器资源访问安全
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        char filename[128]; // 存储音乐文件的完整路径
        // 根据索引获取音乐文件的完整路径
        int retval = file_iterator_get_full_path_from_index(file_iterator, index, filename, sizeof(filename));
        if (retval == 0) { // 获取失败
            ESP_LOGE(TAG, "unable to retrieve filename");
            xSemaphoreGive(player_mutex); // 释放互斥锁
            return;
        }

        // 打开音乐文件
        FILE *fp = fopen(filename, "rb");
        if (fp) { // 文件打开成功
            ESP_LOGI(TAG, "Playing '%s'", filename);
            audio_player_play(fp); // 开始播放音乐
        } else { // 文件打开失败
            ESP_LOGE(TAG, "unable to open index %d, filename '%s'", index, filename);
        }
        
        // 释放播放器互斥锁
        xSemaphoreGive(player_mutex);
    } else { // 无法获取互斥锁
        ESP_LOGE(TAG, "play_index: Failed to take player_mutex");
    }
}
// 设置声音处理函数
static esp_err_t _audio_player_mute_fn(AUDIO_PLAYER_MUTE_SETTING setting)
{
    esp_err_t ret = ESP_OK;
    // 判断是否需要静音
    bsp_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);
    // 如果不是静音 设置音量
    if (setting == AUDIO_PLAYER_UNMUTE) {
        bsp_codec_volume_set(g_sys_volume, NULL);
    }
    ret = ESP_OK;

    return ret;
}
// 播放音乐函数 播放音乐的时候 会不断进入
static esp_err_t _audio_player_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    ret = bsp_i2s_write(audio_buffer, len, bytes_written, timeout_ms);

    return ret;
}

// 设置采样率 播放的时候进入一次
static esp_err_t _audio_player_std_clock(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    // ret = bsp_codec_set_fs(rate, bits_cfg, ch); // 如果播放的音乐固定是16000采样率 这里可以不用打开 如果采样率未知 把这里打开
    return ret;
}

// 回调函数 播放器每次动作都会进入
static void _audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    // 添加空指针检查
    if (ctx == NULL) {
        ESP_LOGE(TAG, "_audio_player_callback: ctx is NULL");
        return;
    }
    
    ESP_LOGI(TAG, "ctx->audio_event = %d", ctx->audio_event);
    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE: {  // 播放完一首歌 进入这个case
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_IDLE");
        
        // 获取播放器互斥锁，确保播放器资源访问安全
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // 指向下一首歌
            file_iterator_next(file_iterator);
            int index = file_iterator_get_index(file_iterator);
            ESP_LOGI(TAG, "playing index '%d'", index);
            play_index(index);
            
            // 释放播放器互斥锁
            xSemaphoreGive(player_mutex);
            
            // 修改当前播放的音乐名称（确保有锁保护）
            lvgl_port_lock(0);
            if (music_list != NULL) {  // 检查music_list是否有效
                lv_dropdown_set_selected(music_list, index);
            }
            lvgl_port_unlock();
        } else {
            ESP_LOGE(TAG, "_audio_player_callback: Failed to take audio_mutex");
        }
        break;
    }
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING: // 正在播放音乐
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PLAY");
        
        // 获取播放器互斥锁，确保播放器资源访问安全
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            pa_en(1); // 打开音频功放
            xSemaphoreGive(player_mutex);
        } else {
            ESP_LOGE(TAG, "_audio_player_callback: Failed to take player_mutex for playing");
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE: // 正在暂停音乐
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PAUSE");
        
        // 获取播放器互斥锁，确保播放器资源访问安全
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            pa_en(0); // 关闭音频功放
            xSemaphoreGive(player_mutex);
        } else {
            ESP_LOGE(TAG, "_audio_player_callback: Failed to take player_mutex for pause");
        }
        break;
    default:
        ESP_LOGI(TAG, "Other audio event: %d", ctx->audio_event);
        break;
    }
}

// mp3播放器初始化
void mp3_player_init(void)
{
    // 初始化播放器互斥锁
    if (player_mutex == NULL) {
        player_mutex = xSemaphoreCreateMutex();
        if (player_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create player_mutex");
            return;
        }
        ESP_LOGI(TAG, "player_mutex created successfully");
    }
    
    // 检查文件系统是否已经挂载，避免重复调用bsp_spiffs_mount
    if (!spiffs_mounted) {
        extern esp_err_t bsp_spiffs_mount(void);
        esp_err_t ret = bsp_spiffs_mount();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS filesystem: %s", esp_err_to_name(ret));
            return;
        }
        spiffs_mounted = true;
        ESP_LOGI(TAG, "SPIFFS filesystem mounted successfully");
    } else {
        ESP_LOGI(TAG, "SPIFFS filesystem already mounted, skipping mount");
    }
    
    // 获取文件信息
    file_iterator = file_iterator_new(SPIFFS_BASE);
    if (file_iterator == NULL) {
        ESP_LOGE(TAG, "Failed to create file iterator for path: %s", SPIFFS_BASE);
        return;
    }

    // 初始化音频播放
    player_config.mute_fn = _audio_player_mute_fn;
    player_config.write_fn = _audio_player_write_fn;
    player_config.clk_set_fn = _audio_player_std_clock;
    player_config.priority = 6;
    player_config.coreID = 1;

    ESP_ERROR_CHECK(audio_player_new(player_config));
    ESP_ERROR_CHECK(audio_player_callback_register(_audio_player_callback, NULL));
}


// 按钮样式相关定义
typedef struct {
    lv_style_t style_bg;
    lv_style_t style_focus_no_outline;
} button_style_t;

static button_style_t g_btn_styles;

button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

// 按钮样式初始化
static void ui_button_style_init(void)
{
    /*Init the style for the default state*/
    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);

    lv_style_init(&g_btn_styles.style_bg);
    lv_style_set_bg_opa(&g_btn_styles.style_bg, LV_OPA_100);
    lv_style_set_bg_color(&g_btn_styles.style_bg, lv_color_make(255, 255, 255));
    lv_style_set_shadow_width(&g_btn_styles.style_bg, 0);
}

// 播放暂停按钮 事件处理函数
static void btn_play_pause_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;

    audio_player_state_t state = audio_player_get_state();
    printf("state=%d\n", state);
    if(state == AUDIO_PLAYER_STATE_IDLE){
        // 使用lv_port_lock保护UI操作
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        lvgl_port_unlock(); // 先释放锁再执行可能耗时的音频操作
        int index = file_iterator_get_index(file_iterator);
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
        extern void pa_en(uint8_t en);
        pa_en(1);
    }else if (state == AUDIO_PLAYER_STATE_PAUSE) {
        // 使用lv_port_lock保护UI操作
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        lvgl_port_unlock(); // 先释放锁再执行可能耗时的音频操作
        audio_player_resume();
        extern void pa_en(uint8_t en);
        pa_en(1);
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
        // 使用lv_port_lock保护UI操作
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PLAY);
        lvgl_port_unlock(); // 先释放锁再执行可能耗时的音频操作
        audio_player_pause();
    }
}

// 上一首 下一首 按键事件处理函数
static void btn_prev_next_cb(lv_event_t *event)
{
    // 安全检查：确保event不为NULL
    if (event == NULL) {
        ESP_LOGE(TAG, "btn_prev_next_cb: event is NULL");
        return;
    }
    
    // 安全检查：确保event->user_data不为NULL（这里通过检查event本身已经确保了）
    bool is_next = (bool) event->user_data;

    // 获取音频互斥锁，确保音频资源访问安全
    if (xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (is_next) {
            ESP_LOGI(TAG, "btn next");
            file_iterator_next(file_iterator);
        } else {
            ESP_LOGI(TAG, "btn prev");
            file_iterator_prev(file_iterator);
        }
        
        // 修改当前的音乐名称
        int index = file_iterator_get_index(file_iterator);
        
        // 使用lv_port_lock保护UI操作
        lvgl_port_lock(0);
        if (music_list != NULL) {
            lv_dropdown_set_selected(music_list, index);
        } else {
            ESP_LOGE(TAG, "btn_prev_next_cb: music_list is NULL");
        }
        lvgl_port_unlock();
        
        // 执行音乐事件
        audio_player_state_t state = audio_player_get_state();
        ESP_LOGI(TAG, "prev_next_state=%d", state);
        
        if (state == AUDIO_PLAYER_STATE_IDLE) { 
            // Nothing to do
        } else if (state == AUDIO_PLAYER_STATE_PAUSE) { // 如果当前正在暂停歌曲
            ESP_LOGI(TAG, "playing index '%d'", index);
            play_index(index);
            audio_player_pause();
        } else if (state == AUDIO_PLAYER_STATE_PLAYING) { // 如果当前正在播放歌曲
            // 播放歌曲
            ESP_LOGI(TAG, "playing index '%d'", index);
            play_index(index);
        }
        
        // 释放音频互斥锁
        xSemaphoreGive(audio_mutex);
    } else {
        ESP_LOGE(TAG, "btn_prev_next_cb: Failed to take audio_mutex");
    }
}

// 音量调节滑动条 事件处理函数
static void volume_slider_cb(lv_event_t *event)
{
    // 安全检查：确保event不为NULL
    if (event == NULL) {
        ESP_LOGE(TAG, "volume_slider_cb: event is NULL");
        return;
    }
    
    // 使用lv_port_lock保护UI操作
    lvgl_port_lock(0);
    lv_obj_t *slider = lv_event_get_target(event);
    // 安全检查：确保slider不为NULL
    if (slider == NULL) {
        lvgl_port_unlock();
        ESP_LOGE(TAG, "volume_slider_cb: slider is NULL");
        return;
    }
    
    int volume = lv_slider_get_value(slider); // 获取slider的值
    // 先释放锁再执行其他操作
    lvgl_port_unlock();
    
    g_sys_volume = volume; // 把声音赋值给g_sys_volume保存
    ESP_LOGI(TAG, "volume '%d'", volume);
    
    // 获取音频互斥锁，确保音频资源访问安全
    if (xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        bsp_codec_volume_set(volume, NULL); // 设置声音大小
        // 释放音频互斥锁
        xSemaphoreGive(audio_mutex);
    } else {
        ESP_LOGE(TAG, "volume_slider_cb: Failed to take audio_mutex");
    }
}

// 音乐列表 点击事件处理函数
static void music_list_cb(lv_event_t *event)
{   
    // 安全检查：确保event不为NULL
    if (event == NULL) {
        ESP_LOGE(TAG, "music_list_cb: event is NULL");
        return;
    }
    
    // 获取音频互斥锁，确保音频资源访问安全
    if (xSemaphoreTake(audio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // 使用lv_port_lock保护UI操作
        lvgl_port_lock(0);
        // 安全检查：确保music_list不为NULL
        if (music_list == NULL) {
            lvgl_port_unlock();
            xSemaphoreGive(audio_mutex);
            ESP_LOGE(TAG, "music_list_cb: music_list is NULL");
            return;
        }
        
        uint16_t index = lv_dropdown_get_selected(music_list);
        lvgl_port_unlock(); // 先释放锁再执行其他操作
        
        ESP_LOGI(TAG, "switching index to '%d'", index);
        file_iterator_set_index(file_iterator, index);
        
        audio_player_state_t state = audio_player_get_state();
        if (state == AUDIO_PLAYER_STATE_PAUSE){ // 如果当前正在暂停歌曲
            play_index(index);
            audio_player_pause();
        } else if (state == AUDIO_PLAYER_STATE_PLAYING) { // 如果当前正在播放歌曲
            play_index(index);
        }
        
        // 释放音频互斥锁
        xSemaphoreGive(audio_mutex);
    } else {
        ESP_LOGE(TAG, "music_list_cb: Failed to take audio_mutex");
    }
}

// 音乐名称加入列表
static void build_file_list(lv_obj_t *music_list)
{
    lvgl_port_lock(0);
    lv_dropdown_clear_options(music_list);
    lvgl_port_unlock();

    for(size_t i = 0; i<file_iterator_get_count(file_iterator); i++)
    {
        const char *file_name = file_iterator_get_name_from_index(file_iterator, i);
        if (NULL != file_name) {
            lvgl_port_lock(0);
            lv_dropdown_add_option(music_list, file_name, i); // 添加音乐名称到列表中
            lvgl_port_unlock();
        }
    }
    lvgl_port_lock(0);
    lv_dropdown_set_selected(music_list, 0); // 选择列表中的第一个
    lvgl_port_unlock();
}

// 播放器界面初始化
void music_ui(void)
{
    lvgl_port_lock(0);

    ui_button_style_init();// 初始化按键风格

    /* 创建播放暂停控制按键 */
    btn_play_pause = lv_btn_create(audio_screen);
    lv_obj_align(btn_play_pause, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn_play_pause, 50, 50);
    lv_obj_set_style_radius(btn_play_pause, 25, LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_play_pause, LV_OBJ_FLAG_CHECKABLE);

    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    label_play_pause = lv_label_create(btn_play_pause);

    lv_label_set_text_static(label_play_pause, LV_SYMBOL_PLAY);
    lv_obj_center(label_play_pause);

    lv_obj_set_user_data(btn_play_pause, (void *) label_play_pause);
    lv_obj_add_event_cb(btn_play_pause, btn_play_pause_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 创建上一首控制按键 */
    lv_obj_t *btn_play_prev = lv_btn_create(audio_screen);
    lv_obj_set_size(btn_play_prev, 50, 50);
    lv_obj_set_style_radius(btn_play_prev, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_prev, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_prev, btn_play_pause, LV_ALIGN_OUT_LEFT_MID, -40, 0); 

    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_prev = lv_label_create(btn_play_prev);
    lv_label_set_text_static(label_prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_prev, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_prev);
    lv_obj_set_user_data(btn_play_prev, (void *) label_prev);
    lv_obj_add_event_cb(btn_play_prev, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) false);

    /* 创建下一首控制按键 */
    lv_obj_t *btn_play_next = lv_btn_create(audio_screen);
    lv_obj_set_size(btn_play_next, 50, 50);
    lv_obj_set_style_radius(btn_play_next, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_next, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_next, btn_play_pause, LV_ALIGN_OUT_RIGHT_MID, 40, 0);

    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_next = lv_label_create(btn_play_next);
    lv_label_set_text_static(label_next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_next, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_next);
    lv_obj_set_user_data(btn_play_next, (void *) label_next);
    lv_obj_add_event_cb(btn_play_next, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) true);

    /* 创建声音调节滑动条 */
    volume_slider = lv_slider_create(audio_screen);
    lv_obj_set_size(volume_slider, 200, 10);
    lv_obj_set_ext_click_area(volume_slider, 15);
    lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, g_sys_volume, LV_ANIM_ON);
    lv_obj_add_event_cb(volume_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lab_vol_min = lv_label_create(audio_screen);
    lv_label_set_text_static(lab_vol_min, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_style_text_font(lab_vol_min, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_min, volume_slider, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_t *lab_vol_max = lv_label_create(audio_screen);
    lv_label_set_text_static(lab_vol_max, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_font(lab_vol_max, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_max, volume_slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    /* 创建音乐列表 */ 
    music_list = lv_dropdown_create(audio_screen);
    lv_dropdown_clear_options(music_list);
    lv_dropdown_set_options_static(music_list, "扫描中...");
    lv_obj_set_style_text_font(music_list, &lv_font_montserrat_20, LV_STATE_ANY);
    lv_obj_set_width(music_list, 200);
    lv_obj_align(music_list, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_event_cb(music_list, music_list_cb, LV_EVENT_VALUE_CHANGED, NULL);

    build_file_list(music_list);

    lvgl_port_unlock();
}

// 返回主界面按钮事件处理函数
static void btn_music_back_cb(lv_event_t * e)
{
    // 安全检查：确保event不为NULL
    if (e == NULL) {
        ESP_LOGE(TAG, "btn_music_back_cb: event is NULL");
        return;
    }
    
    // 获取播放器互斥锁，确保播放器资源访问安全
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // 停止音乐播放
        audio_player_stop();
        
        // 删除音频播放器资源
        audio_player_delete();
        
        // 关闭音频功放
        extern void pa_en(uint8_t en);
        pa_en(0);
        
        // 释放播放器互斥锁
        xSemaphoreGive(player_mutex);
    } else {
        ESP_LOGE(TAG, "btn_music_back_cb: Failed to take player_mutex");
    }
    
  
    
    // 设置当前UI状态为菜单（使用lv_port_lock保护）
    lvgl_port_lock(0);
    extern ui_state_t current_ui_state;
    current_ui_state = UI_STATE_MENU;
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "音乐播放器返回按钮点击，切换到菜单");
}

// 进入音乐播放应用
void music_event_handler(lv_event_t * e)
{
    // 允许event为NULL，因为action1函数会直接调用此函数
    if (e == NULL) {
        ESP_LOGI(TAG, "music_event_handler: Called directly (not via LVGL event)");
        // 继续执行，不返回
    }
    
    // 确保获取音频互斥锁，避免与开机音乐冲突
    // 注意：此处使用audio_mutex仅用于初始化阶段避免与开机音乐冲突
    // 播放器运行时使用player_mutex进行保护
    if (xSemaphoreTake(audio_mutex, portMAX_DELAY) == pdTRUE)
    {
        // 初始化mp3播放器
        mp3_player_init();
        
        // 检查file_iterator是否初始化成功
        if (file_iterator == NULL) {
            xSemaphoreGive(audio_mutex);
            ESP_LOGE(TAG, "music_event_handler: mp3_player_init failed");
            return;
        }
        
        // 释放音频互斥锁
        xSemaphoreGive(audio_mutex);
    } else {
        ESP_LOGE(TAG, "music_event_handler: Failed to take audio_mutex for initialization");
        return; // 如果无法初始化播放器，直接返回
    }
    
    // 所有UI操作需要使用lv_port_lock保护
    lvgl_port_lock(0);
    
    // 确保audio_screen已创建
    if (audio_screen == NULL) {
        // 创建一个界面对象
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_radius(&style, 10);  
        lv_style_set_bg_opa( &style, LV_OPA_COVER );
        lv_style_set_bg_color(&style, lv_color_hex(0xffffff));
        lv_style_set_border_width(&style, 0);
        lv_style_set_pad_all(&style, 0);
        lv_style_set_width(&style, 320);  
        lv_style_set_height(&style, 240); 

        audio_screen = lv_obj_create(NULL); // 创建为独立屏幕而不是当前屏幕的子对象
        if (audio_screen == NULL) {
            lvgl_port_unlock();
            ESP_LOGE(TAG, "music_event_handler: Failed to create audio_screen");
            return;
        }
        lv_obj_add_style(audio_screen, &style, 0);
    } else {
        // 如果audio_screen已存在，清空所有子对象
        lv_obj_clean(audio_screen);
    }

    // 创建标题背景
    lv_obj_t *music_title = lv_obj_create(audio_screen);
    if (music_title == NULL) {
        lvgl_port_unlock();
        ESP_LOGE(TAG, "music_event_handler: Failed to create music_title");
        return;
    }
    lv_obj_set_size(music_title, 320, 40);
    lv_obj_set_style_pad_all(music_title, 0, 0);  // 设置间隙
    lv_obj_align(music_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(music_title, lv_color_hex(0xf87c30), 0);
    
    // 显示标题
    music_title_label = lv_label_create(music_title);
    if (music_title_label == NULL) {
        lvgl_port_unlock();
        ESP_LOGE(TAG, "music_event_handler: Failed to create music_title_label");
        return;
    }
    lv_label_set_text(music_title_label, "音乐播放器");
    lv_obj_set_style_text_color(music_title_label, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_text_font(music_title_label, &tiktok20, 0);
    lv_obj_align(music_title_label, LV_ALIGN_CENTER, 0, 0);
    
    // 取消创建返回键，标题居中显示
    lv_obj_align(music_title_label, LV_ALIGN_CENTER, 0, 0);
    
    // 释放锁后再调用music_ui，因为music_ui内部已经有锁保护
    lvgl_port_unlock();
    
    // 加载音乐播放器界面
    music_ui();
    
   
    ESP_LOGI(TAG, "音乐播放器界面已初始化");
}