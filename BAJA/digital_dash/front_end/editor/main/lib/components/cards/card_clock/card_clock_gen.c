/**
 * @file card_clock_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "card_clock_gen.h"
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

lv_obj_t * card_clock_create(lv_obj_t * parent, int32_t grow)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_clock;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_clock);
        lv_style_set_text_font(&style_clock, font_xl);
        lv_style_set_width(&style_clock, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_clock, BASE_LIB_BLOCK_1_5);
        lv_style_set_flex_track_place(&style_clock, LV_FLEX_ALIGN_START);
        lv_style_set_flex_main_place(&style_clock, LV_FLEX_ALIGN_CENTER);
        lv_style_set_flex_cross_place(&style_clock, LV_FLEX_ALIGN_START);
        lv_style_set_layout(&style_clock, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&style_clock, LV_FLEX_FLOW_COLUMN);
        lv_style_set_pad_row(&style_clock, 0);

        style_inited = true;
    }

    lv_obj_t * card_empty_0 = card_empty_create(parent);
    lv_obj_set_name_static(card_empty_0, "card_clock_#");
    lv_obj_set_style_pad_all(card_empty_0, BASE_LIB_UNIT_XL, 0);
    lv_obj_set_flex_grow(card_empty_0, grow);

    lv_obj_add_style(card_empty_0, &style_clock, 0);
    lv_obj_bind_style(card_empty_0, &style_dark_panel, 0, &subject_dark_mode, 1);
    lv_obj_t * time_0 = time_create(card_empty_0, &subject_hour, &subject_minute, font_xxl);
    lv_obj_set_height(time_0, 48);
    
    date_create(card_empty_0, &subject_day_name, &subject_day_index, &subject_month_name, font_md);

    LV_TRACE_OBJ_CREATE("finished");

    return card_empty_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

