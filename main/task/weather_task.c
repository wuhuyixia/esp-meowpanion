#include "headfile.h"

#include "ui/mainmenu.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "ota_app.h"

// 声明更新天气UI显示的函数（与mainmenu.h中的声明保持一致）
void update_mainpage_weather_display(const char *today_weather, const char *today_temp, 
                                    const char *tomorrow_weather, const char *tomorrow_temp, 
                                    const char *city);



#define RESPONSE_BODY_MAX_SIZE 1024
#define REQUEST_INTERVAL 1 //  minutes
#define MAX_RETRY_COUNT 5  // HTTP请求最大重试次数

void cjson_parse_xinzhi_weather(char *rdata);

// start:起始日期 days:天数  0:今天的天气 1:明天的天气  以此类推...
static const char *TAG = "weather";
// 使用HTTP协议而不是HTTPS，以避免SSL证书问题
static const char *weather_url = "http://api.seniverse.com/v3/weather/daily.json?key=" API_KEY "&location=" LOCATION "&language=" LANGUAGE "&unit=" TEMPERATURE_UNIT "&start=0&days=3";
static char response_body[RESPONSE_BODY_MAX_SIZE]; // 用于接收通过http响应体报文
weather_t weather;

extern uint8_t get_wifi_status(void);

bool check_wifi_connection(void)
{
    uint8_t wifi_status = get_wifi_status();
    if (wifi_status != 1)
    {
        ESP_LOGE(TAG, "WiFi not connected, status: %d", wifi_status);
        return false;
    }
    ESP_LOGI(TAG, "WiFi is connected");
    return true;
}

