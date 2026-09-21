# 收集images文件夹中的所有源文件
file(GLOB IMAGE_SOURCES "images/doganim/*.c")

# 收集ui文件夹中的所有源文件
file(GLOB UI_SOURCES "ui/*.c")

# 收集bsp文件夹中的所有源文件
file(GLOB BSP_SOURCES "bsp/*.c")

# 收集task文件夹中的所有源文件
file(GLOB TASK_SOURCES "task/*.c")



set(EMBEDDED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/sword.pcm")
# 注册主组件
idf_component_register(SRCS 
                    "esp32_s3_szp.c" 
                    "main.c" 
                    "ota_app.c"
                    "ota_web.c"
                    ${IMAGE_SOURCES}
                    ${UI_SOURCES}
                    ${BSP_SOURCES}
                    ${TASK_SOURCES}
                    INCLUDE_DIRS 
                    "."
                    "ui"
                    "bsp"
                    PRIV_REQUIRES
                    # app_update 提供 esp_ota_begin/write/end 等 OTA API；
                    # esp_http_server 提供浏览器上传和状态查询接口。
                    app_update
                    esp_event
                    esp_http_server
                    esp_http_client
                    esp_netif
                    esp_wifi
                    esp_driver_gpio
                    esp_driver_i2c
                    esp_driver_i2s
                    esp_driver_ledc
                    esp_driver_sdmmc
                    esp_driver_spi
                    esp_lcd
                    esp_system
                    esp_timer
                    fatfs
                    json
                    nvs_flash
                    sdmmc
                    spiffs
                    EMBED_FILES
                    ${EMBEDDED_FILES})

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
