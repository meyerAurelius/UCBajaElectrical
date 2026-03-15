/**
 * @file theme_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "theme_gen.h"
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

lv_obj_t * theme_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_width(&style_base, lv_pct(100));
        lv_style_set_height(&style_base, lv_pct(100));
        lv_style_set_flex_grow(&style_base, 1);

        style_inited = true;
    }

    lv_obj_t * column_0 = column_create(parent, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_name_static(column_0, "theme_#");

    lv_obj_add_style(column_0, &style_base, 0);
    lv_obj_add_style(column_0, &style_light_bg, 0);
    lv_obj_bind_style(column_0, &style_dark_bg, 0, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return column_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

