/*
 * @file codec.h
 * @brief 音频编解码器驱动接口
 * @details 本文件提供了音频编解码器的驱动接口，主要功能包括：
 *          1. 音频编解码器初始化
 *          2. I2S音频数据传输
 *          3. 音频采样率和格式设置
 *          4. 音量控制和静音功能
 *          5. 音频数据获取
 */
#pragma once
#include "headfile.h"


extern i2s_chan_handle_t i2s_tx_chan;
extern i2s_chan_handle_t i2s_rx_chan;


esp_err_t bsp_codec_init(void);
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t bsp_speaker_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t bsp_codec_mute_set(bool enable);
esp_err_t bsp_codec_volume_set(int volume, int *volume_set);

int bsp_get_feed_channel(void);
esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len);


void power_music_task(void *pvParameters);
