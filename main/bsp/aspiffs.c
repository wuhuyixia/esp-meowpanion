#include "headfile.h"
static const char *TAG = "spiffs";


// 添加spiffs初始化
void init_spiffs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}


// 显示PNG图片的函数
void display_png_image(const char *png_filepath)
{
    char filepath[256];
    char checkpath[256];
    
    // 使用驱动器字母格式
    snprintf(filepath, sizeof(filepath), "S:/%s", png_filepath);
    
    // 构建文件系统检查路径
    snprintf(checkpath, sizeof(checkpath), "/spiffs/%s", png_filepath);

    // 检查文件是否存在
    FILE *f = fopen(checkpath, "r");  // 使用实际的文件系统路径检查
    if (f == NULL)
    {
        ESP_LOGE(TAG, "png file not found: %s", checkpath);
        return;
    }
    fclose(f);

    ESP_LOGI(TAG, "png file found: %s", checkpath);

    // 创建图像对象
    lv_obj_t *png_img = lv_img_create(lv_scr_act());

    // 使用驱动器字母格式设置图像源
    lv_img_set_src(png_img, filepath);

    // 检查图片源是否有效
    const void *src = lv_img_get_src(png_img);
    if (src == NULL)
    {
        ESP_LOGE(TAG, "Image source is NULL");
        return;
    }
    ESP_LOGI(TAG, "Image source is valid: %s", (const char*)src);

    // 获取图片信息
    lv_img_header_t header;
    if (lv_img_decoder_get_info(src, &header) == LV_RES_OK)
    {
        ESP_LOGI(TAG, "Image info: width=%d, height=%d, cf=%d",
                 header.w, header.h, header.cf);
    }
   
    // 设置图片大小与屏幕相同，确保填满整个屏幕
    lv_obj_set_size(png_img, LV_PCT(100), LV_PCT(100));

    // 设置位置和大小
    lv_obj_align(png_img, LV_ALIGN_CENTER, 0, 0);

  
    ESP_LOGI(TAG, "png image display requested: %s", filepath);
}
void test(void)
{
    // 首先测试LVGL是否正常工作 - 创建一个简单的彩色矩形
    lv_obj_t *test_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(test_obj, 200, 200);
    lv_obj_set_style_bg_color(test_obj, lv_color_hex(0xFF0000), LV_PART_MAIN); // 红色
    lv_obj_align(test_obj, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGI(TAG, "LVGL test object created");

    // 强制刷新屏幕
    lv_obj_invalidate(lv_scr_act());
    lv_disp_flush_ready(lv_disp_get_default()->driver);

    // 等待2秒让测试对象显示
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // 删除测试对象
    lv_obj_del(test_obj);
    ESP_LOGI(TAG, "Test object deleted");

    // 初始化SPIFFS
    init_spiffs();

    // 初始化LVGL文件系统
    lv_fs_stdio_init();

    // 注意：删除PNG初始化，因为您使用的是BMP文件
     lv_png_init();  

    // 显示BMP图片
    display_png_image("duck.png");  // 使用BMP文件
}