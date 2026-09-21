/**
 * @file ota_app.c
 * @brief OTA Flash 写入、镜像校验和启动分区切换的实现。
 *
 * 这里使用 ESP-IDF 的 app_update 组件完成真正的 Flash 操作：
 *
 *   esp_ota_get_next_update_partition()
 *       选择当前运行分区之外的目标槽位；
 *   esp_ota_begin() / esp_ota_write()
 *       创建 OTA 会话并以数据块形式写入目标槽位；
 *   esp_ota_end()
 *       结束写入并校验镜像完整性；
 *   esp_ota_set_boot_partition()
 *       将目标槽位记录为下一次启动分区。
 *
 * 网络接收和 HTTP 响应在 ota_web.c 中完成，本文件只负责“如何安全地
 * 写入和提交固件”。这样可以避免把 Flash 写入细节散落在 HTTP 处理器中。
 */

#include "ota_app.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "OTA";

/*
 * s_writer_lock：保证同一时刻只有一个上传请求可以写 OTA 分区。
 * 如果允许两个 HTTP 请求同时调用 esp_ota_write()，两个数据流会互相
 * 覆盖，最终得到不可启动的镜像，因此这里采用非阻塞抢锁策略：
 * 第二个请求立即失败，不等待第一个请求结束。
 */
static SemaphoreHandle_t s_writer_lock;

/*
 * s_status_lock：保护 s_status 的一致性。HTTP 状态查询任务可能与 OTA
 * 写入任务并发运行，读取状态时必须拿锁，避免读到一半更新的数据。
 */
static SemaphoreHandle_t s_status_lock;

/*
 * 所有对外状态都从这份全局快照读取。访问它必须通过 set_status() 或
 * ota_get_status()，不能在其他文件中直接引用这个静态变量。
 */
static ota_status_t s_status = {
    .state = OTA_STATE_IDLE,
};

static void set_status(ota_state_t state, size_t total, size_t received, const char *error)
{
    /*
     * ota_app_init() 失败时锁可能尚未创建。此时不能调用 FreeRTOS 的
     * xSemaphoreTake(NULL, ...)，直接返回可以避免二次崩溃。
     */
    if (!s_status_lock) {
        return;
    }

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_status.state = state;
    s_status.total = total;
    s_status.received = received;

    /*
     * 使用整数百分比供网页直接显示。total 为 0 时无法计算进度，
     * 约定返回 0；最后再做一次上限保护，防止异常输入显示超过 100%。
     */
    s_status.percent = total > 0 ? (int)((received * 100U) / total) : 0;
    if (s_status.percent > 100) {
        s_status.percent = 100;
    }

    /*
     * error 是固定长度数组，必须使用 snprintf 截断过长信息，避免把
     * ESP-IDF 的错误字符串复制到数组外。非错误状态清空旧错误。
     */
    if (error) {
        snprintf(s_status.error, sizeof(s_status.error), "%s", error);
    } else {
        s_status.error[0] = '\0';
    }
    xSemaphoreGive(s_status_lock);
}

static void set_error(const char *reason)
{
    char message[96];
    snprintf(message, sizeof(message), "%s", reason ? reason : "unknown OTA error");

    /*
     * 保留失败前的 total/received，网页可以显示“失败发生在多少字节处”，
     * 而不是因为状态变成 ERROR 就丢失进度信息。
     */
    ota_status_t old_status = ota_get_status();
    set_status(OTA_STATE_ERROR, old_status.total, old_status.received, message);
    ESP_LOGE(TAG, "%s", message);
}

static void release_writer(ota_writer_t *writer)
{
    /*
     * active 同时表示“本次会话仍有效”和“writer 持有全局锁”。
     * 只在 active 为 true 时释放，避免 finish/abort 被重复调用时多次
     * xSemaphoreGive() 破坏互斥锁状态。
     */
    if (writer && writer->active) {
        writer->active = false;
        xSemaphoreGive(s_writer_lock);
    }
}

