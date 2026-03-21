/**
 * @file card_weather_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "card_weather_gen.h"
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

lv_obj_t * card_weather_create(lv_obj_t * parent, int32_t grow)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_main;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_main);
        lv_style_set_width(&style_main, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_main, BASE_LIB_BLOCK_1_5);
        lv_style_set_layout(&style_main, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&style_main, LV_FLEX_FLOW_COLUMN);
        lv_style_set_flex_main_place(&style_main, LV_FLEX_ALIGN_CENTER);
        lv_style_set_pad_column(&style_main, BASE_LIB_UNIT_SM);
        lv_style_set_pad_all(&style_main, BASE_LIB_UNIT_XL);
        lv_style_set_text_font(&style_main, font_md);

        style_inited = true;
    }

    lv_obj_t * card_empty_0 = card_empty_create(parent);
    lv_obj_set_name_static(card_empty_0, "card_weather_#");
    lv_obj_set_flex_grow(card_empty_0, grow);

    lv_obj_add_style(card_empty_0, &style_main, 0);
    lv_obj_t * row_0 = row_create(card_empty_0, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t * image_mono_0 = image_mono_create(row_0, image_weather_foggy);
    lv_obj_set_style_image_recolor(image_mono_0, COLOR_ACCENT, 0);
    lv_obj_set_style_margin_top(image_mono_0, -6, 0);
    
    value_large_create(row_0, &subject_temp, "%d", "°C");
    
    lv_obj_t * row_1 = row_create(card_empty_0, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t * lv_label_0 = lv_label_create(row_1);
    lv_label_set_text(lv_label_0, "Foggy");
    lv_obj_set_style_text_font(lv_label_0, font_md_bold, 0);
    
    lv_obj_t * lv_label_1 = lv_label_create(row_1);
    lv_label_set_text(lv_label_1, "H:19° L:10°");

    LV_TRACE_OBJ_CREATE("finished");

    return card_empty_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

