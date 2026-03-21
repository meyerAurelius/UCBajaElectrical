/**
 * @file button_circle_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "button_circle_gen.h"
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

lv_obj_t * button_circle_create(lv_obj_t * parent, const void * icon)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_image_recolor_opa(&style_base, 255);
        lv_style_set_width(&style_base, BASE_LIB_UNIT_XXL);
        lv_style_set_height(&style_base, BASE_LIB_UNIT_XXL);
        lv_style_set_radius(&style_base, 100);

        style_inited = true;
    }

    lv_obj_t * lv_image_0 = lv_image_create(parent);
    lv_obj_set_name_static(lv_image_0, "button_circle_#");
    lv_image_set_src(lv_image_0, icon);
    lv_obj_set_flag(lv_image_0, LV_OBJ_FLAG_CLICKABLE, true);

    lv_obj_remove_style_all(lv_image_0);
    lv_obj_add_style(lv_image_0, &style_base, 0);
    lv_obj_add_style(lv_image_0, &style_light_button, 0);
    lv_obj_add_style(lv_image_0, &style_light_button_pressed, LV_STATE_PRESSED);
    lv_obj_bind_style(lv_image_0, &style_dark_button, 0, &subject_dark_mode, 1);
    lv_obj_bind_style(lv_image_0, &style_dark_button_pressed, LV_STATE_PRESSED, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_image_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

