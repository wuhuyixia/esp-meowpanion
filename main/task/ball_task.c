/*
 * @file ball_task.c
 * @brief 小球动画任务实现
 * @details 本文件实现了一个基于加速度传感器控制的小球动画任务，主要功能包括：
 *          1. 根据传感器数据控制小球在屏幕上的物理运动
 *          2. 提供多种小球颜色模式选择
 *          3. 处理模式切换按钮的事件响应
 *          4. 管理小球UI元素的创建和销毁
 *          5. 实现小球碰撞检测和反弹物理效果
 */
#include "headfile.h"
#include "ball_task.h"

static const char *TAG = "ball_task";

// 声明外部ball_screen变量
extern lv_obj_t *ball_screen;
t_sQMI8658 sensor_data = {0};                  // QMI8658传感器数据结构体
static lv_obj_t *ball = NULL;                  // 小球对象
static TaskHandle_t ball_task_handle = NULL;   // 任务句柄
static ball_mode_t current_mode = MODE_NORMAL; // 当前模式
static lv_obj_t *mode_button = NULL;           // 模式切换按钮
// 小球位置和速度限制函数（带回弹）
/**
 * @brief 处理小球碰撞反弹逻辑
 * @details 根据小球当前位置和速度，检测与屏幕边界的碰撞，并实现反弹效果
 * @param x 小球当前X坐标的指针
 * @param y 小球当前Y坐标的指针
 * @param vx 小球当前X方向速度的指针
 * @param vy 小球当前Y方向速度的指针
 */
static void handle_ball_bounce(int16_t *x, int16_t *y, int16_t *vx, int16_t *vy)
{
    // 根据模式获取回弹系数
    float restitution;

    switch (current_mode)
    {
    case MODE_SUPER_BOUNCE:
        restitution = RESTITUTION_SUPER; // 超弹力模式回弹极强，能量增强
        break;
    case MODE_STICKY:
        restitution = RESTITUTION_STICKY; // 粘性模式回弹明显较弱
        break;
    case MODE_CRAZY:
        restitution = 0.95f; // 疯狂模式回弹最强
        break;
    case MODE_ANTI_GRAVITY:
        restitution = RESTITUTION_ANTI_GRAVITY; // 反重力模式使用正常回弹系数
        break;
    case MODE_NORMAL:
        restitution = RESTITUTION_NORMAL;
        break;
    case MODE_COUNT:
        // MODE_COUNT仅用于计数，使用默认回弹系数
        restitution = RESTITUTION_NORMAL;
        break;
    default:
        restitution = RESTITUTION_NORMAL;
        break;
    }

    // X轴边界检测和回弹 - 保持原始坐标系：-(SCREEN_WIDTH/2)到(SCREEN_WIDTH/2)
    if (*x < (-(SCREEN_WIDTH / 2) + BALL_RADIUS))
    {
        *x = (-(SCREEN_WIDTH / 2) + BALL_RADIUS);
        *vx = -(*vx) * restitution; // 反转速度并应用回弹系数
        ESP_LOGD(TAG, "X轴左边界回弹");
    }
    else if (*x > (SCREEN_WIDTH / 2) - BALL_RADIUS)
    {
        *x = (SCREEN_WIDTH / 2) - BALL_RADIUS;
        *vx = -(*vx) * restitution; // 反转速度并应用回弹系数
        ESP_LOGD(TAG, "X轴右边界回弹");
    }

    // Y轴边界检测和回弹 - 保持原始坐标系：-(SCREEN_HEIGHT/2)到(SCREEN_HEIGHT/2)
    if (*y < (-(SCREEN_HEIGHT / 2) + BALL_RADIUS))
    {
        *y = (-(SCREEN_HEIGHT / 2) + BALL_RADIUS);
        *vy = -(*vy) * restitution; // 反转速度并应用回弹系数
        ESP_LOGD(TAG, "Y轴上边界回弹");
    }
    else if (*y > (SCREEN_HEIGHT / 2) - BALL_RADIUS)
    {
        *y = (SCREEN_HEIGHT / 2) - BALL_RADIUS;
        *vy = -(*vy) * restitution; // 反转速度并应用回弹系数
        ESP_LOGD(TAG, "Y轴下边界回弹");
    }
}

