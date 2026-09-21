/**
 * @file defines.h
 * @brief 项目宏定义和常量配置
 * @author ESP32-S3 Project
 * @date 2023
 * @note 本文件包含项目中所有全局宏定义和常量，按功能模块分类管理
 */

#ifndef DEFINES_H
#define DEFINES_H

/*============================================================================
 * 系统事件标志
 *============================================================================*/
#define START_MUSIC_COMPLETED     BIT0  // 启动音乐播放完成标志
#define WIFI_SET_START            BIT1  // WiFi设置开始标志

/*============================================================================
 * I2C配置
 *============================================================================*/
#define BSP_I2C_SDA               GPIO_NUM_1   // SDA引脚
#define BSP_I2C_SCL               GPIO_NUM_2   // SCL引脚
#define BSP_I2C_NUM               0            // I2C外设编号
#define BSP_I2C_FREQ_HZ           100000       // I2C时钟频率 (100kHz)
#define BSP_I2C_TIMEOUT_MS        1000         // I2C操作超时时间

/*============================================================================
 * SD卡配置
 *============================================================================*/
// SD卡引脚定义
#define BSP_SD_CLK                47           // SD卡时钟引脚
#define BSP_SD_CMD                48           // SD卡命令引脚
#define BSP_SD_D0                 21           // SD卡数据0引脚
#define SD_CMD_IO                 48           // SD卡命令引脚（别名）
#define SD_CLK_IO                 47           // SD卡时钟引脚（别名）
#define SD_DAT0_IO                21           // SD卡数据0引脚（别名）

// SD卡挂载配置
#define MOUNT_POINT               "/sdcard"    // SD卡挂载点
#define SD_MOUNT_POINT            "/sdcard"    // SD卡挂载点（别名）
#define EXAMPLE_MAX_CHAR_SIZE     64           // 最大字符长度

/*============================================================================
 * 音频配置
 *============================================================================*/
#define ADC_I2S_CHANNEL           4            // ADC I2S通道号
#define VOLUME_DEFAULT            75           // 默认声音大小 0~100
#define EXAMPLE_VOICE_VOLUME      70           // 示例语音音量

// 编解码器默认参数
#define CODEC_DEFAULT_SAMPLE_RATE (8000)       // 采样率
#define CODEC_DEFAULT_BIT_WIDTH   (16)         // 位宽
#define CODEC_DEFAULT_ADC_VOLUME  (24.0)       // ADC音量
#define CODEC_DEFAULT_CHANNEL     (2)          // 通道数

// I2S配置
#define BSP_I2S_NUM               I2S_NUM_1    // I2S端口号
#define I2S_NUM                   0            // 示例I2S端口号

// I2S引脚定义
#define GPIO_I2S_LRCK             GPIO_NUM_13  // 左右声道时钟
#define GPIO_I2S_MCLK             GPIO_NUM_38  // 主时钟
#define GPIO_I2S_SCLK             GPIO_NUM_14  // 串行时钟
#define GPIO_I2S_SDIN             GPIO_NUM_12  // 串行数据输入
#define GPIO_I2S_DOUT             GPIO_NUM_45  // 串行数据输出
#define GPIO_PWR_CTRL             GPIO_NUM_NC  // 电源控制引脚（未使用）
#define I2S_MCK_IO                GPIO_NUM_38  // 主时钟（别名）
#define I2S_BCK_IO                GPIO_NUM_14  // 位时钟（别名）
#define I2S_WS_IO                 GPIO_NUM_13  // 字时钟（别名）
#define I2S_DO_IO                 GPIO_NUM_45  // 数据输出（别名）
#define I2S_DI_IO                 (-1)         // 数据输入（未使用）

// I2S音频参数
#define EXAMPLE_RECV_BUF_SIZE     (2400)       // 接收缓冲区大小
#define EXAMPLE_SAMPLE_RATE       (16000)      // 示例采样率
#define EXAMPLE_MCLK_MULTIPLE     (384)        // MCLK倍数
#define EXAMPLE_MCLK_FREQ_HZ      (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE) // MCLK频率

/*============================================================================
 * 传感器配置
 *============================================================================*/
