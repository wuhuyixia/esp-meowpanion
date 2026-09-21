/**
 * @file link_list_menu.c
 * @brief 滚动链表菜单功能实现
 * @details 本文件实现了ESP32-S3系统的滚动链表菜单UI，主要包括：
 *          1. 菜单项创建与管理
 *          2. 触摸滑动与动画效果
 *          3. 菜单项点击事件处理
 *          4. 多级菜单导航与返回
 *          5. 菜单界面布局与样式控制
 */

/*********************
 *      INCLUDES
 *********************/

#include "headfile.h"
#if 1

/* LVGL菜单容器 */
static lv_obj_t *menu_container;

/* 当前活动菜单 */
ScrollingMenuItem *current_menu = NULL;
ScrollingMenuItem *menu_head = NULL;

/* 触摸相关变量 */
static bool touched = false; /* 是否正在触摸 */
static int32_t t_offset_x;   /* 触摸偏移量x */
static int32_t scr_w;        /* 屏幕宽度 */
/* 拖动状态 */
static bool isdragged = false;    /* 是否正在拖动 */
static int32_t drag_distance = 0; /* 累计拖动距离 */
static bool has_moved = false;    /* 是否发生过移动 */

/* 循环滚动设置 */
static bool circular_scroll_enabled = true; /* 是否启用循环滚动 */

/* 函数声明 */
static void menu_item_click_cb(lv_event_t *e);
static void back_btn_click_cb(lv_event_t *e);
static void pressing_cb(lv_event_t *e);
static void released_cb(lv_event_t *e);
static void set_x_cb(void *var, int32_t v);
static void lv_myanim_creat(void *var, lv_anim_exec_xcb_t exec_cb, uint32_t time, uint32_t delay,
                            lv_anim_path_cb_t path_cb, int32_t start, int32_t end,
                            lv_anim_ready_cb_t completed_cb, lv_anim_deleted_cb_t deleted_cb);
static void update_item_size(lv_obj_t *item_obj, int32_t x_pos);
static void clear_menu_container(void);

/* 创建菜单项函数 */
ScrollingMenuItem *gdlb_create_item(char *name, void (*action)(), int MenuItemGrade, ScrollingMenuItem *backMenu)
{
    /* 使用 malloc 分配内存，创建一个新的菜单项，所需内存大小为 sizeof(ScrollingMenuItem) */
    /* 将新创建的菜单项结构体指针赋值给临时变量 temp */
    ScrollingMenuItem *temp = (ScrollingMenuItem *)malloc(sizeof(ScrollingMenuItem));

    if (temp != NULL)
    {
        temp->MenuItemGrade = MenuItemGrade;
        temp->back = backMenu;
        strcpy(temp->name, name);
        temp->next = NULL; /* 下一个菜单项指针将在 main() 函数中设置 */
        temp->sub = NULL;  /* 子菜单指针将在 main() 函数中设置 */
        temp->action = action;
        temp->lv_obj = NULL; /* LVGL对象指针初始化为 NULL */
        temp->x = 0;         /* 初始x坐标设置为 0 */
        temp->icon = NULL;   /* 图标指针初始化为 NULL */
    }

    return temp;
}

