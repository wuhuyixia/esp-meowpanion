#include "headfile.h"
lv_obj_t * sdcard_title; // SD卡页面标题背景
lv_obj_t * sdcard_label; // SD卡页面标题
lv_obj_t * sdcard_file_list; // SD卡文件列表
lv_obj_t * icon_in_obj; // SD卡页面内容容器

extern sdmmc_card_t *sdmmc_card;
static const char *TAG = "sdcard";

LV_FONT_DECLARE(tiktok20);
LV_FONT_DECLARE(tiktok32);
// 使用与main_page.h中一致的typedef定义
file_path_info_t file_path_info;

// 函数声明
esp_err_t list_sdcard_files(char * path);
static void file_list_btn_cb(lv_event_t * e); 

// 返回主界面按钮事件处理函数
static void btn_sdback_cb(lv_event_t * e)
{
    if (file_path_info.path_index == 0){ // 如果当前是根目录
        // 根目录下的返回操作由main_page.c中的UI_STATE_SDCARD逻辑处理
        // 这里不做任何处理
    }else{ // 非根目录下，返回上一级目录
        // 非根目录下，返回上一级目录
        lv_obj_clean(sdcard_file_list); // 清除当前文件列表
        esp_err_t ret = list_sdcard_files(file_path_info.path_back); // 列出上一级目录文件
        if (ret == ESP_OK){ // 如果成功列出目录
                strcpy(file_path_info.path_now, file_path_info.path_back); // 刚刚进入的这个目录路径 变成当前路径
                file_path_info.path_index--; // 目录级数索引退一级
                // 计算再向下退一级的目录路径
                char *slash = strrchr(file_path_info.path_back, '/'); // 从后往前查找字符'/'
                if (slash!= NULL) { // 如果查找到
                    *slash = '\0'; // 替换为NULL 表示字符串结束
                }
                ESP_LOGI(TAG, "path_index: %d", file_path_info.path_index);
                ESP_LOGI(TAG, "path_now: %s", file_path_info.path_now);
                ESP_LOGI(TAG, "path_back: %s", file_path_info.path_back);
                
                // 检查是否返回到了根目录
                if (file_path_info.path_index == 0) {
                    // 如果是根目录，隐藏返回按钮
                    lvgl_port_lock(0);
                    // 清空标题区域
                    lv_obj_clean(sdcard_title);
                    // 重新创建标题标签（不包含返回按钮）
                    sdcard_label = lv_label_create(sdcard_title);
                    if (sdmmc_card != NULL) {
                        lv_label_set_text_fmt(sdcard_label, "SD: %lluGB",
                            (((uint64_t)sdmmc_card->csd.capacity) * sdmmc_card->csd.sector_size) >> 30);
                    } else {
                        lv_label_set_text(sdcard_label, "SD卡");
                    }
                    lv_obj_set_style_text_color(sdcard_label, lv_color_hex(0xffffff), 0); 
                    lv_obj_set_style_text_font(sdcard_label, &lv_font_montserrat_24, 0);
                    lv_obj_align(sdcard_label, LV_ALIGN_CENTER, 0, 0);
                    lvgl_port_unlock();
                }
            }
    }
}

