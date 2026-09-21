#pragma once

/*
 * OTA 应用层的职责：
 *
 * 1. 初始化 OTA 写入器和状态管理所需的互斥锁；
 * 2. 选择“当前运行分区之外”的下一个 OTA 应用分区；
 * 3. 把网络层收到的固件数据分块写入 Flash；
 * 4. 在写入结束后让 ESP-IDF 校验镜像，并切换下次启动分区；
 * 5. 为 Web 层提供可查询的进度、状态和错误信息。
 *
 * 这个模块不关心数据来自 HTTP、HTTPS、串口还是其他传输方式。
 * 传输层只需要按 begin -> write(可多次) -> finish 的顺序调用接口，
 * 就可以复用同一套 OTA 写入逻辑。
 */

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_ota_ops.h"

/**
 * @brief OTA 写入过程的状态机。
 *
 * 状态的生命周期通常为：
 *
 *     IDLE -> RECEIVING -> VERIFYING -> DONE
 *                         \-> ERROR
 *
 * OTA 成功后设备会立即重启，所以 DONE 只会在重启前短暂存在；
 * 如果网络中断、Flash 写入失败、镜像校验失败或分区不可用，
 * 则进入 ERROR，当前固件仍然可以继续运行。
 */
typedef enum {
    OTA_STATE_IDLE = 0,       /**< 没有正在进行的升级。 */
    OTA_STATE_RECEIVING,      /**< 正在接收并写入固件数据。 */
    OTA_STATE_VERIFYING,      /**< 数据写完，正在结束 OTA 并校验镜像。 */
    OTA_STATE_DONE,           /**< 校验成功且已设置新的启动分区。 */
    OTA_STATE_ERROR,          /**< 本次升级失败，error 中保存原因。 */
} ota_state_t;

/**
 * @brief 对外暴露的 OTA 状态快照。
 *
 * ota_get_status() 返回的是结构体副本，而不是内部全局变量本身，
 * 因此 Web 任务读取状态时不会直接修改 OTA 模块的共享数据。
 */
typedef struct {
    ota_state_t state;        /**< 当前状态。 */
    size_t total;             /**< 期望接收的固件总字节数，来自 HTTP Content-Length。 */
    size_t received;          /**< 已经成功写入 OTA 分区的字节数。 */
    int percent;              /**< 根据 received/total 计算的 0~100 整数进度。 */
    char error[96];           /**< 最近一次失败原因；没有错误时为空字符串。 */
} ota_status_t;

/**
 * @brief 一次 OTA 写入会话的上下文。
 *
 * 一个 writer 对应一次固件上传请求。HTTP 层负责接收网络数据，
 * 本结构体负责记录 ESP-IDF OTA handle、目标分区和进度。
 */
typedef struct {
    esp_ota_handle_t handle;          /**< esp_ota_begin() 返回的 OTA 会话句柄。 */
    const esp_partition_t *partition; /**< 实际写入的 ota_0 或 ota_1 分区。 */
    size_t total;                     /**< 本次上传声明的总大小。 */
    size_t received;                  /**< 本次已经写入 Flash 的大小。 */
    bool active;                      /**< 是否仍持有全局 OTA 写入锁。 */
} ota_writer_t;

/**
 * @brief 初始化 OTA 模块。
 *
 * 应在 app_main() 早期调用一次。函数是幂等的，重复调用不会重复创建
 * 互斥锁。调用成功后才可以使用下面的 writer 和状态接口。
 */
esp_err_t ota_app_init(void);

/**
 * @brief 确认当前运行固件可以正常工作，取消启动回滚。
 *
 * 当启用了 CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 时，新固件首次启动后
 * 处于“待确认”状态。应用完成基础初始化后调用该函数，Bootloader 才会
 * 把本次启动确认成有效版本；否则设备在异常重启后可能自动回退到旧版本。
 */
void ota_mark_valid(void);

/**
 * @brief 获取线程安全的 OTA 状态快照。
 */
ota_status_t ota_get_status(void);

/**
 * @brief 判断是否正在接收或校验固件。
 */
bool ota_is_active(void);

/**
 * @brief 开始一次 OTA 写入会话。
 *
 * 函数会独占 OTA 写入锁，选择下一个可用分区，检查镜像大小，
 * 然后调用 esp_ota_begin() 准备 Flash 写入。成功后必须继续调用
 * ota_writer_write()，最后调用 ota_writer_finish() 或 ota_writer_abort()。
 *
 * @param writer 由调用者提供、用于保存会话上下文的结构体。
 * @param total  固件总大小；传入 0 表示大小未知，但当前 Web 上传流程
 *               会传入 HTTP Content-Length，因此通常大于 0。
 */
esp_err_t ota_writer_begin(ota_writer_t *writer, size_t total);

/**
 * @brief 把一段连续的固件数据写入当前 OTA 分区。
 *
 * 一个固件可以拆成任意多个数据块写入；每次成功写入都会更新状态进度。
 * data 指向的缓存只需在本次调用期间有效，函数返回后即可复用。
 */
esp_err_t ota_writer_write(ota_writer_t *writer, const void *data, size_t length);

/**
 * @brief 结束写入、校验镜像并设置新的启动分区。
 *
 * 内部依次调用 esp_ota_end() 和 esp_ota_set_boot_partition()。注意：
 * 设置启动分区只影响下一次启动，不会把当前运行中的程序立即替换掉。
 */
esp_err_t ota_writer_finish(ota_writer_t *writer);

/**
 * @brief 中止当前写入会话并记录失败原因。
 *
 * 网络断开、内存不足或 Flash 写入失败时必须调用该函数释放 OTA handle
 * 和全局写入锁，避免后续 OTA 请求永久被判断为“已有升级正在进行”。
 */
void ota_writer_abort(ota_writer_t *writer, const char *reason);
