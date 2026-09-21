#include "headfile.h"

lv_obj_t *forest_bg_img; // 森林背景图像对象

// 保持原始的图像资源声明名称不变
LV_IMG_DECLARE(screen_animimg_1PixPin_0);
LV_IMG_DECLARE(screen_animimg_1PixPin_1);
LV_IMG_DECLARE(screen_animimg_1PixPin_2);
LV_IMG_DECLARE(screen_animimg_1PixPin_3);
LV_IMG_DECLARE(screen_animimg_1PixPin_4);
LV_IMG_DECLARE(screen_animimg_1PixPin_5);
LV_IMG_DECLARE(screen_animimg_1PixPin_6);
LV_IMG_DECLARE(screen_animimg_1PixPin_7);
LV_IMG_DECLARE(screen_animimg_1PixPin_8);
LV_IMG_DECLARE(screen_animimg_1PixPin_9);
LV_IMG_DECLARE(screen_animimg_1PixPin_10);
LV_IMG_DECLARE(screen_animimg_1PixPin_11);
LV_IMG_DECLARE(screen_animimg_1PixPin_12);
LV_IMG_DECLARE(screen_animimg_1PixPin_13);
LV_IMG_DECLARE(screen_animimg_1PixPin_14);
LV_IMG_DECLARE(screen_animimg_1PixPin_15);
LV_IMG_DECLARE(screen_animimg_1PixPin_16);
LV_IMG_DECLARE(screen_animimg_1PixPin_17);
LV_IMG_DECLARE(screen_animimg_1PixPin_18);
LV_IMG_DECLARE(screen_animimg_1PixPin_19);
LV_IMG_DECLARE(screen_animimg_1PixPin_20);
LV_IMG_DECLARE(screen_animimg_1PixPin_21);
LV_IMG_DECLARE(screen_animimg_1PixPin_22);
LV_IMG_DECLARE(screen_animimg_1PixPin_23);
LV_IMG_DECLARE(screen_animimg_1PixPin_24);
LV_IMG_DECLARE(screen_animimg_1PixPin_25);
LV_IMG_DECLARE(screen_animimg_1PixPin_26);
LV_IMG_DECLARE(screen_animimg_1PixPin_27);
LV_IMG_DECLARE(screen_animimg_1PixPin_28);
LV_IMG_DECLARE(screen_animimg_1PixPin_29);
LV_IMG_DECLARE(screen_animimg_1PixPin_30);
LV_IMG_DECLARE(screen_animimg_1PixPin_31);
LV_IMG_DECLARE(screen_animimg_1PixPin_32);
LV_IMG_DECLARE(screen_animimg_1PixPin_33);
LV_IMG_DECLARE(screen_animimg_1PixPin_34);
LV_IMG_DECLARE(tree); // 森林背景

// 创建小狗动画帧数组（使用更直观的名称）
const lv_img_dsc_t *dog_animation_frames[35] = {
    &screen_animimg_1PixPin_0,
    &screen_animimg_1PixPin_1,
    &screen_animimg_1PixPin_2,
    &screen_animimg_1PixPin_3,
    &screen_animimg_1PixPin_4,
    &screen_animimg_1PixPin_5,
    &screen_animimg_1PixPin_6,
    &screen_animimg_1PixPin_7,
    &screen_animimg_1PixPin_8,
    &screen_animimg_1PixPin_9,
    &screen_animimg_1PixPin_10,
    &screen_animimg_1PixPin_11,
    &screen_animimg_1PixPin_12,
    &screen_animimg_1PixPin_13,
    &screen_animimg_1PixPin_14,
    &screen_animimg_1PixPin_15,
    &screen_animimg_1PixPin_16,
    &screen_animimg_1PixPin_17,
    &screen_animimg_1PixPin_18,
    &screen_animimg_1PixPin_19,
    &screen_animimg_1PixPin_20,
    &screen_animimg_1PixPin_21,
    &screen_animimg_1PixPin_22,
    &screen_animimg_1PixPin_23,
    &screen_animimg_1PixPin_24,
    &screen_animimg_1PixPin_25,
    &screen_animimg_1PixPin_26,
    &screen_animimg_1PixPin_27,
    &screen_animimg_1PixPin_28,
    &screen_animimg_1PixPin_29,
    &screen_animimg_1PixPin_30,
    &screen_animimg_1PixPin_31,
    &screen_animimg_1PixPin_32,
    &screen_animimg_1PixPin_33,
    &screen_animimg_1PixPin_34,
};

