#pragma once

// 注意：系统头文件已在headfile.h中统一包含

// 声明外部图像对象
extern lv_obj_t *forest_bg_img;

// 声明动画帧数组
extern const lv_img_dsc_t *dog_animation_frames[35];

// 函数声明
void create_forest_background(void);
void create_dog_walking_animation(void);
void loading_anim(void);
void delete_loading_anim(void);