// 列出SD卡中的文件
esp_err_t list_sdcard_files(char * path) 
{
    esp_err_t ret;
    DIR *dir;
    struct dirent *ent;
    lv_obj_t * btn;
    if ((dir = opendir(path))!= NULL) { // 打开目录
        while ((ent = readdir(dir))!= NULL) { // 读取目录中的文件
            /* 常规文件处理 */
            if (ent->d_type == DT_REG){ // 如果是常规文件
                int file_type_flag = 0;
                const char* extension = strrchr(ent->d_name, '.'); // 从后往前 找到字符'.'
                if (extension != NULL) { // 如果找到了'.'
                    extension++; // 指针地址+1
                    if (strcmp(extension, "mp3") == 0) { // 如果是mp3
                        file_type_flag = 1; // 标记为音乐文件
                    } else if (strcmp(extension, "wav") == 0) { // 如果是wav
                        file_type_flag = 1; // 标记为音乐文件
                    } else if (strcmp(extension, "mp4") == 0) {
                        file_type_flag = 2; // 标记为视频文件
                    } else if (strcmp(extension, "avi") == 0) {
                        file_type_flag = 2; // 标记为视频文件
                    } else if (strcmp(extension, "jpg") == 0) {
                        file_type_flag = 3; // 标记为图片
                    } else if (strcmp(extension, "jpeg") == 0) {
                        file_type_flag = 3; // 标记为图片
                    } else if (strcmp(extension, "png") == 0) {
                        file_type_flag = 3; // 标记为图片
                    } else if (strcmp(extension, "bmp") == 0) {
                        file_type_flag = 3; // 标记为图片
                    } else if (strcmp(extension, "gif") == 0) {
                        file_type_flag = 3; // 标记为图片
                    } else {
                        file_type_flag = 0; // 除了以上文件 其它文件都归类为普通文件
                    }
                }
                lvgl_port_lock(0);
                switch (file_type_flag)
                {
                case 1:
                    btn = lv_list_add_btn(sdcard_file_list, LV_SYMBOL_AUDIO, (const char *)ent->d_name);  // 显示音乐文件图标
                    break;
                case 2:
                    btn = lv_list_add_btn(sdcard_file_list, LV_SYMBOL_VIDEO, (const char *)ent->d_name);  // 显示视频文件图标
                    break;
                case 3:
                    btn = lv_list_add_btn(sdcard_file_list, LV_SYMBOL_IMAGE, (const char *)ent->d_name);  // 显示图片文件图标
                    break;
                default:
                    btn = lv_list_add_btn(sdcard_file_list, LV_SYMBOL_FILE, (const char *)ent->d_name);  // 显示普通文件图标
                    break;
                }
                lv_obj_t *icon = lv_obj_get_child(btn, 0); // 获取图标指针
                lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0); // 修改图标的字体    
                lv_obj_add_event_cb(btn, file_list_btn_cb, LV_EVENT_CLICKED, NULL);  // 添加点击回调函数
                lvgl_port_unlock();
            }
            /* 文件夹处理 */
            else if (ent->d_type == DT_DIR) { // 如果是文件夹
                lvgl_port_lock(0);
                btn = lv_list_add_btn(sdcard_file_list, LV_SYMBOL_DIRECTORY, (const char *)ent->d_name); 
                lv_obj_t *icon = lv_obj_get_child(btn, 0); // 获取图标指针
                lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0); // 修改图标的字体
                lv_obj_add_event_cb(btn, file_list_btn_cb, LV_EVENT_CLICKED, NULL); // 添加点击回调函数
                lvgl_port_unlock();
            }
        }
        closedir(dir);
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to open directory %s.", path);
        ret = ESP_FAIL;
    }
    return ret;
}

