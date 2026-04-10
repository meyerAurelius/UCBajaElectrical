/**
 * @file slider_box_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "slider_box_gen.h"
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

lv_obj_t * slider_box_create(lv_obj_t * parent, const char * title, lv_subject_t * subject, const char * format)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_width(&style_base, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_base, BASE_LIB_BLOCK_1_5);

        style_inited = true;
    }

    lv_obj_t * panel_0 = panel_create(parent, BASE_LIB_UNIT_LG);
    lv_obj_set_name_static(panel_0, "slider_box_#");

    lv_obj_add_style(panel_0, &style_base, 0);
    lv_obj_t * column_0 = column_create(panel_0, BASE_LIB_UNIT_MD, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_width(column_0, lv_pct(100));
    lv_obj_set_height(column_0, lv_pct(100));
    lv_obj_set_style_flex_main_place(column_0, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    h3_create(column_0, title);
    
    lv_obj_t * value_setter_0 = value_setter_create(column_0, subject, format);
    lv_obj_set_width(value_setter_0, lv_pct(100));
    
    lv_obj_t * slider_0 = slider_create(column_0, subject);
    lv_obj_set_width(slider_0, lv_pct(100));

    LV_TRACE_OBJ_CREATE("finished");

    return panel_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