/* 根据位置更新菜单项大小 */
static void update_item_size(lv_obj_t *item_obj, int32_t x_pos)
{
    // 查找标签对象
    lv_obj_t *label = NULL;
    uint32_t child_cnt = lv_obj_get_child_cnt(item_obj);

    // 遍历所有子对象，找到标签对象
    for (uint32_t i = 0; i < child_cnt; i++)
    {
        lv_obj_t *child = lv_obj_get_child(item_obj, i);
        if (lv_obj_check_type(child, &lv_label_class))
        {
            label = child;
            break;
        }
    }

    // 如果没有找到标签对象，则直接返回
    if (label == NULL)
    {
        return;
    }

    if (lvgl_port_lock(100))
    {
        // 如果x位置为0，表示在中心位置，使用大字体
        if (x_pos == 0)
        {
           
            lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0); // 设置为大字体
            lvgl_port_unlock();
            return;
        }

        // 如果x位置超出图标距离，使用小字体
        if (x_pos >= ICON_DISTANCE || x_pos <= -ICON_DISTANCE)
        {
           
            lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0); // 设置为小字体
            lvgl_port_unlock();
            return;
        }

        // 如果x位置为负数，取其绝对值
        if (x_pos < 0)
            x_pos = -x_pos;

        // 计算新的尺寸
        int32_t new_size = ICON_SIZE_SMALL + (float)(ICON_DISTANCE - x_pos) / (float)ICON_DISTANCE * (ICON_SIZE_BIG - ICON_SIZE_SMALL);
        //  lv_obj_set_size(item_obj, new_size, new_size);

        // 根据新尺寸选择字体
        if (new_size > (ICON_SIZE_SMALL + ICON_SIZE_BIG) / 2)
        {
            // 如果尺寸大于中间值，使用大字体
            lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
        }
        else
        {
            // 否则使用小字体
            lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
        }

        lvgl_port_unlock();
    }
}

/**
 * @brief 菜单项点击回调函数
 * @details 处理菜单项点击事件，根据菜单项类型执行相应操作
 *          1. 首先判断是否为拖动操作，若是则不触发点击事件
 *          2. 重置拖动状态变量
 *          3. 找到被点击的菜单项，并验证其有效性
 *          4. 检查菜单项是否在中心位置（可接受误差范围为±10像素）
 *          5. 根据菜单项属性执行不同操作：
 *             - 若有子菜单，切换到子菜单
 *             - 若有动作函数，执行相应动作
 * @param e LVGL事件对象，包含触发事件的目标对象等信息
 * @return 无返回值
 */
static void menu_item_click_cb(lv_event_t *e)
{
    // 改进的点击判断逻辑：如果发生了拖动或累计拖动距离超过阈值，不触发点击
    if (isdragged || drag_distance > 10) // 小阈值用于检测任何微小移动
    {
        // 重置拖动状态
        isdragged = false;
        drag_distance = 0;
        return;
    }

    lv_obj_t *btn = lv_event_get_target(e);
    ScrollingMenuItem *item = current_menu;

    // 检查按钮和当前菜单是否有效
    if (btn == NULL || item == NULL || menu_container == NULL)
    {
        return;
    }

    // 重置拖动相关状态
    isdragged = false;
    drag_distance = 0;
    has_moved = false;

    // 遍历当前菜单项
    while (item != NULL)
    {
        // 检查菜单项对象是否仍然有效
        if (item->lv_obj == NULL)
        {
            item = item->next;
            continue;
        }

        if (item->lv_obj == btn)
        {
            // 检查菜单项是否位于中心位置（x坐标接近0）
            // 允许一定的误差范围，比如10像素
            if (item->x >= -10 && item->x <= 10)
            {
                // 如果有子菜单，则进入子菜单
                if (item->sub != NULL)
                {
                    // 重置拖动状态
                    isdragged = false;
                    gdlb_operate_menu(item->sub);
                }
                // 如果没有子菜单但有回调函数，则执行回调函数
                else if (item->action != NULL)
                {
                    // 重置拖动状态
                    isdragged = false;
                    item->action();
                }
            }
            break;
        }
        item = item->next;
    }
}