// 根据模式获取小球颜色
static lv_color_t get_ball_color_by_mode(ball_mode_t mode)
{
    switch (mode)
    {
    case MODE_NORMAL:
        return lv_color_hex(0xAE8EE8); // 紫色
    case MODE_ANTI_GRAVITY:
        return lv_color_hex(0x00FFFF); // 青色（代表反重力）
    case MODE_SUPER_BOUNCE:
        return lv_color_hex(0xFFFF00); // 黄色（代表超弹力）
    case MODE_STICKY:
        return lv_color_hex(0xFF00FF); // 品红（代表粘性）
    case MODE_CRAZY:
        return lv_color_hex(0xFF1493); // 深粉红（代表疯狂）
    case MODE_COUNT:
        // MODE_COUNT仅用于计数，使用默认颜色
        ESP_LOGW(TAG, "使用默认颜色：MODE_COUNT");
        return lv_color_hex(0xAE8EE8); // 默认紫色
    default:
        return lv_color_hex(0xAE8EE8); // 默认紫色
    }
}

// 更新小球颜色
/**
 * @brief 更新小球颜色
 * @details 根据当前模式设置小球的填充颜色和边框颜色
 * @param mode 当前小球显示模式（决定颜色方案）
 */
static void update_ball_color(ball_mode_t mode)
{
    if (ball != NULL && lvgl_port_lock(100))
    {
        lv_obj_set_style_bg_color(ball, get_ball_color_by_mode(mode), 0);
        lvgl_port_unlock();
    }
}

// Mode switch button event handler
/**
 * @brief 模式切换按钮事件处理器
 * @details 响应模式按钮的点击事件，切换小球的显示模式
 * @param e LVGL事件对象
 */
static void mode_button_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        // Switch to next mode
        current_mode = (current_mode + 1) % MODE_COUNT;

        // Update button text
        const char *mode_names[MODE_COUNT] = {
            "Normal Mode",
            "Anti-Gravity",
            "Super Bounce",
            "Sticky Mode",
            "Crazy Mode"};

        // English mode descriptions for comments and logging
        const char *mode_descriptions[MODE_COUNT] = {
            "Normal mode: Standard gravity and physics",
            "Anti-Gravity mode: Ball moves in opposite direction to device tilt",

            "Super Bounce mode: Extremely elastic ball with enhanced bouncing",
            "Sticky mode: Ball sticks to walls when hitting them",
            "Crazy mode: Ball jumps randomly without gyroscope input"};

        if (lvgl_port_lock(100))
        {
            lv_label_set_text(lv_obj_get_child(mode_button, 0), mode_names[current_mode]);

            // Adjust button width to fit text
            lv_obj_update_layout(lv_obj_get_child(mode_button, 0));
            lv_coord_t label_width = lv_obj_get_width(lv_obj_get_child(mode_button, 0));
            lv_obj_set_width(mode_button, label_width + 60); // Keep larger margins (30px each side)

            lvgl_port_unlock();
        }

        // Update ball color
        update_ball_color(current_mode);

        ESP_LOGI(TAG, "Switched to mode %d: %s - %s", current_mode, mode_names[current_mode], mode_descriptions[current_mode]);
    }
}

// Create mode switch button
/**
 * @brief 创建模式切换按钮
 * @details 在屏幕上创建一个用于切换小球显示模式的按钮
 */
