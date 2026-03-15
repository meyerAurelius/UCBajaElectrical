/**
 * @file card_empty_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "card_empty_gen.h"
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

lv_obj_t * card_empty_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_width(&style_base, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_base, BASE_LIB_BLOCK_1);
        lv_style_set_radius(&style_base, BASE_LIB_RADIUS);
        lv_style_set_pad_all(&style_base, BASE_LIB_UNIT_XL);
        lv_style_set_pad_row(&style_base, BASE_LIB_UNIT_MD);
        lv_style_set_pad_column(&style_base, BASE_LIB_UNIT_MD);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_set_name_static(lv_obj_0, "card_empty_#");

    lv_obj_remove_style_all(lv_obj_0);
    lv_obj_add_style(lv_obj_0, &style_light_panel, 0);
    lv_obj_bind_style(lv_obj_0, &style_dark_panel, 0, &subject_dark_mode, 1);
    lv_obj_add_style(lv_obj_0, &style_base, 0);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

