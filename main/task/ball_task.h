#pragma once

// 函数声明
void action5(void);
void ball_task_start(void);
void ball_task_stop(void);
// Define ball movement modes
typedef enum
{
    MODE_NORMAL,       // Normal mode with standard physics
    MODE_ANTI_GRAVITY, // Anti-gravity mode (reversed gyroscope direction)
    MODE_SUPER_BOUNCE, // Super bounce mode (extremely elastic)
    MODE_STICKY,       // Sticky mode (sticks to walls)
    MODE_CRAZY,        // Crazy mode (completely random movement)
    MODE_COUNT         // Total number of modes (not a valid mode)
} ball_mode_t;