static void create_mode_button(void)
{
    if (mode_button == NULL && ball_screen != NULL)
    {
        if (lvgl_port_lock(100))
        {
            // Create button
            mode_button = lv_btn_create(ball_screen);
            if (mode_button != NULL)
            {
                // Set button style
                lv_obj_remove_style_all(mode_button);
                lv_obj_set_style_bg_color(mode_button, lv_color_hex(0x333333), 0);
                lv_obj_set_style_bg_opa(mode_button, 200, 0);
                lv_obj_set_style_text_color(mode_button, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_text_font(mode_button, &lv_font_montserrat_14, 0);
                lv_obj_set_style_radius(mode_button, 10, 0);

                // Add label to button
                lv_obj_t *label = lv_label_create(mode_button);
                // 使用英文文本保持一致性，确保与模式切换后的字体显示兼容
                lv_label_set_text(label, "Normal Mode");
                lv_obj_center(label);

                // Set button position (top center of screen)
                // Make button larger for better touch experience
                lv_obj_set_height(mode_button, 40); // Increase button height
                lv_obj_update_layout(label);
                lv_coord_t label_width = lv_obj_get_width(label);
                lv_obj_set_width(mode_button, label_width + 60); // Increase margins to 30px on each side
                lv_obj_align(mode_button, LV_ALIGN_TOP_MID, 0, 20);

                // Add event callback
                lv_obj_add_event_cb(mode_button, mode_button_event_handler, LV_EVENT_CLICKED, NULL);

                ESP_LOGI(TAG, "Mode switch button created successfully");
            }
            lvgl_port_unlock();
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get LVGL lock");
        }
    }
}

// 删除模式切换按钮
static void delete_mode_button(void)
{
    if (mode_button != NULL)
    {
        if (lvgl_port_lock(100))
        {
            lv_obj_del(mode_button);
            mode_button = NULL;
            ESP_LOGI(TAG, "模式切换按钮已删除");
            lvgl_port_unlock();
        }
        else
        {
            ESP_LOGE(TAG, "获取LVGL锁失败");
        }
    }
}

// 创建小球函数
/**
 * @brief 创建小球UI对象
 * @details 在屏幕中心位置创建小球圆形对象，设置初始样式和颜色
 */
static void create_ball(void)
{
    if (ball == NULL && ball_screen != NULL)
    {
        if (lvgl_port_lock(100))
        {
            // 创建小球对象，使用ball_screen作为容器
            ball = lv_obj_create(ball_screen);

            if (ball != NULL)
            {
                // 设置小球样式
                lv_obj_remove_style_all(ball);
                lv_obj_set_size(ball, BALL_RADIUS * 2, BALL_RADIUS * 2);
                // 使用正确的API设置圆角
                lv_obj_set_style_radius(ball, BALL_RADIUS, 0);

                // 设置小球颜色
                lv_obj_set_style_bg_color(ball, get_ball_color_by_mode(current_mode), 0);
                lv_obj_set_style_bg_opa(ball, 255, 0);

                // 设置小球初始位置在屏幕中心
                lv_obj_center(ball);

                ESP_LOGI(TAG, "小球创建成功");
            }
            lvgl_port_unlock();
        }
        else
        {
            ESP_LOGE(TAG, "获取LVGL锁失败");
        }
    }
}

// 删除小球函数
static void delete_ball(void)
{
    if (ball != NULL)
    {
        if (lvgl_port_lock(100))
        {
            lv_obj_del(ball);
            ball = NULL;
            ESP_LOGI(TAG, "小球已删除");
            lvgl_port_unlock();
        }
        else
        {
            ESP_LOGE(TAG, "获取LVGL锁失败");
        }
    }

    // 同时删除模式切换按钮
    delete_mode_button();
}

// 小球任务函数
/**
 * @brief 小球动画任务主函数
 * @details 实现小球动画的核心逻辑，包括：
 *          1. 初始化小球和UI元素
 *          2. 循环读取加速度传感器数据
 *          3. 根据传感器数据更新小球位置和速度
 *          4. 处理碰撞检测和反弹
 *          5. 更新小球UI显示
 *          6. 处理任务退出条件
 * @param pvParameters 任务参数（未使用）
 */
static void ball_task(void *pvParameters)
{
    ESP_LOGI(TAG, "小球任务已启动");

    // 创建小球和模式切换按钮
    create_ball();
    create_mode_button();

    // 初始位置在屏幕中心
    int16_t ball_x = 0; // 使用相对坐标系：-(SCREEN_WIDTH/2)到(SCREEN_WIDTH/2)
    int16_t ball_y = 0; // 使用相对坐标系：-(SCREEN_HEIGHT/2)到(SCREEN_HEIGHT/2)

    // 添加速度变量
    int16_t ball_vx = 0; // X轴速度
    int16_t ball_vy = 0; // Y轴速度

    // 模式特定变量
    uint32_t last_crazy_change = 0;
    uint32_t wall_stick_start_time = 0;
    bool is_stuck_to_wall = false;

    while (1)
    {
        // 读取当前时间（用于疯狂模式）
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 根据当前模式更新物理参数和运动逻辑
        switch (current_mode)
        {
        case MODE_NORMAL:
            // 正常模式
            qmi8658_fetch_angleFromAcc(&sensor_data);
            ball_vy += (int16_t)(sensor_data.AngleX * SENSITIVITY_X_NORMAL);
            ball_vx += (int16_t)(sensor_data.AngleY * SENSITIVITY_Y_NORMAL);
            ball_vx = (int16_t)(ball_vx * FRICTION_NORMAL);
            ball_vy = (int16_t)(ball_vy * FRICTION_NORMAL);
            break;

        case MODE_ANTI_GRAVITY:
            // 反重力模式：使用与正常模式相反的陀螺仪数据
            qmi8658_fetch_angleFromAcc(&sensor_data);
            // 将陀螺仪角度数据取反，实现反重力效果
            ball_vy += (int16_t)(-sensor_data.AngleX * SENSITIVITY_X_ANTI_GRAVITY);
            ball_vx += (int16_t)(-sensor_data.AngleY * SENSITIVITY_Y_ANTI_GRAVITY);
            // 使用正常模式的摩擦力
            ball_vx = (int16_t)(ball_vx * FRICTION_NORMAL);
            ball_vy = (int16_t)(ball_vy * FRICTION_NORMAL);
            break;

        case MODE_SUPER_BOUNCE:
            // 超弹力模式：普通灵敏度但超强弹性
            qmi8658_fetch_angleFromAcc(&sensor_data);
            ball_vy += (int16_t)(sensor_data.AngleX * SENSITIVITY_X_SUPER);
            ball_vx += (int16_t)(sensor_data.AngleY * SENSITIVITY_Y_SUPER);
            ball_vx = (int16_t)(ball_vx * FRICTION_SUPER);
            ball_vy = (int16_t)(ball_vy * FRICTION_SUPER);
            break;

        case MODE_STICKY:
            // 粘性模式：当小球碰到墙壁时会粘在上面
            if (!is_stuck_to_wall)
            {
                // 未粘在墙上时，正常受重力影响但灵敏度较低
                qmi8658_fetch_angleFromAcc(&sensor_data);
                ball_vy += (int16_t)(sensor_data.AngleX * SENSITIVITY_X_NORMAL * 0.5f);
                ball_vx += (int16_t)(sensor_data.AngleY * SENSITIVITY_Y_NORMAL * 0.5f);

                // 较高的摩擦力使小球容易停止
                ball_vx = (int16_t)(ball_vx * 0.8f);
                ball_vy = (int16_t)(ball_vy * 0.8f);
            }
            else
            {
                // 粘在墙上时，完全停止移动
                ball_vx = 0;
                ball_vy = 0;

                // 检查粘墙时间是否已到
                if (current_time - wall_stick_start_time > STICK_DURATION_MS)
                {
                    is_stuck_to_wall = false;
                    ESP_LOGI(TAG, "粘性模式：小球从墙上脱落");
                }
            }
            break;

        case MODE_CRAZY:
            // 疯狂模式：完全随机运动，但仍需更新传感器数据以避免切换模式时卡死
            qmi8658_fetch_angleFromAcc(&sensor_data); // 确保传感器数据始终更新
            
            if (current_time - last_crazy_change > CRAZY_CHANGE_INTERVAL)
            {
                // 完全随机生成新的速度，让小球乱跳
                ball_vx = (rand() % (2 * CRAZY_VELOCITY_RANGE)) - CRAZY_VELOCITY_RANGE;
                ball_vy = (rand() % (2 * CRAZY_VELOCITY_RANGE)) - CRAZY_VELOCITY_RANGE;
                last_crazy_change = current_time;
                ESP_LOGI(TAG, "疯狂模式：随机改变速度 VX: %d, VY: %d", ball_vx, ball_vy);
            }

            // 疯狂模式不需要陀螺仪输入，保持随机速度
            // 低摩擦使小球能够持续跳跃
            ball_vx = (int16_t)(ball_vx * 0.98f);
            ball_vy = (int16_t)(ball_vy * 0.98f);
            break;

        case MODE_COUNT:
            // MODE_COUNT仅用于计数，实际运行中不应该到达这里
            // 如果意外到达，重置为正常模式
            current_mode = MODE_NORMAL;
            ESP_LOGW(TAG, "意外进入MODE_COUNT，已重置为正常模式");
            // 执行正常模式的逻辑
            qmi8658_fetch_angleFromAcc(&sensor_data);
            ball_vy += (int16_t)(sensor_data.AngleX * SENSITIVITY_X_NORMAL);
            ball_vx += (int16_t)(sensor_data.AngleY * SENSITIVITY_Y_NORMAL);
            ball_vx = (int16_t)(ball_vx * FRICTION_NORMAL);
            ball_vy = (int16_t)(ball_vy * FRICTION_NORMAL);
            is_stuck_to_wall = false;
            break;
        }

        // 如果速度很小，直接归零（避免抖动）
        if (abs(ball_vx) < 1)
            ball_vx = 0;
        if (abs(ball_vy) < 1)
            ball_vy = 0;

        // 根据速度更新位置
        ball_x += ball_vx;
        ball_y += ball_vy;

        // 处理边界回弹
        // 保存碰撞前的速度，用于检测是否发生碰撞
        int16_t old_vx = ball_vx;
        int16_t old_vy = ball_vy;

        handle_ball_bounce(&ball_x, &ball_y, &ball_vx, &ball_vy);

        // 粘性模式特殊处理：当小球碰到墙壁时粘住
        if (current_mode == MODE_STICKY && !is_stuck_to_wall)
        {
            // 检测是否发生碰撞（速度方向改变）
            if ((old_vx > 0 && ball_vx <= 0) || (old_vx < 0 && ball_vx >= 0) ||
                (old_vy > 0 && ball_vy <= 0) || (old_vy < 0 && ball_vy >= 0))
            {
                // 发生碰撞，粘在墙上
                is_stuck_to_wall = true;
                wall_stick_start_time = current_time;
                ball_vx = 0;
                ball_vy = 0;
                ESP_LOGI(TAG, "粘性模式：小球粘在墙上，位置 X: %d, Y: %d", ball_x, ball_y);
            }
        }

        // 更新小球位置
        if (lvgl_port_lock(100))
        {
            if (ball != NULL)
            {
                // Direct use of ball_x and ball_y for LVGL positioning
                // No coordinate transformation needed as per original implementation
                int16_t lv_x = ball_x;
                int16_t lv_y = ball_y;
                lv_obj_set_pos(ball, lv_x, lv_y);
            }
            lvgl_port_unlock();
        }

        // 打印调试信息，显示当前模式名称以便识别
        const char *mode_names[MODE_COUNT] = {"Normal", "Anti-Gravity", "Super Bounce", "Sticky", "Crazy"};

        // 粘性模式额外显示粘墙状态
        if (current_mode == MODE_STICKY)
        {
            ESP_LOGI(TAG, "当前模式: %s, 角度 X: %.2f°, Y: %.2f°, 粘墙状态: %s",
                     mode_names[current_mode], sensor_data.AngleX, sensor_data.AngleY,
                     is_stuck_to_wall ? "已粘墙" : "未粘墙");
        }
        else
        {
            ESP_LOGI(TAG, "当前模式: %s, 角度 X: %.2f°, Y: %.2f°",
                     mode_names[current_mode], sensor_data.AngleX, sensor_data.AngleY);
        }
        ESP_LOGI(TAG, "小球位置 X: %d, Y: %d, 速度 VX: %d, VY: %d", ball_x, ball_y, ball_vx, ball_vy);

        // 延时50ms，控制更新频率
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    
    // 任务结束时的清理工作
    ESP_LOGI(TAG, "小球任务正在执行清理工作");
    delete_ball(); // 删除小球对象
    ball_task_handle = NULL; // 清除任务句柄
    
    ESP_LOGI(TAG, "小球任务已成功退出");
}

// 启动小球任务函数
/**
 * @brief 启动小球动画任务
 * @details 创建小球动画任务，分配任务堆栈和优先级
 * @return 无
 */
void ball_task_start(void)
{
    if (ball_task_handle == NULL)
    {
        // 初始化ball_screen
        if (ball_screen == NULL)
        {
            if (lvgl_port_lock(100))
            {
                ball_screen = lv_obj_create(NULL);
                if (ball_screen != NULL)
                {
                    // 设置ball_screen样式
                    lv_obj_remove_style_all(ball_screen);
                    lv_obj_set_size(ball_screen, SCREEN_WIDTH, SCREEN_HEIGHT);
                    lv_obj_set_style_bg_color(ball_screen, lv_color_hex(0x000000), 0); // 黑色背景
                    lv_obj_set_style_bg_opa(ball_screen, 255, 0);

                    // 加载ball_screen
                    lv_scr_load(ball_screen);
                    ESP_LOGI(TAG, "小球屏幕初始化成功");
                }
                lvgl_port_unlock();
            }
        }

        // 初始化随机数生成器（用于疯狂模式）
        srand(xTaskGetTickCount());

        // 重置为默认模式
        current_mode = MODE_NORMAL;

        // 创建小球任务
        xTaskCreate(ball_task, "ball_task", 4096, NULL, 5, &ball_task_handle);

        // 设置UI状态为小球页面
        extern ui_state_t current_ui_state;
        current_ui_state = UI_STATE_BALL;
        ESP_LOGI(TAG, "小球任务启动，UI状态设置为BALL");
    }
    else
    {
        ESP_LOGW(TAG, "小球任务已经在运行");
    }
}

// 停止小球任务函数
void ball_task_stop(void)
{
    if (ball_task_handle != NULL)
    {
        // 先删除小球对象和模式切换按钮
        delete_ball();

        // 删除任务
        vTaskDelete(ball_task_handle);
        ball_task_handle = NULL;

        ESP_LOGI(TAG, "小球任务停止，任务句柄已清除");
    }
}

// 动作函数
void action5(void)
{
    // 检查是否已经初始化了QMI8658
    // 这里假设QMI8658已经在系统初始化时被初始化

    // 切换小球任务状态
    if (ball_task_handle == NULL)
    {
        ball_task_start();
    }
    else
    {
        ball_task_stop();
    }
}