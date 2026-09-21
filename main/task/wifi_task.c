#include "headfile.h"

#include <time.h>
#include <sys/time.h>
#include "esp_system.h"

#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "esp_netif.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "esp_mac.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif



static const char *TAG = "wifi";
uint16_t wifi_ip_addr[4];
uint8_t wifi_connect_flag = 0;

// 全局时间变量，用于在其他地方访问当前时间
struct tm global_timeinfo = {0};
bool global_time_valid = false; // 标记时间是否有效

// 事件处理回调
void event_handler_cb(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START: // WIFI以STA模式启动后触发此事件
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "conneted success!");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            wifi_connect_flag = 0;
            esp_wifi_connect();
            ESP_LOGI(TAG, "retry to connect...");
            break;

        default:
            break;
        }
    }
    if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            wifi_connect_flag = 1;
            ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
            wifi_ip_addr[0] = esp_ip4_addr1_16(&ev->ip_info.ip);
            wifi_ip_addr[1] = esp_ip4_addr2_16(&ev->ip_info.ip);
            wifi_ip_addr[2] = esp_ip4_addr3_16(&ev->ip_info.ip);
            wifi_ip_addr[3] = esp_ip4_addr4_16(&ev->ip_info.ip);
            ESP_LOGI(TAG, "get ip: %d.%d.%d.%d", wifi_ip_addr[0], wifi_ip_addr[1], wifi_ip_addr[2], wifi_ip_addr[3]);
            break;

        default:
            break;
        }
    }
}

esp_err_t wifi_sta_init(void)
{
    // 初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_netif_init());
    // 创建一个默认系统事件调度循环，之后可以注册回调函数来处理系统的一些事件
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 使用默认配置创建STA对象
    esp_netif_create_default_wifi_sta();

    // 初始化wifi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册WIFI以及IP事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_cb, NULL));

    // wifi配置
    wifi_config_t wifi_cfg =
        {
            .sta =
                {
                    .ssid = MY_WIFI_SSID,
                    .password = MY_WIFI_PASSWORD,
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK, // 加密方式

                    .pmf_cfg =
                        {
                            .capable = true,  // 启用保护管理帧
                            .required = false // 禁止仅与保护管理帧设备通信
                        }}};
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    // 启动wifi
    esp_wifi_start();

    return ESP_OK;
}

uint8_t get_wifi_status(void)
{
    return wifi_connect_flag;
}

/**
 * @brief WiFi连接监控任务
 * @details 监控WiFi连接状态，当WiFi连接成功后启动SNTP任务
 */
