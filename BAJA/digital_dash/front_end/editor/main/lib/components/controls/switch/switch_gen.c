/**
 * @file switch_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "switch_gen.h"
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

lv_obj_t * switch_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;
    static lv_style_t style_base_dark;
    static lv_style_t style_indicator_checked;
    static lv_style_t style_knob;
    static lv_style_t style_knob_checked;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_bg_color(&style_base, COLOR_ACCENT);
        lv_style_set_bg_opa(&style_base, (255 * 24 / 100));

        lv_style_init(&style_base_dark);
        lv_style_set_bg_color(&style_base_dark, COLOR_ACCENT);

        lv_style_init(&style_indicator_checked);
        lv_style_set_bg_color(&style_indicator_checked, COLOR_ACCENT);
        lv_style_set_bg_opa(&style_indicator_checked, (255 * 100 / 100));

        lv_style_init(&style_knob);
        lv_style_set_bg_color(&style_knob, COLOR_ACCENT);
        lv_style_set_pad_all(&style_knob, -5);

        lv_style_init(&style_knob_checked);
        lv_style_set_bg_color(&style_knob_checked, COLOR_ACCENT_TEXT);
        lv_style_set_bg_opa(&style_knob_checked, (255 * 100 / 100));

        style_inited = true;
    }

    lv_obj_t * lv_switch_0 = lv_switch_create(parent);
    lv_obj_set_name_static(lv_switch_0, "switch_#");

    lv_obj_add_style(lv_switch_0, &style_base, 0);
    lv_obj_add_style(lv_switch_0, &style_knob, LV_PART_KNOB);
    lv_obj_add_style(lv_switch_0, &style_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(lv_switch_0, &style_knob_checked, LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_bind_style(lv_switch_0, &style_base_dark, 0, &subject_dark_mode, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_switch_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

