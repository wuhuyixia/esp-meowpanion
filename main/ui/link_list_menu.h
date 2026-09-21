/**
 * @file 滚动链表菜单.h
 * 包含滚动链表菜单相关函数和定义的头文件
 */

#ifndef GUNDONLIANBIAO_H
#define GUNDONLIANBIAO_H

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include "defines.h" // 包含统一的宏定义文件

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      DEFINES
 *********************/



    /**********************
     *      TYPEDEFS
     **********************/

    /* 菜单项结构体定义 */
    typedef struct ScrollingMenuItem
    {
        char name[32];                  /* 菜单项名称 */
        int MenuItemGrade;              /* 菜单项等级 */
        struct ScrollingMenuItem *back; /* 指向上一级菜单的指针 */
        struct ScrollingMenuItem *next; /* 下一个菜单项的指针 */
        struct ScrollingMenuItem *sub;  /* 子菜单的指针 */
        void (*action)();               /* 菜单项活动回调函数 */
        lv_obj_t *lv_obj;               /* LVGL对象指针 */
        int32_t x;                      /* x坐标，用于滚动效果 */
        const char *icon;               /* 菜单项图标 */
    } ScrollingMenuItem;

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    extern ScrollingMenuItem *menu_head;
    /**
     * @brief 创建菜单项
     * @param name 菜单项名称
     * @param action 菜单项活动回调函数
     * @param MenuItemGrade 菜单项等级
     * @param backMenu 指向上一级菜单的指针
     * @return 创建的菜单项指针
     */
    ScrollingMenuItem *gdlb_create_item(char *name, void (*action)(), int MenuItemGrade, ScrollingMenuItem *backMenu);

    /**
     * @brief 创建LVGL菜单系统
     * @param menu_root 菜单根节点
     */
    void gdlb_create_menu(ScrollingMenuItem *menu_root);

    /**
     * @brief 操作菜单系统
     * @param menu_root 当前菜单
     */
    void gdlb_operate_menu(ScrollingMenuItem *menu_root);

    /**
     * @brief 释放菜单内存
     * @param menu_root 菜单根节点
     */
    void gdlb_free_menu(ScrollingMenuItem *menu_root);

    /**
     * @brief 设置循环滚动
     * @param enable 是否启用循环滚动
     */
    void gdlb_set_circular_scroll(bool enable);

    /**********************
     *      MACROS
     **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SCROLLING_LINKED_LIST_MENU_H*/