esp_err_t ota_app_init(void)
{
    /*
     * app_main() 目前只调用一次，但做成幂等函数后，其他启动流程或
     * 测试代码重复调用也不会重复创建锁、覆盖正在使用的状态。
     */
    if (s_writer_lock && s_status_lock) {
        return ESP_OK;
    }

    /*
     * 两把锁分别保护“写入会话唯一性”和“状态快照一致性”。互斥锁
     * 创建失败时要清理已经创建成功的另一把锁，避免留下半初始化状态。
     */
    s_writer_lock = xSemaphoreCreateMutex();
    s_status_lock = xSemaphoreCreateMutex();
    if (!s_writer_lock || !s_status_lock) {
        if (s_writer_lock) {
            vSemaphoreDelete(s_writer_lock);
            s_writer_lock = NULL;
        }
        if (s_status_lock) {
            vSemaphoreDelete(s_status_lock);
            s_status_lock = NULL;
        }
        return ESP_ERR_NO_MEM;
    }

    /* 初始化完成后，网页首次访问 /ota-status 会得到干净的 IDLE 状态。 */
    set_status(OTA_STATE_IDLE, 0, 0, NULL);
    ESP_LOGI(TAG, "OTA writer initialized");
    return ESP_OK;
}

ota_status_t ota_get_status(void)
{
    /*
     * 返回副本而不是返回内部指针。即使调用者在锁释放后长时间处理
     * 这个结果，也不会阻塞 OTA 写入线程或破坏全局状态。
     */
    ota_status_t snapshot = {
        .state = OTA_STATE_IDLE,
    };

    if (!s_status_lock) {
        return snapshot;
    }

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    snapshot = s_status;
    xSemaphoreGive(s_status_lock);
    return snapshot;
}

bool ota_is_active(void)
{
    ota_status_t status = ota_get_status();
    return status.state == OTA_STATE_RECEIVING || status.state == OTA_STATE_VERIFYING;
}

void ota_mark_valid(void)
{
    /*
     * 当 Bootloader 开启应用回滚功能时，新镜像第一次启动后需要由应用
     * 主动确认。这里放在核心硬件、UI 和任务已经启动之后调用，意味着
     * 新固件至少完成了基本自检；但不等待 Wi-Fi，避免网络暂时不可用
     * 导致本来健康的新固件被误回滚。
     */
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Current firmware marked valid");
    } else if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "Current firmware did not require rollback confirmation");
    } else {
        ESP_LOGW(TAG, "Could not mark firmware valid: %s", esp_err_to_name(err));
    }
}

esp_err_t ota_writer_begin(ota_writer_t *writer, size_t total)
{
    /*
     * writer 和两个锁都是必要前提。尤其不能在 OTA 模块未初始化时
     * 继续调用 ESP-IDF OTA API，否则错误处理也无法更新状态。
     */
    if (!writer || !s_writer_lock || !s_status_lock) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 清除调用者可能复用的上下文，避免继承上一次会话的句柄和计数。 */
    memset(writer, 0, sizeof(*writer));

    /*
     * 传入 0 表示不等待锁。OTA 是长时间操作，第二个浏览器请求如果
     * 阻塞在这里，会占用 HTTPD 工作线程并且覆盖/延迟第一条请求的反馈。
     */
    if (xSemaphoreTake(s_writer_lock, 0) != pdTRUE) {
        // Do not overwrite the first upload's status when a second browser
        // request arrives while the writer is already active.
        ESP_LOGW(TAG, "Another OTA is already running");
        return ESP_ERR_INVALID_STATE;
    }

    writer->total = total;

    /*
     * ESP-IDF 会根据当前启动分区和 otadata 记录选择下一个 OTA 槽位。
     * 正常情况下：当前运行 ota_0 就返回 ota_1，当前运行 ota_1 就返回
     * ota_0。这样写入时不会覆盖正在执行的应用。
     */
    writer->partition = esp_ota_get_next_update_partition(NULL);
    if (!writer->partition) {
        set_error("No OTA partition available");
        xSemaphoreGive(s_writer_lock);
        return ESP_ERR_NOT_FOUND;
    }

    /*
     * 先按分区容量做快速检查。它不能代替后续镜像校验，但可以在上传
     * 前就拒绝明显过大的文件，避免不必要的 Flash 擦写。
     */
    if (total > writer->partition->size) {
        set_error("Firmware is larger than the OTA slot");
        xSemaphoreGive(s_writer_lock);
        return ESP_ERR_INVALID_SIZE;
    }

    /*
     * esp_ota_begin() 会准备目标分区并返回本次写入的句柄。已知文件大小
     * 时传入 total，大小未知时传入 OTA_SIZE_UNKNOWN；当前 Web 层因为
     * HTTP 请求有 Content-Length，所以通常走已知大小分支。
     */
    esp_err_t err = esp_ota_begin(
        writer->partition,
        total > 0 ? total : OTA_SIZE_UNKNOWN,
        &writer->handle);
    if (err != ESP_OK) {
        char message[96];
        snprintf(message, sizeof(message), "OTA begin failed: %s", esp_err_to_name(err));
        set_error(message);
        xSemaphoreGive(s_writer_lock);
        return err;
    }

    /* 从这里开始，abort/finish 都必须负责释放 writer 锁。 */
    writer->active = true;
    set_status(OTA_STATE_RECEIVING, total, 0, NULL);
    ESP_LOGI(TAG, "Writing %s at 0x%lx, slot size=%lu, image size=%lu",
             writer->partition->label,
             (unsigned long)writer->partition->address,
             (unsigned long)writer->partition->size,
             (unsigned long)total);
    return ESP_OK;
}

