#pragma once

/*
 * OTA Web 层提供一个局域网内的简单浏览器升级页面：
 *
 *   GET  /             返回选择 bin 文件和显示进度的 HTML 页面；
 *   GET  /ota-status   返回当前 OTA 状态 JSON；
 *   POST /ota-upload   接收原始 bin 文件并交给 ota_app 写入 Flash。
 *
 * 本层只负责 HTTP 协议、缓存和重启时序，真正的分区选择、写入和镜像
 * 校验由 ota_app.c 统一完成。
 */

#include "esp_err.h"

/**
 * @brief 启动 OTA HTTP 服务器并注册三个 OTA 路由。
 *
 * 函数具有幂等性：服务器已经启动时直接返回 ESP_OK。调用前应确保
 * Wi-Fi/网络接口已经初始化；HTTP 服务器本身不会负责连接 Wi-Fi。
 */
esp_err_t ota_web_start(void);
