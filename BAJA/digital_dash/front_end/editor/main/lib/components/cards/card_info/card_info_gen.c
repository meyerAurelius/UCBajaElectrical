/**
 * @file card_info_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "card_info_gen.h"
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

lv_obj_t * card_info_create(lv_obj_t * parent, const char * title, const char * subtitle, const void * icon, int32_t grow)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_text_font(&style_base, font_xl);
        lv_style_set_width(&style_base, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_base, BASE_LIB_BLOCK_1_5);

        style_inited = true;
    }

    lv_obj_t * card_empty_0 = card_empty_create(parent);
    lv_obj_set_name_static(card_empty_0, "card_info_#");
    lv_obj_set_flex_grow(card_empty_0, grow);

    lv_obj_add_style(card_empty_0, &style_base, 0);
    lv_obj_t * column_0 = column_create(card_empty_0, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_width(column_0, LV_SIZE_CONTENT);
    lv_obj_set_height(column_0, lv_pct(100));
    lv_obj_set_style_flex_main_place(column_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(column_0, LV_FLEX_ALIGN_START, 0);
    lv_obj_t * row_0 = row_create(column_0, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t * image_mono_0 = image_mono_create(row_0, icon);
    lv_obj_set_style_image_recolor(image_mono_0, COLOR_ACCENT, 0);
    lv_obj_set_style_margin_top(image_mono_0, -6, 0);
    
    h1_create(row_0, title);
    
    h3_create(column_0, subtitle);

    LV_TRACE_OBJ_CREATE("finished");

    return card_empty_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