// 文件点击 事件处理函数
static void file_list_btn_cb(lv_event_t * e)
{
    const char *file_name = NULL; // 当前文件名称
    // 获取点击的按钮名称 即文件名称
    file_name = lv_list_get_btn_text(lv_obj_get_parent(e->target), e->target);
    ESP_LOGI(TAG, "file name: %s", file_name);
    // 列出 SD 卡中的文件
    struct stat st; // 获取文件状态信息结构体
    strcpy(file_path_info.path_back, file_path_info.path_now); // 保存上一级目录
    strcat(file_path_info.path_now, "/");
    strcat(file_path_info.path_now, file_name);
    if (stat(file_path_info.path_now, &st) == 0){ // 如果成功获取到状态信息
        if (S_ISDIR(st.st_mode)){ // 如果是目录
            lv_obj_clean(sdcard_file_list); // 清除当前wifi列表
            esp_err_t ret = list_sdcard_files(file_path_info.path_now);
            if (ret == ESP_OK){ // 如果成功列出了目录
                file_path_info.path_index++; // 表示进入到下一集目录
                ESP_LOGI(TAG, "path_index: %d", file_path_info.path_index);
                ESP_LOGI(TAG, "path_now: %s", file_path_info.path_now);
                ESP_LOGI(TAG, "path_back: %s", file_path_info.path_back);
                
                // 创建返回按钮（因为现在是非根目录了）
                lvgl_port_lock(0);
                // 清空标题区域，准备重新创建返回按钮和标题
                lv_obj_clean(sdcard_title);
                
                // 创建返回按钮
                lv_obj_t *btn_back = lv_btn_create(sdcard_title);
                lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
                lv_obj_set_size(btn_back, 60, 30);
                lv_obj_set_style_border_width(btn_back, 0, 0); // 设置边框宽度
                lv_obj_set_style_pad_all(btn_back, 0, 0);  // 设置间隙
                lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, LV_PART_MAIN); // 背景透明
                lv_obj_set_style_shadow_opa(btn_back, LV_OPA_TRANSP, LV_PART_MAIN); // 阴影透明
                lv_obj_add_event_cb(btn_back, btn_sdback_cb, LV_EVENT_CLICKED, NULL); // 添加按键处理函数
                lv_obj_set_ext_click_area(btn_back, 20); // 扩大触摸区域20像素
                
                // 创建返回按钮的文本标签
                lv_obj_t *label_back = lv_label_create(btn_back);
                lv_label_set_text(label_back, LV_SYMBOL_LEFT);  // 按键上显示左箭头符号
                lv_obj_set_style_text_font(label_back, &lv_font_montserrat_24, 0);
                lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), 0); 
                lv_obj_align(label_back, LV_ALIGN_CENTER, -10, 0);
                
                // 重新创建标题标签
                sdcard_label = lv_label_create(sdcard_title);
                if (sdmmc_card != NULL) {
                    lv_label_set_text_fmt(sdcard_label, "SD: %lluGB",
                        (((uint64_t)sdmmc_card->csd.capacity) * sdmmc_card->csd.sector_size) >> 30);
                } else {
                    lv_label_set_text(sdcard_label, "SD卡");
                }
                lv_obj_set_style_text_color(sdcard_label, lv_color_hex(0xffffff), 0); 
                lv_obj_set_style_text_font(sdcard_label, &lv_font_montserrat_24, 0);
                lv_obj_align(sdcard_label, LV_ALIGN_CENTER, 0, 0);
                
                lvgl_port_unlock();
            }
            return;
        }
    }
    // 如果没有成功进入目录
    strcpy(file_path_info.path_now, file_path_info.path_back); // 没有列出新的列表 还原当前路径
    // 还原再向下退一级的目录路径
    char *slash = strrchr(file_path_info.path_back, '/'); // 从后往前查找字符'/'
    if (slash!= NULL) { // 如果查找到
        *slash = '\0'; // 替换为NULL 表示字符串结束
    }
    ESP_LOGI(TAG, "path_index: %d", file_path_info.path_index);
    ESP_LOGI(TAG, "path_now: %s", file_path_info.path_now);
    ESP_LOGI(TAG, "path_back: %s", file_path_info.path_back);
}