void weather_task(void *param)
{
    int content_length = 0;
    int retry_count = 0;

    // 创建一个esp_http_client_config_t的实例（实例化对象），并配置HTTP-Client句柄
    esp_http_client_config_t http_client_cfg = {
        .url = weather_url,
        .disable_auto_redirect = true,
        .keep_alive_enable = true,
        .keep_alive_idle = 5,
        .keep_alive_interval = 5,
    };

    esp_http_client_handle_t http_client_handle = esp_http_client_init(&http_client_cfg); // 初始化http连接
    esp_http_client_set_method(http_client_handle, HTTP_METHOD_GET);                      // 向服务器发送get请求
    memset(&weather, 0, sizeof(weather_t));

    memset(&response_body, 0, sizeof(response_body));
    while (1)
    {
        // Do not start another weather request while a Web OTA is receiving
        // an image and using the WiFi/flash path.
        if (ota_is_active())
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // 先检查WiFi连接状态
        if (!check_wifi_connection())
        {
            ESP_LOGE(TAG, "Skipping weather request: WiFi not connected");
            goto __fail_delay;
        }

        retry_count = 0;
        bool request_success = false;

        // 添加重试机制
        while (retry_count < MAX_RETRY_COUNT && !request_success && !ota_is_active())
        {
            retry_count++;
            ESP_LOGI(TAG, "HTTP request attempt %d/%d", retry_count, MAX_RETRY_COUNT);

            // 重置响应缓冲区
            memset(response_body, 0, sizeof(response_body));

            // 与目标主机创建连接，并且声明写入内容长度为0,打开连接，发送http请求头
            esp_err_t err = esp_http_client_open(http_client_handle, 0);

            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
                // 短暂延迟后重试
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                continue;
            }

            // 读取响应头报文，自动处理掉响应头并返回接收包长度
            content_length = esp_http_client_fetch_headers(http_client_handle);
            if (content_length < 0 || content_length > RESPONSE_BODY_MAX_SIZE) // 返回响应头报文长度太短太长都是问题
            {
                ESP_LOGE(TAG, "HTTP client fetch headers failed, content_length: %d", content_length);
                esp_http_client_close(http_client_handle);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                continue;
            }

            // 获取并检查HTTP状态码
            int status_code = esp_http_client_get_status_code(http_client_handle);
            if (status_code != 200)
            {
                ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
                esp_http_client_close(http_client_handle);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                continue;
            }

            // 读取响应体，使用增强的读取机制
            int read_length = 0;
            int total_read = 0;
            const int MAX_READ_ATTEMPTS = 10;
            int read_attempts = 0;

            while (total_read < RESPONSE_BODY_MAX_SIZE - 1 && read_attempts < MAX_READ_ATTEMPTS)
            {
                read_attempts++;
                read_length = esp_http_client_read_response(http_client_handle,
                                                            response_body + total_read,
                                                            RESPONSE_BODY_MAX_SIZE - 1 - total_read);

                if (read_length < 0)
                {
                    ESP_LOGE(TAG, "Failed to read response, attempt %d", read_attempts);
                    break;
                }
                else if (read_length == 0)
                {
                    // 读取到0字节，可能是已读完或网络暂时中断
                    if (total_read > 0)
                    {
                        // 已经读取到一些数据，可能是读取完成
                        ESP_LOGI(TAG, "Read completed after %d bytes", total_read);
                        break;
                    }
                    // 第一次读取就得到0字节，等待一段时间后重试
                    ESP_LOGI(TAG, "Read 0 bytes, waiting...");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                else
                {
                    total_read += read_length;
                    ESP_LOGI(TAG, "Read %d bytes, total: %d", read_length, total_read);
                }
            }

            if (total_read <= 0)
            {
                ESP_LOGE(TAG, "Failed to read any data after %d attempts", read_attempts);
                esp_http_client_close(http_client_handle);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                continue;
            }

            // 确保响应体以null结尾
            response_body[total_read] = '\0';

            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d, actual_read = %d",
                     status_code,
                     content_length,
                     total_read);

            // 打印完整的响应体内容
            ESP_LOGI(TAG, "完整响应体内容:");
            printf("%s\n", response_body);

            // 尝试解析JSON数据
            cjson_parse_xinzhi_weather(response_body);
            esp_http_client_close(http_client_handle); // 断开连接
            request_success = true;
            ESP_LOGI(TAG, "Weather data fetched and parsed successfully");
            
            // 更新UI显示
        char today_temp_range[20], tomorrow_temp_range[20];
        // 构建今天的温度范围字符串
        sprintf(today_temp_range, "%d°C/%d°C", weather.degree_low[0], weather.degree_high[0]);
        // 构建明天的温度范围字符串
        sprintf(tomorrow_temp_range, "%d°C/%d°C", weather.degree_low[1], weather.degree_high[1]);
        // 使用正确的结构体成员和索引调用函数
        update_mainpage_weather_display(weather.text_day[0], today_temp_range, 
                                       weather.text_day[1], tomorrow_temp_range, 
                                       weather.city);
        }

    __fail_delay:
        // 使用更灵活的延迟方式
        for (size_t i = 0; i < REQUEST_INTERVAL * 60; i++)
        {
            // 每10秒检查一次WiFi状态
            if (i % 10 == 0)
            {
                ESP_LOGI(TAG, "Waiting %d seconds for next weather update", REQUEST_INTERVAL * 60 - i);
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
// cjson解析心知天气报文
void cjson_parse_xinzhi_weather(char *rdata)
{
    cJSON *pJsonRoot = cJSON_Parse(rdata); // 解析json字符串

    // 如果是json格式数据，则开始解析
    if (pJsonRoot == NULL)
    {
        ESP_LOGE(TAG, "JSON解析失败: %s", cJSON_GetErrorPtr());
        goto __fail_exit;
    }

    // 获取results数组内容
    cJSON *pResults = cJSON_GetObjectItem(pJsonRoot, "results");
    if (pResults == NULL)
    {
        ESP_LOGE(TAG, "未能找到results字段");
        goto __fail_exit;
    }

    cJSON *pObject = cJSON_GetArrayItem(pResults, 0); // 解析 results 数组的内容
    if (pObject == NULL)
    {
        ESP_LOGE(TAG, "results数组为空");
        goto __fail_exit;
    }

    // 解析 results -> location 数组元素内容
    cJSON *pLocation = cJSON_GetObjectItem(pObject, "location");
    if (pLocation == NULL)
    {
        ESP_LOGE(TAG, "未能找到location字段");
        goto __fail_exit;
    }

    // 解析 results -> daily 数组元素内容
    cJSON *pDaily = cJSON_GetObjectItem(pObject, "daily");
    if (pDaily == NULL)
    {
        ESP_LOGE(TAG, "未能找到daily字段，尝试使用now字段");
        // 尝试解析now.json格式的响应
        cJSON *pNow = cJSON_GetObjectItem(pObject, "now");
        if (pNow == NULL)
        {
            ESP_LOGE(TAG, "既没有找到daily字段也没有找到now字段");
            goto __fail_exit;
        }

        // 解析now格式的数据
        memset(&weather, 0, sizeof(weather_t));
        strcpy(weather.city, cJSON_GetObjectItem(pLocation, "name")->valuestring);

        // 只填充今天的数据
        strcpy(weather.text_day[0], cJSON_GetObjectItem(pNow, "text")->valuestring);
        strcpy(weather.text_night[0], cJSON_GetObjectItem(pNow, "text")->valuestring);
        weather.code_day[0] = atoi(cJSON_GetObjectItem(pNow, "code")->valuestring);
        weather.code_night[0] = atoi(cJSON_GetObjectItem(pNow, "code")->valuestring);

        // 从now数据中获取当前温度，作为最高和最低温度
        int current_temp = atoi(cJSON_GetObjectItem(pNow, "temperature")->valuestring);
        weather.degree_high[0] = current_temp;
        weather.degree_low[0] = current_temp;

        // 尝试获取湿度，如果没有则设为0
        cJSON *pHumidity = cJSON_GetObjectItem(pNow, "humidity");
        weather.humidity[0] = pHumidity ? atoi(pHumidity->valuestring) : 0;

        // 打印解析后的天气数据
        ESP_LOGI(TAG, "===== 天气数据解析结果 =====");
        ESP_LOGI(TAG, "城市: %s", weather.city);
        ESP_LOGI(TAG, "今天天气:");
        ESP_LOGI(TAG, "  天气状况: %s (代码: %d)", weather.text_day[0], weather.code_day[0]);
        ESP_LOGI(TAG, "  当前温度: %d°C", weather.degree_high[0]);
        ESP_LOGI(TAG, "  湿度: %d%%", weather.humidity[0]);
        ESP_LOGI(TAG, "============================");

        goto __fail_exit;
    }

    // 正常解析daily格式数据
    memset(&weather, 0, sizeof(weather_t));
    // 获取城市
    strcpy(weather.city, cJSON_GetObjectItem(pLocation, "name")->valuestring);
    // 获取 daily 的数组长度(默认3天)
    int daily_size = cJSON_GetArraySize(pDaily);

    // 打印解析后的天气数据
    ESP_LOGI(TAG, "===== 天气数据解析结果 =====");
    ESP_LOGI(TAG, "城市: %s", weather.city);
    ESP_LOGI(TAG, "预报天数: %d天", daily_size);

    for (int i = 0; i < daily_size && i < 3; i++) // 最多显示3天
    {
        cJSON *daily_elem = cJSON_GetArrayItem(pDaily, i); // 从daily数组从取第i个元素
        if (daily_elem != NULL)
        {
            // 根据索引判断是今天、明天还是后天
            const char *day_desc = NULL;
            if (i == 0)
                day_desc = "今天";
            else if (i == 1)
                day_desc = "明天";
            else if (i == 2)
                day_desc = "后天";
            else
                day_desc = "未来";

            /* 为简化代码, 不考虑解析失败情况(这里解析失败概率极低) */
            strcpy(weather.text_day[i], cJSON_GetObjectItem(daily_elem, "text_day")->valuestring);
            strcpy(weather.text_night[i], cJSON_GetObjectItem(daily_elem, "text_night")->valuestring);
            weather.code_day[i] = atoi(cJSON_GetObjectItem(daily_elem, "code_day")->valuestring);
            weather.code_night[i] = atoi(cJSON_GetObjectItem(daily_elem, "code_night")->valuestring);
            weather.degree_high[i] = atoi(cJSON_GetObjectItem(daily_elem, "high")->valuestring);
            weather.degree_low[i] = atoi(cJSON_GetObjectItem(daily_elem, "low")->valuestring);
            weather.humidity[i] = atoi(cJSON_GetObjectItem(daily_elem, "humidity")->valuestring);

            // 打印每一天的天气信息
            ESP_LOGI(TAG, "%s天气:", day_desc);
            ESP_LOGI(TAG, "  白天: %s (代码: %d)", weather.text_day[i], weather.code_day[i]);
            ESP_LOGI(TAG, "  夜晚: %s (代码: %d)", weather.text_night[i], weather.code_night[i]);
            ESP_LOGI(TAG, "  温度范围: %d°C ~ %d°C", weather.degree_low[i], weather.degree_high[i]);
            ESP_LOGI(TAG, "  湿度: %d%%", weather.humidity[i]);
        }
    }

    ESP_LOGI(TAG, "============================");

__fail_exit:
    if (pJsonRoot != NULL)
        cJSON_Delete(pJsonRoot); // 释放cJSON_Parse分配内存
}