// 创建森林背景的函数
void create_forest_background(void)
{
    // 1. 创建一个屏幕对象
    lv_obj_t *main_screen = lv_obj_create(NULL);
    lv_scr_load(main_screen); // 加载屏幕

    // 2. 创建图像控件作为森林背景
    forest_bg_img = lv_img_create(main_screen);

    // 3. 设置图像源为森林背景（保持原始名称）
    lv_img_set_src(forest_bg_img, &tree);

    // 4. 设置图像位置和大小以覆盖整个屏幕
    lv_obj_set_pos(forest_bg_img, 0, 0);
    lv_obj_set_size(forest_bg_img, LV_PCT(100), LV_PCT(100));

    // 5. 确保森林背景位于最底层
    lv_obj_move_background(forest_bg_img);
}

// 创建小狗穿过森林动画的函数
void create_dog_walking_animation(void)
{
    // 1. 创建一个透明容器对象，用于承载小狗动画和控制移动
    lv_obj_t *dog_container = lv_obj_create(forest_bg_img);

    // 设置容器大小和位置
    lv_obj_set_size(dog_container, 108, 105); // 容器大小
    lv_obj_set_pos(dog_container, 0, 130);    // 初始位置（y=130表示小狗在地面上）

    // 设置容器为透明，不可见
    lv_obj_set_style_bg_opa(dog_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_width(dog_container, 0, 0);
    lv_obj_set_style_border_width(dog_container, 0, 0);
    lv_obj_set_style_border_opa(dog_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_side(dog_container, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_scrollbar_mode(dog_container, LV_SCROLLBAR_MODE_OFF);

    // 2. 创建小狗动画图像控件
    lv_obj_t *dog_animation = lv_animimg_create(dog_container);

    // 设置小狗动画为不透明
    lv_obj_set_style_bg_opa(dog_animation, LV_OPA_COVER, 0);

    // 3. 设置小狗动画帧源
    lv_animimg_set_src(dog_animation, (const void **)dog_animation_frames, DOG_ANIMATION_FRAME_COUNT);

    // 4. 设置小狗动画持续时间（整个动画序列的时间，单位：毫秒）
    // 例如：每帧显示100ms，35帧总共3500ms
    lv_animimg_set_duration(dog_animation, 100 * DOG_ANIMATION_FRAME_COUNT);

    // 5. 设置小狗动画重复次数
    // LV_ANIM_REPEAT_INFINITE 表示无限循环
    lv_animimg_set_repeat_count(dog_animation, LV_ANIM_REPEAT_INFINITE);

    // 6. 启动小狗动画
    lv_animimg_start(dog_animation);

    // 7. 设置小狗动画控件的位置和大小
    lv_obj_set_pos(dog_animation, 0, 0);
    lv_obj_set_size(dog_animation, 100, 100);

    // 可选：设置小狗动画控件的样式
    lv_obj_set_style_bg_color(dog_animation, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dog_animation, LV_OPA_COVER, 0);

    // 8. 创建小狗容器（包含小狗动画）的移动动画，模拟小狗穿过森林
    lv_anim_t dog_walk_animation;
    lv_anim_init(&dog_walk_animation);                                          // 初始化动画
    lv_anim_set_var(&dog_walk_animation, dog_container);                        // 设置动画的目标对象为小狗容器
    lv_anim_set_exec_cb(&dog_walk_animation, (lv_anim_exec_xcb_t)lv_obj_set_x); // 设置动画执行的回调函数，改变x坐标
    lv_anim_set_values(&dog_walk_animation, -100, 320);                         // 设置动画的起始值和结束值，从左边-100移动到右边320
    lv_anim_set_time(&dog_walk_animation, 4000);                                // 设置动画的持续时间为4000毫秒
    lv_anim_start(&dog_walk_animation);                                         // 启动动画
}
void loading_anim(void)
{
    create_forest_background();
    create_dog_walking_animation();
}
// 删除加载动画的函数
void delete_loading_anim(void)
{
    lv_obj_del(forest_bg_img); // 删除开机logo

    // 等待一小段时间，确保LVGL完全处理完删除操作
    vTaskDelay(pdMS_TO_TICKS(50));
}