// QMI8658加速度传感器
#define QMI8658_SENSOR_ADDR       0x6A         // QMI8658 I2C地址

// PCA9557 IO扩展芯片
#define PCA9557_SENSOR_ADDR       0x19         // PCA9557 I2C地址
#define PCA9557_INPUT_PORT        0x00         // 输入端口寄存器
#define PCA9557_OUTPUT_PORT       0x01         // 输出端口寄存器
#define PCA9557_POLARITY_INVERSION_PORT 0x02   // 极性反转寄存器
#define PCA9557_CONFIGURATION_PORT 0x03        // 配置寄存器

// PCA9557 GPIO定义
#define LCD_CS_GPIO               BIT(0)       // LCD片选GPIO
#define PA_EN_GPIO                BIT(1)       // 功率放大器使能GPIO
#define DVP_PWDN_GPIO             BIT(2)       // 摄像头电源控制GPIO

// 通用位操作宏
#define SET_BITS(_m, _s, _v)      ((_v) ? (_m) | ((_s)) : (_m) & ~((_s))) // 位设置宏

/*============================================================================
 * 显示屏配置
 *============================================================================*/
// 屏幕基本参数
#define SCREEN_WIDTH              320          // 屏幕宽度
#define SCREEN_HEIGHT             240          // 屏幕高度
#define BSP_LCD_H_RES             320          // 屏幕水平分辨率（别名）
#define BSP_LCD_V_RES             240          // 屏幕垂直分辨率（别名）

// LCD硬件参数
#define BSP_LCD_PIXEL_CLOCK_HZ    (80 * 1000 * 1000) // 像素时钟频率
#define BSP_LCD_SPI_NUM           SPI3_HOST    // LCD SPI端口
#define LCD_CMD_BITS              (8)          // 命令位数
#define LCD_PARAM_BITS            (8)          // 参数位数
#define BSP_LCD_BITS_PER_PIXEL    (16)         // 每像素位数
#define LCD_LEDC_CH               LEDC_CHANNEL_0 // LEDC通道
#define BSP_LCD_DRAW_BUF_HEIGHT   (20)         // 绘制缓冲区高度

// LCD引脚定义
#define BSP_LCD_SPI_MOSI          GPIO_NUM_40  // SPI MOSI引脚
#define BSP_LCD_SPI_CLK           GPIO_NUM_41  // SPI CLK引脚
#define BSP_LCD_SPI_CS            GPIO_NUM_NC  // SPI CS引脚（未使用）
#define BSP_LCD_DC                GPIO_NUM_39  // DC引脚
#define BSP_LCD_RST               GPIO_NUM_NC  // 复位引脚（未使用）
#define BSP_LCD_BACKLIGHT         GPIO_NUM_42  // 背光引脚

/*============================================================================
 * 摄像头配置
 *============================================================================*/
#define CAMERA_EN                 1            // 摄像头使能

// 摄像头引脚定义
#define CAMERA_PIN_PWDN           -1           // 电源关闭引脚（未使用）
#define CAMERA_PIN_RESET          -1           // 复位引脚（未使用）
#define CAMERA_PIN_XCLK           5            // 时钟引脚
#define CAMERA_PIN_SIOD           1            // I2C SDA引脚
#define CAMERA_PIN_SIOC           2            // I2C SCL引脚
#define CAMERA_PIN_D7             9            // 数据位7
#define CAMERA_PIN_D6             4            // 数据位6
#define CAMERA_PIN_D5             6            // 数据位5
#define CAMERA_PIN_D4             15           // 数据位4
#define CAMERA_PIN_D3             17           // 数据位3
#define CAMERA_PIN_D2             8            // 数据位2
#define CAMERA_PIN_D1             18           // 数据位1
#define CAMERA_PIN_D0             16           // 数据位0
#define CAMERA_PIN_VSYNC          3            // 垂直同步引脚
#define CAMERA_PIN_HREF           46           // 水平参考引脚
#define CAMERA_PIN_PCLK           7            // 像素时钟引脚

// 摄像头时钟配置
#define XCLK_FREQ_HZ              24000000     // 摄像头时钟频率

/*============================================================================
 * 文件系统配置
 *============================================================================*/
#define SPIFFS_BASE               "/spiffs"    // SPIFFS挂载点