void wifi_monitor_task(void* pvParameters)
{
    ESP_LOGI(TAG, "WiFi monitor task started");
    
    // 等待WiFi连接
    while (!wifi_connect_flag) {
        ESP_LOGI(TAG, "Waiting for WiFi connection...");
        vTaskDelay(3000 / portTICK_PERIOD_MS); // 每3秒检查一次
    }
    
    // WiFi已连接，启动SNTP和天气任务
    ESP_LOGI(TAG, "WiFi connected, starting SNTP and weather tasks");
    xTaskCreatePinnedToCore(sntp_task, "sntp_task", 4*1152, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(weather_task, "weather_task", 4*1152, NULL, 4, NULL, 1);
    
    // 任务完成，删除自身
    vTaskDelete(NULL);
}
/////////////////////////////////////////////////////////////////////////////

RTC_DATA_ATTR static int boot_count = 0; 

static void obtain_time(void);
static void continuous_time_sync(void); // 连续时间同步函数声明
 
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif
 
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
 
void sntp_task(void* param)
{
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
    
    
 
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
    ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    else {
        // add 500 ms error to the current system time.
        // Only to demonstrate a work of adjusting method!
        {
            ESP_LOGI(TAG, "Add a error for test adjtime");
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
            int64_t error_time = cpu_time + 500 * 1000L;
            struct timeval tv_error = { .tv_sec = error_time / 1000000L, .tv_usec = error_time % 1000000L };
            settimeofday(&tv_error, NULL);
        }
 
        ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#endif
 
    char strftime_buf[64];
 
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
 
    uint8_t rest = sntp_get_sync_mode();
    printf("mode_id: %d\r\n", rest);
    if (rest == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        rest = sntp_get_sync_status();
        printf("sync status: %d\r\n", rest);
        while (rest == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %jd sec: %li ms: %li us",
                        (intmax_t)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
 
    // const int deep_sleep_sec = 10;
    // ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
    // esp_deep_sleep(1000000LL * deep_sleep_sec);
    
    ESP_LOGI(TAG, "准备启动连续时间同步...");
    // 启动连续时间同步
    continuous_time_sync();
    
  // vTaskDelete(NULL);
}
 
static void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");
 
    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}
 
static void obtain_time(void)
{
    // 首先检查网络连接状态
    if (!wifi_connect_flag) {
        ESP_LOGI(TAG, "Network not connected, skipping NTP time synchronization");
        // 设置一个默认时间，避免时间显示异常
        time_t now = time(NULL);
        struct tm default_timeinfo = {
            .tm_year = 123,  // 2023 - 1900
            .tm_mon = 7,     // 8月 (0-11)
            .tm_mday = 31,   // 31日
            .tm_hour = 20,   // 20点
            .tm_min = 42,    // 42分
            .tm_sec = 47,    // 47秒
            .tm_wday = 4     // 周四 (0-6, 0是周日)
        };
        // 将默认时间转换为time_t并设置系统时间
        time_t default_time = mktime(&default_timeinfo);
        struct timeval tv = { .tv_sec = default_time, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
 
#if LWIP_DHCP_GET_NTP_SRV 
    /**
     * NTP server address could be acquired via DHCP,
     * see following menuconfig options:
     * 'LWIP_DHCP_GET_NTP_SRV' - enable STNP over DHCP
     * 'LWIP_SNTP_DEBUG' - enable debugging messages
     *
     * NOTE: This call should be made BEFORE esp acquires IP address from DHCP,
     * otherwise NTP option would be rejected by default.
     */
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(3, ESP_SNTP_SERVER_LIST("cn.pool.ntp.org", "time.windows.com", "ntp.sjtu.edu.cn"));
 
    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
#else
    config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;
#endif
    config.sync_cb = time_sync_notification_cb; // only if we need the notification function
    esp_netif_sntp_init(&config);
 
#endif /* LWIP_DHCP_GET_NTP_SRV */
 
#if LWIP_DHCP_GET_NTP_SRV
    ESP_LOGI(TAG, "Starting SNTP");
    esp_netif_sntp_start();
#endif
 
    print_servers();
 
    // wait for time to be set with shorter timeout
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 5;  // 减少重试次数从15次到5次
    while (esp_netif_sntp_sync_wait(1000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        
        // 如果在等待过程中网络断开，立即退出循环
        if (!wifi_connect_flag) {
            ESP_LOGI(TAG, "Network disconnected during NTP sync, aborting");
            break;
        }
    }
    time(&now);
    localtime_r(&now, &timeinfo);
 
    esp_netif_sntp_deinit();
}

/**
 * @brief 连续时间同步函数
 * @details 实现每秒打印一次时间，每小时自动同步一次时间的功能
 */
static void continuous_time_sync(void)
{
    const int SYNC_INTERVAL_HOURS = 1;        // 同步间隔（小时）
    const int CHECK_INTERVAL_SECONDS = 1;     // 检查间隔（秒）
    int second_counter = 0;                   // 秒计数器
    char strftime_buf[64];
    
    // 初始化时立即检查当前时间是否有效
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 如果时间仍为1970年（未同步），设置一个默认时间
    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGI(TAG, "Initial time invalid (1970), setting default time");
        // 设置默认时间，避免显示1970年
        struct tm default_timeinfo = {
            .tm_year = 123,  // 2023 - 1900
            .tm_mon = 7,     // 8月 (0-11)
            .tm_mday = 31,   // 31日
            .tm_hour = 20,   // 20点
            .tm_min = 42,    // 42分
            .tm_sec = 47,    // 47秒
            .tm_wday = 4     // 周四 (0-6, 0是周日)
        };
        // 将默认时间转换为time_t并设置系统时间
        time_t default_time = mktime(&default_timeinfo);
        struct timeval tv = { .tv_sec = default_time, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        
        // 更新当前时间信息
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    
    // 初始化全局时间变量
    memcpy(&global_timeinfo, &timeinfo, sizeof(struct tm));
    global_time_valid = true;
    
    ESP_LOGW(TAG, "==== 连续时间同步已启动 - 每秒打印一次时间 ====");
    ESP_LOGW(TAG, "Starting continuous time synchronization (every %d hours) with second-by-second display", SYNC_INTERVAL_HOURS);
    
    // 主循环，定期检查和同步时间
    while (1) {
        // 显示当前时间并更新全局时间变量
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        // 更新全局时间变量（使用memcpy确保完整复制）
        memcpy(&global_timeinfo, &timeinfo, sizeof(struct tm));
        global_time_valid = true;
        
        ESP_LOGW(TAG, "==== 当前上海时间: %s ====", strftime_buf);
        
        // 增加秒计数器
        second_counter += CHECK_INTERVAL_SECONDS;
        
        // 检查是否达到同步时间（每小时）
        if (second_counter >= SYNC_INTERVAL_HOURS * 3600) {
            ESP_LOGI(TAG, "Time to synchronize with NTP server...");
            
            // 只有在网络连接的情况下才进行时间同步
            if (wifi_connect_flag) {
                // 重新同步时间
                obtain_time();
                
                // 显示同步后的时间并更新全局时间变量
                time(&now);
                localtime_r(&now, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
                
                // 更新全局时间变量
                memcpy(&global_timeinfo, &timeinfo, sizeof(struct tm));
                global_time_valid = true;
                
                ESP_LOGI(TAG, "Time synchronized. New time: %s", strftime_buf);
            } else {
                ESP_LOGI(TAG, "Network not connected, skipping scheduled time synchronization");
            }
            
            // 无论是否同步成功，都重置计数器
            second_counter = 0;
        }
        
        // 每秒检查一次
        vTaskDelay((CHECK_INTERVAL_SECONDS * 1000) / portTICK_PERIOD_MS);
    }
}
