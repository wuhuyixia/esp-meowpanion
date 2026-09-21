/**
 * @file ota_web.c
 * @brief 基于 ESP-IDF HTTP Server 的浏览器 OTA 入口。
 *
 * 浏览器端通过 XMLHttpRequest 直接把用户选择的 bin 文件作为 HTTP
 * 请求体发送，不使用 multipart/form-data。这样设备端收到的就是连续
 * 的固件字节流，可以边接收边写入 Flash，不需要在 RAM 或 SPIFFS 中
 * 再保存一份完整镜像。
 */

#include "ota_web.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ota_app.h"

static const char *TAG = "OTA_WEB";

/* HTTPD 句柄只在本文件中使用；非 NULL 表示服务器已经启动。 */
static httpd_handle_t s_server;

/*
 * 内嵌的升级页面。页面逻辑分成两条进度来源：
 *
 * 1. XMLHttpRequest.upload.onprogress：反映浏览器把数据发到设备的网络
 *    进度，反馈更及时；
 * 2. /ota-status 轮询：反映设备实际写入 Flash 和校验的进度，网络上传
 *    完成后仍能显示 VERIFYING、DONE 或 ERROR。
 *
 * 页面中的“仅限同一局域网使用”是使用场景提示，不是安全认证。当前
 * 服务器没有登录、签名校验或 HTTPS，不能直接暴露到公网。
 */
static const char s_index_html[] =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32-S3 OTA</title>"
    "<style>body{font-family:Arial;max-width:620px;margin:auto;padding:20px}"
    "button{padding:10px 16px;margin-top:10px}progress{width:100%;height:22px}"
    ".warn{color:#a50}.ok{color:#087f23}.err{color:#b00}"
    "#pct{font-weight:bold;margin-left:8px}</style></head><body>"
    "<h2>ESP32-S3 Web OTA</h2>"
    "<p>仅限同一局域网使用。请选择当前工程生成的应用固件 bin 文件。</p>"
    "<input id='fw' type='file' accept='.bin'><br>"
    "<button id='go' onclick='upload()'>上传并升级</button>"
    "<p id='msg'>状态：IDLE</p>"
    "<progress id='bar' value='0' max='100'></progress><span id='pct'>0%</span>"
    "<script>"
    "const m=document.getElementById('msg'),b=document.getElementById('bar'),"
    "g=document.getElementById('go'),p=document.getElementById('pct');"
    "function setProgress(v){v=Math.max(0,Math.min(100,Math.round(v)));b.value=v;p.textContent=v+'%';}"
    "async function poll(){try{let r=await fetch('/ota-status',{cache:'no-store'});let s=await r.json();"
    "if(s.state!=='RECEIVING'){setProgress(s.percent);}"
    "m.textContent='状态：'+s.state+'，进度：'+s.percent+'%'+(s.error?'，错误：'+s.error:'');"
    "}catch(e){m.textContent='状态读取失败';}}"
    "function upload(){let f=document.getElementById('fw').files[0];"
    "if(!f){m.textContent='请先选择 bin 文件';return;}"
    "g.disabled=true;setProgress(0);m.className='';m.textContent='正在上传固件，请勿断电';"
    "let x=new XMLHttpRequest();x.open('POST','/ota-upload',true);"
    "x.upload.onprogress=function(e){if(e.lengthComputable){let v=e.loaded*100/e.total;setProgress(v);"
    "m.textContent='正在上传固件：'+Math.round(v)+'% ('+e.loaded+'/'+e.total+' bytes)';}};"
    "x.upload.onload=function(){setProgress(100);m.textContent='固件已上传，正在校验并写入启动分区...';};"
    "x.onload=function(){g.disabled=false;if(x.status>=200&&x.status<300){setProgress(100);"
    "m.className='ok';m.textContent=x.responseText||'OTA success, rebooting...';}else{"
    "m.className='err';m.textContent='OTA失败：'+x.status+' '+x.responseText;poll();}};"
    "x.onerror=function(){g.disabled=false;m.className='err';m.textContent='上传失败：网络连接中断';poll();};"
    "x.ontimeout=function(){g.disabled=false;m.className='err';m.textContent='上传失败：请求超时';poll();};"
    "x.send(f);}poll();"
    "</script></body></html>";

static const char *state_name(ota_state_t state)
{
    /*
     * JSON 接口使用稳定的英文状态名，方便网页 JavaScript 或其他上位机
     * 程序解析；中文解释放在网页显示层，不把本地化文本写死在协议里。
     */
    switch (state) {
    case OTA_STATE_RECEIVING:
        return "RECEIVING";
    case OTA_STATE_VERIFYING:
        return "VERIFYING";
    case OTA_STATE_DONE:
        return "DONE";
    case OTA_STATE_ERROR:
        return "ERROR";
    case OTA_STATE_IDLE:
    default:
        return "IDLE";
    }
}