esp_err_t ota_writer_write(ota_writer_t *writer, const void *data, size_t length)
{
    /*
     * 不接受空数据、不接受未 begin 的 writer。这样可以把调用顺序错误
     * 尽早暴露出来，避免向无效 OTA handle 写入。
     */
    if (!writer || !writer->active || !data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * 用“剩余空间”表达式检查长度，避免 received + length 发生 size_t
     * 整数溢出。HTTP 层虽然按 content_len 接收，但应用层仍要独立防守。
     */
    if (writer->total > 0 && length > writer->total - writer->received) {
        return ESP_ERR_INVALID_SIZE;
    }

    /*
     * esp_ota_write() 负责把这一块数据写入目标分区。它可能在内部按
     * Flash 擦写粒度处理数据，调用方无需自行对齐或操作分区地址。
     */
    esp_err_t err = esp_ota_write(writer->handle, data, length);
    if (err != ESP_OK) {
        return err;
    }

    /* 只有 Flash 写成功后才增加计数，进度代表“已落盘”的字节数。 */
    writer->received += length;
    set_status(OTA_STATE_RECEIVING, writer->total, writer->received, NULL);
    return ESP_OK;
}

esp_err_t ota_writer_finish(ota_writer_t *writer)
{
    /* finish 只能用于仍处于 active 状态的会话。 */
    if (!writer || !writer->active) {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * HTTP Content-Length 是本次上传应有的总长度。少收一块数据时，不能
     * 把“部分镜像”交给 Bootloader；先拒绝并释放会话。
     */
    if (writer->total > 0 && writer->received != writer->total) {
        set_error("Upload ended before the declared image size");
        release_writer(writer);
        return ESP_ERR_INVALID_SIZE;
    }

    /*
     * 所有字节已经写入，接下来不再是网络接收阶段，而是镜像提交阶段。
     * 网页轮询可据此显示 VERIFYING，而不是停留在 RECEIVING。
     */
    set_status(OTA_STATE_VERIFYING, writer->total, writer->received, NULL);

    /*
     * esp_ota_end() 会结束 OTA handle，并校验镜像头、镜像长度及摘要等
     * ESP-IDF 支持的完整性信息。校验失败时不能切换启动分区。
     */
    esp_err_t err = esp_ota_end(writer->handle);
    if (err != ESP_OK) {
        char message[96];
        snprintf(message, sizeof(message), "Firmware validation failed: %s", esp_err_to_name(err));
        set_error(message);
        release_writer(writer);
        return err;
    }

    /*
     * 只有镜像校验成功后才更新 otadata。该调用不会立即切换正在运行的
     * CPU，而是让下一次复位/重启从新分区启动。
     */
    err = esp_ota_set_boot_partition(writer->partition);
    if (err != ESP_OK) {
        char message[96];
        snprintf(message, sizeof(message), "Set boot partition failed: %s", esp_err_to_name(err));
        set_error(message);
        release_writer(writer);
        return err;
    }

    /* 提交成功，通知 Web 层，然后由 HTTP 层发送响应并调用 esp_restart。 */
    set_status(OTA_STATE_DONE, writer->total, writer->received, NULL);
    ESP_LOGI(TAG, "OTA image accepted, bytes=%lu; rebooting", (unsigned long)writer->received);
    release_writer(writer);
    return ESP_OK;
}

void ota_writer_abort(ota_writer_t *writer, const char *reason)
{
    if (!writer) {
        return;
    }

    if (writer->active) {
        /*
         * 放弃未完成的写入会话。该镜像不会被设置为启动分区，下一次 OTA
         * 仍可重新选择可用槽位；同时必须释放全局锁，否则后续上传都会
         * 被误判为“已有 OTA 正在执行”。
         */
        esp_ota_abort(writer->handle);
        release_writer(writer);
    }

    /* 错误状态保留已接收字节数，便于网页和串口日志定位中断位置。 */
    set_error(reason ? reason : "OTA upload aborted");
}