/*============================================================================
 * WiFi配置
 *============================================================================*/
#define MY_WIFI_SSID              "Sci this way"      // WiFi名称
#define MY_WIFI_PASSWORD          "n507thisway" // WiFi密码
#define ESP_MAXIMUM_RETRY         5            // WiFi最大重试次数

// WiFi事件标志
#define WIFI_CONNECTED_BIT        BIT0         // WiFi已连接标志
#define WIFI_FAIL_BIT             BIT1         // WiFi连接失败标志

/*============================================================================
 * 天气API配置
 *============================================================================*/
#define FORECAST_DAY              3            // 天气预报天数
#define HOST                      "api.seniverse.com" // 天气API主机
#define HTTP_PORT                 80           // HTTP端口
#define API_KEY                   "SJj5ROp2qcczMMZkT" // API密钥
#define LOCATION                  "huilai"    // 地点
#define LANGUAGE                  "zh-Hans"   // 语言（简体中文）
#define TEMPERATURE_UNIT          "c"         // 温度单位（摄氏度）
#define RESPONSE_BODY_MAX_SIZE    1024         // 响应体最大大小
#define REQUEST_INTERVAL          1            // 请求间隔（分钟）
#define MAX_RETRY_COUNT           5            // HTTP请求最大重试次数

/*============================================================================
 * 应用程序配置
 *============================================================================*/
// 小狗动画配置
#define DOG_ANIMATION_FRAME_COUNT 35           // 小狗动画帧数量

// 按键配置
#define KEY_GPIO                  GPIO_NUM_0   // 按键GPIO
#define DEBOUNCE_TIME_MS          50           // 消抖时间(毫秒)
#define KEY_DEBOUNCE_TIME         20           // 按键消抖时间(毫秒) - 兼容旧代码

// 菜单配置
#define ICON_DISTANCE             100          // 图标间距
#define ICON_SIZE_SMALL           60           // 小图标大小
#define ICON_SIZE_BIG             90           // 大图标大小
#define ICON_SIZE_SMALL_HEIGHT    ICON_SIZE_SMALL+20           // 小图标高度
#define ICON_SIZE_BIG_HEIGHT      ICON_SIZE_BIG+20           // 大图标高度

// 小球游戏配置
#define BALL_RADIUS               30           // 小球半径

// 球体物理参数 - 正常模式
#define SENSITIVITY_X_NORMAL      2.0f         // 正常模式X轴灵敏度系数
#define SENSITIVITY_Y_NORMAL      2.0f         // 正常模式Y轴灵敏度系数
#define FRICTION_NORMAL           0.95f        // 正常模式摩擦力系数
#define RESTITUTION_NORMAL        0.8f         // 正常模式回弹系数

// 球体物理参数 - 反重力模式
#define SENSITIVITY_X_ANTI_GRAVITY 0.1f        // 反重力模式X轴灵敏度系数
#define SENSITIVITY_Y_ANTI_GRAVITY 0.1f        // 反重力模式Y轴灵敏度系数
#define RESTITUTION_ANTI_GRAVITY  0.5f         // 反重力模式回弹系数

// 球体物理参数 - 超弹力模式
#define SENSITIVITY_X_SUPER       3.0f         // 超弹力模式X轴灵敏度系数
#define SENSITIVITY_Y_SUPER       3.0f         // 超弹力模式Y轴灵敏度系数
#define FRICTION_SUPER            0.98f        // 超弹力模式摩擦力系数（低摩擦）
#define RESTITUTION_SUPER         0.9f         // 超弹力模式回弹系数

// 球体物理参数 - 粘性模式
#define FRICTION_STICKY           0.0f         // 粘性模式摩擦力系数（完全停止）
#define RESTITUTION_STICKY        0.0f         // 粘性模式回弹系数（零回弹，粘墙效果）
#define STICK_DURATION_MS         1000         // 粘性模式粘墙持续时间（毫秒）

// 球体物理参数 - 疯狂模式
#define CRAZY_CHANGE_INTERVAL     200          // 疯狂模式速度改变间隔（ms）
#define CRAZY_VELOCITY_RANGE      120          // 疯狂模式随机速度范围

#endif /* DEFINES_H */