static esp_err_t handle_index(httpd_req_t *request)
{
    /* 浏览器访问设备 IP 根路径时返回升级页面。 */
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, s_index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_status(httpd_req_t *request)
{
    /*
     * 读取的是 ota_app 返回的线程安全快照。HTTP 处理器不直接访问 OTA
     * 内部变量，因此不会和 Flash 写入过程产生数据竞争。
     */
    ota_status_t status = ota_get_status();
    char response[256];

    /*
     * 返回的字段含义：
     *   state    当前状态字符串；
     *   percent  0~100 的进度；
     *   received 已写入字节数；
     *   total    本次上传总字节数；
     *   error    失败原因，没有错误时为空字符串。
     *
     * error 只由本模块生成的固定错误信息写入，长度也在 ota_app 中被
     * 限制为 96 字节，因此这里可以放入固定大小的响应缓冲区。
     */
    snprintf(response, sizeof(response),
             "{\"state\":\"%s\",\"percent\":%d,\"received\":%lu,\"total\":%lu,\"error\":\"%s\"}",
             state_name(status.state),
             status.percent,
             (unsigned long)status.received,
             (unsigned long)status.total,
             status.error);
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_upload(httpd_req_t *request)
{
    /*
     * 由于浏览器直接发送 bin 文件，content_len 就是镜像大小。空请求
     * 没有任何可写内容，直接返回 400，避免创建无意义的 OTA 会话。
     */
    if (request->content_len == 0) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "empty firmware");
    }

    ota_writer_t writer;

    /*
     * begin 内部会：抢占唯一 OTA 锁、选择非当前分区、检查容量并调用
     * esp_ota_begin()。失败时不能进入接收循环。
     */
    esp_err_t err = ota_writer_begin(&writer, request->content_len);
    if (err != ESP_OK) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
    }

    /*
     * 使用 2 KiB 小缓冲分块接收。固件完整大小可能是数 MB，不能一次
     * malloc(request->content_len)；同时工程中的 LVGL、音频和摄像头任务
     * 已经占用了一部分 RAM，小缓冲能降低 OTA 对业务运行的影响。
     */
    uint8_t *buffer = malloc(2048);
    if (!buffer) {
        ota_writer_abort(&writer, "out of memory");
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
    }

    /*
     * remaining 记录 HTTP 请求体尚未读取的字节数。httpd_req_recv() 允许
     * 返回小于 requested 的部分数据，所以每次都按实际 received 扣减，
     * 不能假设一次 recv 就拿到完整请求块。
     */
    size_t remaining = request->content_len;
    while (remaining > 0) {
        size_t requested = remaining > 2048 ? 2048 : remaining;
        int received = httpd_req_recv(request, (char *)buffer, requested);
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            /*
             * HTTPD 的读超时不一定代表连接断开；重试即可。remaining
             * 不变，只有拿到实际数据后才继续推进。
             */
            continue;
        }
        if (received <= 0) {
            /* 0/负数表示客户端断开或底层 socket 出错。 */
            free(buffer);
            ota_writer_abort(&writer, "upload interrupted");
            return ESP_FAIL;
        }

        /*
         * 先写 Flash，成功后 ota_app 才会增加 received 并更新网页状态。
         * 如果写入失败，必须 abort，释放 OTA handle 和全局写入锁。
         */
        err = ota_writer_write(&writer, buffer, (size_t)received);
        if (err != ESP_OK) {
            free(buffer);
            ota_writer_abort(&writer, "flash write failed");
            return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "flash write failed");
        }
        remaining -= (size_t)received;
    }

    /* 所有 HTTP 数据已读完，缓冲区不再需要。 */
    free(buffer);

    /*
     * finish 会进入 VERIFYING，调用 esp_ota_end() 校验镜像，并把目标
     * 分区写入 otadata 作为下一次启动分区。只有它成功后才允许重启。
     */
    err = ota_writer_finish(&writer);
    if (err != ESP_OK) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "invalid firmware image");
    }

    /*
     * 先给浏览器发送成功响应，再延时重启。若先 esp_restart()，TCP 连接
     * 可能来不及把成功消息发出去，用户会误以为升级失败。
     */
    httpd_resp_sendstr(request, "OTA success, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(800));
    esp_restart();
    return ESP_OK;
}

esp_err_t ota_web_start(void)
{
    /* app_main 只应启动一次；重复调用直接复用现有服务器。 */
    if (s_server) {
        return ESP_OK;
    }

    /*
     * HTTPD 默认配置基础上做几项适配：
     *   stack_size         OTA 上传循环需要一定栈空间；
     *   max_uri_handlers   正好注册根页面、状态和上传三个接口；
     *   recv/send timeout   避免网络短暂停顿时 HTTPD 永久阻塞；
     *   lru_purge_enable   连接资源紧张时允许清理最久未使用的连接。
     */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 6144;
    config.max_uri_handlers = 3;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
        return err;
    }

    /*
     * 三个路由组成完整的 Web OTA 协议：页面负责交互，status 负责查询，
     * upload 负责发送固件。上传接口接收的是原始 application/octet-stream
     * 风格的数据流，当前实现不解析表单字段。
     */
    const httpd_uri_t routes[] = {
        {
            .uri = "/",
            .method = HTTP_GET,
            .handler = handle_index,
            .user_ctx = NULL,
        },
        {
            .uri = "/ota-status",
            .method = HTTP_GET,
            .handler = handle_status,
            .user_ctx = NULL,
        },
        {
            .uri = "/ota-upload",
            .method = HTTP_POST,
            .handler = handle_upload,
            .user_ctx = NULL,
        },
    };

    /*
     * 逐个注册路由。中途失败时停止整个 HTTP 服务器，避免只启动了部分
     * OTA 接口却仍然向上层报告成功。
     */
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        err = httpd_register_uri_handler(s_server, &routes[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP route registration failed: %s", esp_err_to_name(err));
            httpd_stop(s_server);
            s_server = NULL;
            return err;
        }
    }

    ESP_LOGI(TAG, "Web OTA server started on port 80");
    return ESP_OK;
}