// SD卡处理任务
static void task_process_sdcard(void *arg)
{
    esp_err_t ret;

    ret = bsp_sdcard_mount(); // 挂载SD卡
    if(ret != ESP_OK){ // 如果没有挂载成功
        ESP_LOGE(TAG, "Failed to mount filesystem.");
        lvgl_port_lock(0);
        lv_label_set_text(sdcard_label, "SD卡挂载不成功");
        lvgl_port_unlock();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 给上面一点显示的时间
        
        // 挂载失败，设置UI状态为MENU，让main_page.c中的逻辑来处理界面切换
        current_ui_state = UI_STATE_MENU;
        
        // 删除SD卡界面元素
        lvgl_port_lock(0);
        lv_obj_del(icon_in_obj);
        sdcard_screen = NULL; // 重置SD卡屏幕对象
        lvgl_port_unlock();
    }else{ // 如果挂载成功
        // 终端显示SD卡信息
        sdmmc_card_print_info(stdout, sdmmc_card);
        // 液晶屏标题栏显示SD卡容量
        lvgl_port_lock(0);
        lv_label_set_text_fmt(sdcard_label, "SD: %lluGB",
            (((uint64_t)sdmmc_card->csd.capacity) * sdmmc_card->csd.sector_size) >> 30);
        lvgl_port_unlock();

        // 只在非根目录下显示返回按钮
        if (file_path_info.path_index > 0) {
            lvgl_port_lock(0);
            lv_obj_t *btn_back = lv_btn_create(sdcard_title);
            lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_size(btn_back, 60, 30);
            lv_obj_set_style_border_width(btn_back, 0, 0); // 设置边框宽度
            lv_obj_set_style_pad_all(btn_back, 0, 0);  // 设置间隙
            lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, LV_PART_MAIN); // 背景透明
            lv_obj_set_style_shadow_opa(btn_back, LV_OPA_TRANSP, LV_PART_MAIN); // 阴影透明
            lv_obj_add_event_cb(btn_back, btn_sdback_cb, LV_EVENT_CLICKED, NULL); // 添加按键处理函数
            lv_obj_set_ext_click_area(btn_back, 20); // 扩大触摸区域20像素

            lv_obj_t *label_back = lv_label_create(btn_back); 
            lv_label_set_text(label_back, LV_SYMBOL_LEFT);  // 按键上显示左箭头符号
            lv_obj_set_style_text_font(label_back, &lv_font_montserrat_24, 0);
            lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), 0); 
            lv_obj_align(label_back, LV_ALIGN_CENTER, -10, 0);
            lvgl_port_unlock();
        }

        // 创建文件列表
        sdcard_file_list = lv_list_create(icon_in_obj);
        lv_obj_set_size(sdcard_file_list, 320, 200);
        lv_obj_align(sdcard_file_list, LV_ALIGN_TOP_LEFT, 0, 40);
        lv_obj_set_style_border_width(sdcard_file_list, 0, 0);
        lv_obj_set_style_text_font(sdcard_file_list, &tiktok20, 0);
        lv_obj_set_scrollbar_mode(sdcard_file_list, LV_SCROLLBAR_MODE_OFF); // 隐藏wifi_list滚动条
        lvgl_port_unlock();
        // 列出 SD 卡中的文件
        file_path_info.path_index = 0; // 表示当前在根目录
        strcpy(file_path_info.path_now, SD_MOUNT_POINT); // 装入当前路径
        list_sdcard_files(file_path_info.path_now); // 列出当前目录文件
    }
    
    vTaskDelete(NULL);
}


// 进入SD卡应用程序
// action2函数 - 与菜单对接的入口函数
void action2(void) 
{ 
    printf("Action action2 被执行\n");
    
    // 设置UI状态为SD卡状态
    current_ui_state = UI_STATE_SDCARD;
    
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

    // 创建sdcard_screen对象
    if (lvgl_port_lock(100)) {
        sdcard_screen = lv_obj_create(NULL);
        if (sdcard_screen != NULL) {
            lv_scr_load(sdcard_screen);
        }
        lvgl_port_unlock();
    }
    
    icon_in_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(icon_in_obj, &style, 0);
    
    // 创建标题背景
    sdcard_title = lv_obj_create(icon_in_obj);
    lv_obj_set_size(sdcard_title, 320, 40);
    lv_obj_set_style_pad_all(sdcard_title, 0, 0);  // 设置间隙
    lv_obj_align(sdcard_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(sdcard_title, lv_color_hex(0x008b8b), 0);
    
    // 显示标题
    sdcard_label = lv_label_create(sdcard_title);
    lv_label_set_text(sdcard_label, "TF卡扫描中...");
    lv_obj_set_style_text_color(sdcard_label, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_text_font(sdcard_label, &lv_font_montserrat_24, 0);
    lv_obj_align(sdcard_label, LV_ALIGN_CENTER, 0, 0);
    
    // 创建并启动SD卡处理任务
    xTaskCreatePinnedToCore(task_process_sdcard, "task_process_sdcard", 3 * 1024, NULL, 5, NULL, 1);
}



