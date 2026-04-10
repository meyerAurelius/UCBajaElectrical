/**
 * @file image_mono_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "image_mono_gen.h"
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

lv_obj_t * image_mono_create(lv_obj_t * parent, const void * src)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base_light;
    static lv_style_t style_base_dark;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base_light);
        lv_style_set_image_recolor(&style_base_light, COLOR_LIGHT_TEXT);
        lv_style_set_image_recolor_opa(&style_base_light, (255 * 100 / 100));

        lv_style_init(&style_base_dark);
        lv_style_set_image_recolor(&style_base_dark, COLOR_DARK_TEXT);

        style_inited = true;
    }

    lv_obj_t * lv_image_0 = lv_image_create(parent);
    lv_obj_set_name_static(lv_image_0, "image_mono_#");
    lv_image_set_src(lv_image_0, src);

    lv_obj_add_style(lv_image_0, &style_base_light, 0);
    lv_obj_bind_style(lv_image_0, &style_base_dark, 0, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_image_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

