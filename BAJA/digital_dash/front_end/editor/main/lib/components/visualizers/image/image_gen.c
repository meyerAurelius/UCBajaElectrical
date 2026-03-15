/**
 * @file image_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "image_gen.h"
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

lv_obj_t * image_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_bar;
    static lv_style_t style_bar_indicator;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_bar);
        lv_style_set_height(&style_bar, 27);
        lv_style_set_bg_color(&style_bar, lv_color_hex(0xffffff));
        lv_style_set_bg_opa(&style_bar, 30);
        lv_style_set_radius(&style_bar, 9);

        lv_style_init(&style_bar_indicator);
        lv_style_set_bg_color(&style_bar_indicator, lv_color_hex(0xffffff));
        lv_style_set_bg_opa(&style_bar_indicator, 255);
        lv_style_set_radius(&style_bar_indicator, 9);

        style_inited = true;
    }

    lv_obj_t * lv_bar_0 = lv_bar_create(parent);
    lv_obj_set_name_static(lv_bar_0, "image_#");

    lv_obj_add_style(lv_bar_0, &style_bar, 0);
    lv_obj_add_style(lv_bar_0, &style_bar_indicator, LV_PART_INDICATOR);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_bar_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

