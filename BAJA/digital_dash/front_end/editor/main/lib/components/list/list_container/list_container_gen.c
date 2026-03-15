/**
 * @file list_container_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "list_container_gen.h"
#include "../../../starter.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * list_container_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;
    static lv_style_t style_base_dark;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_height(&style_base, LV_SIZE_CONTENT);
        lv_style_set_width(&style_base, lv_pct(100));
        lv_style_set_bg_color(&style_base, COLOR_LIGHT_PANEL);
        lv_style_set_bg_opa(&style_base, (255 * 100 / 100));
        lv_style_set_radius(&style_base, BASE_LIB_RADIUS);
        lv_style_set_pad_all(&style_base, 0);
        lv_style_set_pad_row(&style_base, 0);
        lv_style_set_flex_grow(&style_base, 1);

        lv_style_init(&style_base_dark);
        lv_style_set_bg_color(&style_base_dark, COLOR_DARK_PANEL);

        style_inited = true;
    }

    lv_obj_t * column_0 = column_create(parent, BASE_LIB_UNIT_MD, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_name_static(column_0, "list_container_#");

    lv_obj_add_style(column_0, &style_base, 0);
    lv_obj_bind_style(column_0, &style_base_dark, 0, &subject_dark_mode, 1);
    lv_obj_add_style(column_0, &style_light_scrollbar, LV_PART_SCROLLBAR);

    LV_TRACE_OBJ_CREATE("finished");

    return column_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

