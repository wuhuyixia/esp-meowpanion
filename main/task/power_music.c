/*
 * @file power_music.c
 * @brief 开机音乐播放功能实现
 * @details 本文件实现了ESP32-S3系统的开机音乐播放功能，主要包括：
 *          1. 初始化音频解码器和播放器
 *          2. 播放内置的开机音效文件
 *          3. 控制音频播放流程和资源释放
 *          4. 实现系统启动时的音效反馈
 */
#include "headfile.h"

// 开机音乐 任务函数
static const char *TAG = "power_music";
extern EventGroupHandle_t my_event_group;

extern const uint8_t music_pcm_start[] asm("_binary_sword_pcm_start");
extern const uint8_t music_pcm_end[] asm("_binary_sword_pcm_end");

static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"};

// 添加全局互斥锁用于音乐播放保护
extern SemaphoreHandle_t audio_mutex;

/**
 * @brief 开机音乐播放任务函数
 * @details 实现ESP32-S3系统开机音乐播放的主要逻辑，包括：
 *          1. 获取音频系统互斥锁保护资源
 *          2. 预加载PCM音频数据并配置I2S输出通道
 *          3. 控制音频功率放大器并输出音频数据
 *          4. 处理播放结果，释放资源并设置完成标志位
 * @param pvParameters 任务参数（未使用）
 * @return 无返回值
 */
void power_music_task(void *pvParameters)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;

    // 尝试获取音频互斥锁，确保一次只有一个任务访问音频资源
    if (xSemaphoreTake(audio_mutex, portMAX_DELAY) == pdTRUE)
    {
        /* (Optional) Disable TX channel and preload the data before enabling the TX channel,
         * so that the valid data can be transmitted immediately */
        ESP_ERROR_CHECK(i2s_channel_disable(i2s_tx_chan));
        ESP_ERROR_CHECK(i2s_channel_preload_data(i2s_tx_chan, data_ptr, music_pcm_end - data_ptr, &bytes_write));
        data_ptr += bytes_write; // Move forward the data pointer

        /* Enable the TX channel */
        ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));

        pa_en(1); // 打开音频输出，启动功率放大器

        /* Write music to earphone */
        ret = i2s_channel_write(i2s_tx_chan, data_ptr, music_pcm_end - data_ptr, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            /* Since we set timeout to 'portMAX_DELAY' in 'i2s_channel_write'
                so you won't reach here unless you set other timeout value,
                if timeout detected, it means write operation failed. */
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            pa_en(0);                    // 关闭音频输出
            xSemaphoreGive(audio_mutex); // 释放互斥锁
            abort();
        }
        if (bytes_write > 0)
        {
            ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
        }
        else
        {
            ESP_LOGE(TAG, "[music] i2s music play failed.");
            pa_en(0);                    // 关闭音频输出
            xSemaphoreGive(audio_mutex); // 释放互斥锁
            abort();
        }

        pa_en(0);                    // 关闭音频输出，关闭功率放大器
        xSemaphoreGive(audio_mutex); // 释放互斥锁，允许其他任务访问音频资源
        xEventGroupSetBits(my_event_group, START_MUSIC_COMPLETED); // 设置事件标志，通知系统开机音乐播放完成
    }

    vTaskDelete(NULL); // 播放任务完成，删除自身
}
