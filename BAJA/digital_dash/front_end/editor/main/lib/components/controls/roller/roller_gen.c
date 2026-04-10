/**
 * @file roller_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "roller_gen.h"
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

lv_obj_t * roller_create(lv_obj_t * parent, const char * options)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_main;
    static lv_style_t style_main_dark;
    static lv_style_t style_indicator;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_main);
        lv_style_set_radius(&style_main, BASE_LIB_RADIUS);
        lv_style_set_border_width(&style_main, 0);
        lv_style_set_bg_color(&style_main, COLOR_LIGHT_PANEL);
        lv_style_set_text_font(&style_main, font_md);
        lv_style_set_text_color(&style_main, COLOR_LIGHT_TEXT);

        lv_style_init(&style_main_dark);
        lv_style_set_bg_color(&style_main_dark, COLOR_DARK_PANEL);
        lv_style_set_text_color(&style_main_dark, COLOR_DARK_TEXT);

        lv_style_init(&style_indicator);
        lv_style_set_border_width(&style_indicator, 0);
        lv_style_set_bg_color(&style_indicator, COLOR_ACCENT);
        lv_style_set_text_color(&style_indicator, COLOR_ACCENT_TEXT);

        style_inited = true;
    }

    lv_obj_t * lv_roller_0 = lv_roller_create(parent);
    lv_obj_set_name_static(lv_roller_0, "roller_#");
    lv_roller_set_options(lv_roller_0, options, LV_ROLLER_MODE_NORMAL);

    lv_obj_add_style(lv_roller_0, &style_main, 0);
    lv_obj_bind_style(lv_roller_0, &style_main_dark, 0, &subject_dark_mode, 1);
    lv_obj_add_style(lv_roller_0, &style_indicator, LV_PART_SELECTED);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_roller_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

