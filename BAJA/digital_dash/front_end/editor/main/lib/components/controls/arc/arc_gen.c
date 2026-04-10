/**
 * @file arc_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "arc_gen.h"
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

lv_obj_t * arc_create(lv_obj_t * parent, lv_subject_t * subject)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_light_arc;
    static lv_style_t style_light_arc_indicator;
    static lv_style_t style_light_arc_knob;
    static lv_style_t style_dark_arc;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_light_arc);
        lv_style_set_arc_width(&style_light_arc, BASE_LIB_UNIT_LG);
        lv_style_set_arc_color(&style_light_arc, COLOR_ACCENT);
        lv_style_set_arc_opa(&style_light_arc, (255 * 16 / 100));

        lv_style_init(&style_light_arc_indicator);
        lv_style_set_arc_width(&style_light_arc_indicator, BASE_LIB_UNIT_LG);
        lv_style_set_arc_color(&style_light_arc_indicator, COLOR_ACCENT);
        lv_style_set_arc_opa(&style_light_arc_indicator, (255 * 100 / 100));

        lv_style_init(&style_light_arc_knob);
        lv_style_set_bg_color(&style_light_arc_knob, COLOR_ACCENT_TEXT);
        lv_style_set_pad_all(&style_light_arc_knob, -4);

        lv_style_init(&style_dark_arc);
        lv_style_set_arc_width(&style_dark_arc, BASE_LIB_UNIT_LG);
        lv_style_set_arc_color(&style_dark_arc, COLOR_ACCENT);
        lv_style_set_arc_opa(&style_dark_arc, (255 * 16 / 100));

        style_inited = true;
    }

    lv_obj_t * lv_arc_0 = lv_arc_create(parent);
    lv_obj_set_name_static(lv_arc_0, "arc_#");
    lv_arc_bind_value(lv_arc_0, subject);

    lv_obj_add_style(lv_arc_0, &style_light_arc, 0);
    lv_obj_add_style(lv_arc_0, &style_light_arc_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(lv_arc_0, &style_light_arc_knob, LV_PART_KNOB);
    lv_obj_bind_style(lv_arc_0, &style_dark_arc, 0, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_arc_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