/* 返回按钮点击回调函数 */
/* 处理返回按钮点击事件，返回上一级菜单 */
static void back_btn_click_cb(lv_event_t *e)
{
    // 如果是拖动操作，不触发点击事件
    if (isdragged)
    {
        return;
    }

    // 重置拖动状态
    isdragged = false;

    // 如果当前菜单不是根菜单且有上级菜单，则返回上级菜单
    if (current_menu != NULL && current_menu->back != NULL && menu_container != NULL)
    {
        gdlb_operate_menu(current_menu->back);
    }
}
/* 触摸按下回调函数 */
static void pressing_cb(lv_event_t *e)
{
    static lv_point_t click_point1, click_point2;
    ScrollingMenuItem *temp;
    lv_indev_t *indev = lv_indev_get_act();

    // 如果是第一次触摸
    if (touched == false)
    {
        if (lvgl_port_lock(100))
        {
            temp = current_menu;
            while (temp != NULL)
            {
                // 停止所有菜单项的动画
                if (temp->lv_obj != NULL)
                {
                    lv_anim_del(temp->lv_obj, set_x_cb);
                }
                temp = temp->next;
            }
            lvgl_port_unlock();
        }

        // 获取第一次触摸点的位置
        if (indev != NULL)
        {
            lv_indev_get_point(indev, &click_point1);
        }
        // 标记为正在触摸并重置状态
        touched = true;
        isdragged = false;
        drag_distance = 0;
        has_moved = false;
        return;
    }
    else
    {
        // 获取当前触摸点的位置
        if (indev != NULL)
        {
            lv_indev_get_point(indev, &click_point2);
        }
    }

    // 计算触摸偏移量x
    t_offset_x = click_point2.x - click_point1.x;
    // 累计拖动距离
    drag_distance += abs(t_offset_x);

    // 更新触摸点位置
    click_point1.x = click_point2.x;

    // 如果有任何移动，标记为已移动
    if (abs(t_offset_x) > 0)
    {
        has_moved = true;
    }

    // 提高拖动阈值，减少误触发 (从20像素增加到30像素)
    if (drag_distance > 30)
    {
        isdragged = true;
    }

    // 计算菜单项总数
    int32_t item_count = 0;
    ScrollingMenuItem *count_temp = current_menu;
    while (count_temp != NULL)
    {
        item_count++;
        count_temp = count_temp->next;
    }

    if (lvgl_port_lock(100))
    {
        temp = current_menu;
        while (temp != NULL)
        {
            if (temp->lv_obj != NULL)
            {
                // 更新菜单项的x位置
                temp->x += t_offset_x;

                // 确保x位置在合理范围内，实现循环滚动效果
                while (temp->x < (-item_count / 2) * ICON_DISTANCE)
                {
                    temp->x += (item_count)*ICON_DISTANCE;
                }
                while (temp->x > (item_count / 2) * ICON_DISTANCE)
                {
                    temp->x -= (item_count)*ICON_DISTANCE;
                }

                // 设置菜单项对象的x位置
                lv_obj_set_x(temp->lv_obj, temp->x);

                // 更新菜单项的大小
                update_item_size(temp->lv_obj, temp->x);
            }
            temp = temp->next;
        }
        lvgl_port_unlock();
    }
}

/* 触摸释放回调函数 */
static void released_cb(lv_event_t *e)
{
    int32_t offset_x;
    offset_x = 0;
    // 标记触摸结束
    touched = false;
    // 不在这里重置isdragged，让点击回调函数使用它进行判断
    // 在动画完成后再重置拖动状态
    // 计算菜单项总数
    int32_t item_count = 0;
    ScrollingMenuItem *count_temp = current_menu;
    while (count_temp != NULL)
    {
        item_count++;
        count_temp = count_temp->next;
    }

    // 找到第一个x位置大于0的菜单项
    ScrollingMenuItem *temp = current_menu;
    while (temp != NULL)
    {
        if (temp->lv_obj != NULL && temp->x > 0)
        {
            // 根据x位置计算偏移量，实现吸附效果
            if (temp->x % ICON_DISTANCE > ICON_DISTANCE / 2)
            {
                offset_x = ICON_DISTANCE - temp->x % ICON_DISTANCE;
            }
            else
            {
                offset_x = -temp->x % ICON_DISTANCE;
            }
            break;
        }
        temp = temp->next;
    }

    if (lvgl_port_lock(100))
    {
        // 为所有菜单项应用动画
        temp = current_menu;
        while (temp != NULL)
        {
            if (temp->lv_obj != NULL)
            {
                // 创建动画，实现平滑滚动效果
                lv_myanim_creat(temp->lv_obj, set_x_cb, t_offset_x > 0 ? 300 + t_offset_x * 5 : 300 - t_offset_x * 5, 0,
                                lv_anim_path_ease_out, temp->x, temp->x + offset_x + t_offset_x / 20 * ICON_DISTANCE, 0, 0);
                // 更新菜单项的x位置
                temp->x += offset_x + t_offset_x / 20 * ICON_DISTANCE;
                // 确保x位置在合理范围内，实现循环滚动效果
                while (temp->x < (-item_count / 2) * ICON_DISTANCE)
                {
                    temp->x += (item_count)*ICON_DISTANCE;
                }
                while (temp->x > (item_count / 2) * ICON_DISTANCE)
                {
                    temp->x -= (item_count)*ICON_DISTANCE;
                }
            }
            temp = temp->next;
        }
        lvgl_port_unlock();
    }
}

