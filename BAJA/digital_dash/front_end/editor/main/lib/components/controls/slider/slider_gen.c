/**
 * @file slider_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "slider_gen.h"
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

lv_obj_t * slider_create(lv_obj_t * parent, lv_subject_t * subject)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_slider_light;
    static lv_style_t style_slider_light_indicator;
    static lv_style_t style_slider_light_knob;
    static lv_style_t style_slider_light_knob_pressed;
    static lv_style_t style_slider_dark;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_slider_light);
        lv_style_set_height(&style_slider_light, BASE_LIB_UNIT_LG);
        lv_style_set_bg_color(&style_slider_light, COLOR_ACCENT);
        lv_style_set_bg_opa(&style_slider_light, (255 * 16 / 100));
        lv_style_set_radius(&style_slider_light, BASE_LIB_RADIUS);
        lv_style_set_clip_corner(&style_slider_light, true);

        lv_style_init(&style_slider_light_indicator);
        lv_style_set_bg_color(&style_slider_light_indicator, COLOR_ACCENT);
        lv_style_set_bg_opa(&style_slider_light_indicator, (255 * 100 / 100));
        lv_style_set_radius(&style_slider_light_indicator, BASE_LIB_RADIUS);

        lv_style_init(&style_slider_light_knob);
        lv_style_set_bg_color(&style_slider_light_knob, COLOR_ACCENT_TEXT);
        lv_style_set_pad_left(&style_slider_light_knob, 3);
        lv_style_set_pad_right(&style_slider_light_knob, -10);
        lv_style_set_pad_ver(&style_slider_light_knob, -4);

        lv_style_init(&style_slider_light_knob_pressed);
        lv_style_set_transform_height(&style_slider_light_knob_pressed, 0);
        lv_style_set_transform_width(&style_slider_light_knob_pressed, 0);

        lv_style_init(&style_slider_dark);
        lv_style_set_bg_color(&style_slider_dark, COLOR_DARK_TEXT);

        style_inited = true;
    }

    lv_obj_t * lv_slider_0 = lv_slider_create(parent);
    lv_obj_set_name_static(lv_slider_0, "slider_#");
    lv_slider_set_value(lv_slider_0, 50, false);
    lv_slider_bind_value(lv_slider_0, subject);

    lv_obj_add_style(lv_slider_0, &style_slider_light, 0);
    lv_obj_add_style(lv_slider_0, &style_slider_light_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(lv_slider_0, &style_slider_light_knob, LV_PART_KNOB);
    lv_obj_add_style(lv_slider_0, &style_slider_light_knob_pressed, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_bind_style(lv_slider_0, &style_slider_dark, 0, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_slider_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

