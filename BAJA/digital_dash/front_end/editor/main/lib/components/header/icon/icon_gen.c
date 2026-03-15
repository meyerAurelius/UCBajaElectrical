/**
 * @file icon_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "icon_gen.h"
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

lv_obj_t * icon_create(lv_obj_t * parent, const void * icon)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_icon;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_icon);
        lv_style_set_image_recolor(&style_icon, COLOR_DARK_TEXT);
        lv_style_set_image_recolor_opa(&style_icon, (255 * 100 / 100));

        style_inited = true;
    }

    lv_obj_t * lv_image_0 = lv_image_create(parent);
    lv_obj_set_name_static(lv_image_0, "icon_#");
    lv_image_set_src(lv_image_0, icon);
    lv_image_set_inner_align(lv_image_0, LV_IMAGE_ALIGN_CENTER);

    lv_obj_add_style(lv_image_0, &style_icon, 0);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_image_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