/* 创建自定义动画函数 */
static void lv_myanim_creat(void *var, lv_anim_exec_xcb_t exec_cb, uint32_t time, uint32_t delay,
                            lv_anim_path_cb_t path_cb, int32_t start, int32_t end,
                            lv_anim_ready_cb_t completed_cb, lv_anim_deleted_cb_t deleted_cb)
{
    lv_anim_t xxx;
    // 初始化动画结构体
    lv_anim_init(&xxx);
    // 设置动画的目标对象
    lv_anim_set_var(&xxx, var);
    // 设置动画的执行回调函数
    lv_anim_set_exec_cb(&xxx, exec_cb);
    // 设置动画的持续时间
    lv_anim_set_time(&xxx, time);
    // 设置动画的延迟时间
    lv_anim_set_delay(&xxx, delay);
    // 设置动画的起始和结束值
    lv_anim_set_values(&xxx, start, end);
    // 如果提供了路径回调函数，则设置动画的路径
    if (path_cb)
        lv_anim_set_path_cb(&xxx, path_cb);
    // 如果提供了完成回调函数，则设置动画完成时的回调
    if (completed_cb)
        lv_anim_set_ready_cb(&xxx, completed_cb);
    // 如果提供了删除回调函数，则设置动画删除时的回调
    if (deleted_cb)
        lv_anim_set_deleted_cb(&xxx, deleted_cb);
    // 启动动画
    lv_anim_start(&xxx);
}

/* 设置x坐标的回调函数 */
static void set_x_cb(void *var, int32_t v)
{
    // 计算菜单项总数
    int32_t item_count = 0;
    ScrollingMenuItem *count_temp = current_menu;
    while (count_temp != NULL)
    {
        item_count++;
        count_temp = count_temp->next;
    }

    // 确保x位置在合理范围内，实现循环滚动效果
    while (v < (-item_count / 2) * ICON_DISTANCE)
    {
        v += (item_count)*ICON_DISTANCE;
    }
    while (v > (item_count / 2) * ICON_DISTANCE)
    {
        v -= (item_count)*ICON_DISTANCE;
    }

    if (lvgl_port_lock(100))
    {
        // 设置对象的x坐标
        lv_obj_set_x(var, v);

        // 更新对应菜单项的x坐标
        ScrollingMenuItem *temp = current_menu;
        while (temp != NULL)
        {
            if (temp->lv_obj == var)
            {
                temp->x = v;
                break;
            }
            temp = temp->next;
        }

        // 更新菜单项的大小
        update_item_size(var, v);

        lvgl_port_unlock();
    }
}

/* 操作LVGL菜单系统 */
void gdlb_operate_menu(ScrollingMenuItem *menu_root)
{
    ScrollingMenuItem *temp = menu_root;
    int32_t item_count = 0;
    int32_t i = 0;

    // 计算菜单项总数
    while (temp != NULL)
    {
        item_count++;
        temp = temp->next;
    }

    // 设置当前菜单
    current_menu = menu_root;

    // 清空菜单容器中的内容
    clear_menu_container();

    if (lvgl_port_lock(100))
    {
        // 如果不是根菜单，则创建返回按钮
        if (menu_root->back != NULL)
        {
            // 创建返回按钮，并设置样式和回调
            lv_obj_t *back_btn = lv_btn_create(lv_scr_act());
            if (back_btn != NULL)
            {
                lv_obj_set_size(back_btn, ICON_SIZE_SMALL, ICON_SIZE_SMALL_HEIGHT);
                lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 0, 10);

                lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x222222), 0);
                lv_obj_set_style_text_color(back_btn, lv_color_hex(0xAE8EE8), 0);
                lv_obj_set_style_radius(back_btn, ICON_SIZE_SMALL / 2, 0);
                lv_obj_set_style_border_side(back_btn, LV_BORDER_SIDE_NONE, 0);
                lv_obj_set_style_border_width(back_btn, 0, 0); // 添加这行来隐藏边框
                lv_obj_set_style_shadow_width(back_btn, 0, 0);
                lv_obj_t *back_label = lv_label_create(back_btn);
                if (back_label != NULL)
                {
                    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
                    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_48, 0); // 设置为大字体
                    lv_obj_center(back_label);
                }
                lv_obj_add_event_cb(back_btn, back_btn_click_cb, LV_EVENT_CLICKED, 0);
            }
        }

        // 为每个菜单项创建LVGL对象
        temp = menu_root;
        while (temp != NULL)
        {
            // 创建菜单项按钮
            lv_obj_t *item_btn = lv_btn_create(menu_container);
            if (item_btn == NULL)
            {
                temp = temp->next;
                continue; // 如果创建失败，跳过当前菜单项
            }

            // 计算初始x位置
            int32_t initial_x = (i - item_count / 2) * ICON_DISTANCE;

            // 设置菜单项按钮的样式和位置
            lv_obj_set_size(item_btn, ICON_SIZE_BIG, ICON_SIZE_BIG_HEIGHT);
            lv_obj_center(item_btn);           // 先居中
            lv_obj_set_x(item_btn, initial_x); // 再设置x位置
            lv_obj_set_style_bg_color(item_btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(item_btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(item_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(item_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

            // 创建菜单项标签
            lv_obj_t *item_label = lv_label_create(item_btn);
            if (item_label != NULL)
            {
                // 设置标签文本
                if (temp->icon != NULL)
                {
                    lv_label_set_text(item_label, temp->icon);
                }
                else
                {
                    lv_label_set_text(item_label, temp->name);
                }
                lv_obj_center(item_label);
                lv_obj_set_style_text_color(item_label, lv_color_hex(0xAE8EE8), 0);
                // 根据初始位置设置字体大小
                if (initial_x == 0)
                {
                    lv_obj_set_style_text_font(item_label, &lv_font_montserrat_48, 0);
                }
                else if (initial_x >= ICON_DISTANCE || initial_x <= -ICON_DISTANCE)
                {
                    lv_obj_set_style_text_font(item_label, &lv_font_montserrat_32, 0);
                }
                else
                {
                    // 计算绝对位置
                    int32_t abs_x = initial_x;
                    if (abs_x < 0)
                        abs_x = -abs_x;

                    int32_t new_size = ICON_SIZE_SMALL + (float)(ICON_DISTANCE - abs_x) / (float)ICON_DISTANCE * (ICON_SIZE_BIG - ICON_SIZE_SMALL);

                    if (new_size > (ICON_SIZE_SMALL + ICON_SIZE_BIG) / 2)
                    {
                        lv_obj_set_style_text_font(item_label, &lv_font_montserrat_48, 0);
                    }
                    else
                    {
                        lv_obj_set_style_text_font(item_label, &lv_font_montserrat_32, 0);
                    }
                }
            }

            // 保存菜单项对象和位置
            temp->lv_obj = item_btn;
            temp->x = initial_x;

            // 添加事件回调
            lv_obj_add_event_cb(item_btn, menu_item_click_cb, LV_EVENT_CLICKED, 0);
            // 添加触摸按下事件回调
            lv_obj_add_event_cb(item_btn, pressing_cb, LV_EVENT_PRESSING, 0);
            // 添加触摸释放事件回调
            lv_obj_add_event_cb(item_btn, released_cb, LV_EVENT_RELEASED, 0);

            // 增加计数器
            i++;

            // 移动到下一个菜单项
            temp = temp->next;
        }

        lvgl_port_unlock();
    }
}

/* 释放菜单内存 */
void gdlb_free_menu(ScrollingMenuItem *menu_root)
{
    ScrollingMenuItem *current = menu_root;
    while (current != NULL)
    {
        ScrollingMenuItem *temp = current;

        // 如果有子菜单，递归释放子菜单
        if (temp->sub != NULL)
        {
            gdlb_free_menu(temp->sub);
        }

        // 删除LVGL对象
        if (temp->lv_obj != NULL)
        {
            lv_obj_del(temp->lv_obj);
        }

        current = current->next;
        free(temp);
    }

    // 删除菜单容器
    if (menu_container != NULL)
    {
        lv_obj_del(menu_container);
        menu_container = NULL;
    }
}

#endif

/* 创建LVGL菜单系统 */
void gdlb_create_menu(ScrollingMenuItem *menu_root)
{
    if (lvgl_port_lock(100))
    {
        /* 获取屏幕宽度 */
        scr_w = lv_disp_get_hor_res(lv_disp_get_default());

        /* 创建菜单容器 */
        menu_container = lv_obj_create(menu_screen);
        if (menu_container == NULL)
        {
            lvgl_port_unlock();
            return; // 如果创建失败，直接返回
        }
        lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(menu_container, scr_w, lv_disp_get_ver_res(NULL));
        lv_obj_center(menu_container);
        lv_obj_set_style_bg_color(menu_container, lv_color_hex(0x222222), 0);
        lv_obj_set_style_border_width(menu_container, 0, 0);
        lv_obj_set_style_pad_all(menu_container, 10, 0);
        lv_obj_set_style_radius(menu_container, 0, 0);

        lvgl_port_unlock();
    }
}

/* 为菜单容器添加触摸按下事件回调 */
// lv_obj_add_event_cb(menu_container, pressing_cb, LV_EVENT_PRESSING, 0);
/* 为菜单容器添加触摸释放事件回调 */
//  lv_obj_add_event_cb(menu_container, released_cb, LV_EVENT_RELEASED, 0);
//

/* ???????????��????��??? */
/* 清空菜单容器 */
static void clear_menu_container(void)
{
    if (menu_container == NULL)
        return;

    if (lvgl_port_lock(100))
    {
        /* 获取当前活动屏幕 */
        lv_obj_t *scr = lv_scr_act();
        if (scr == NULL)
        {
            lvgl_port_unlock();
            return;
        }

        /* 查找并删除返回按钮 */
        uint32_t scr_child_cnt = lv_obj_get_child_cnt(scr);
        for (int i = scr_child_cnt - 1; i >= 0; i--)
        {
            lv_obj_t *child = lv_obj_get_child(scr, i);
            if (child == NULL)
                continue;

            /* 检查是否是返回按钮 */
            if (lv_obj_get_x(child) == 0 && lv_obj_get_y(child) == 10 &&
                lv_obj_get_width(child) == ICON_SIZE_SMALL &&
                lv_obj_get_height(child) == ICON_SIZE_SMALL_HEIGHT)
            {
                // 先移除事件回调，防止删除时触发事件
                lv_obj_remove_event_cb(child, back_btn_click_cb);
                lv_obj_del(child);
                break;
            }
        }

        /* 清空菜单容器中的所有子对象 */
        uint32_t child_cnt = lv_obj_get_child_cnt(menu_container);
        for (int i = child_cnt - 1; i >= 0; i--)
        {
            lv_obj_t *child = lv_obj_get_child(menu_container, i);
            if (child == NULL)
                continue;

            // 先移除所有事件回调，防止删除时触发事件
            lv_obj_remove_event_cb(child, menu_item_click_cb);
            lv_obj_remove_event_cb(child, pressing_cb);
            lv_obj_remove_event_cb(child, released_cb);

            // 删除子对象
            lv_obj_del(child);
        }

        lvgl_port_unlock();
    }
}
/* ??????????????? */
void gdlb_set_circular_scroll(bool enable)
{
    circular_scroll_enabled = enable;
}
