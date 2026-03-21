/**
 * @file screen_home_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_home_gen.h"
#include "../starter.h"

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

lv_obj_t * screen_home_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
    lv_obj_set_name_static(lv_obj_0, "screen_home_#");

    lv_obj_t * column_0 = column_create(lv_obj_0, 0, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_width(column_0, lv_pct(100));
    lv_obj_set_height(column_0, lv_pct(100));
    lv_obj_t * header = header_create(column_0);
    lv_obj_set_name(header, "header");
    time_create(header, &subject_hour, &subject_minute, font_sm);
    
    spacer_create(header);
    
    h4_create(header, "Main screen");
    
    spacer_create(header);
    
    icon_create(header, icon_wifi);
    
    lv_obj_t * theme_0 = theme_create(column_0);
    lv_obj_t * intro = column_create(theme_0, BASE_LIB_UNIT_XL, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_name(intro, "intro");
    lv_obj_set_width(intro, lv_pct(100));
    lv_obj_t * h2_2 = h2_create(intro, "Starter Component Library Demo");
    lv_obj_set_name(h2_2, "h2_2");
    
    lv_obj_t * h3_1 = h3_create(intro, "See how LVGL Pro's new Design Mode brings components to life.");
    lv_obj_set_name(h3_1, "h3_1");
    
    lv_obj_t * column_1 = column_create(theme_0, BASE_LIB_UNIT_XL, BASE_LIB_UNIT_XL, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_width(column_1, lv_pct(100));
    lv_obj_t * cards = row_create(column_1, 1, BASE_LIB_UNIT_LG, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_name(cards, "cards");
    lv_obj_set_width(cards, lv_pct(100));
    lv_obj_t * card_clock_1 = card_clock_create(cards, 1);
    lv_obj_set_name(card_clock_1, "card_clock_1");
    
    lv_obj_t * card_weather_1 = card_weather_create(cards, 1);
    lv_obj_set_name(card_weather_1, "card_weather_1");
    
    lv_obj_t * card_info_1 = card_info_create(cards, "6:05", "This Morning", image_alarm, 1);
    lv_obj_set_name(card_info_1, "card_info_1");
    
    lv_obj_t * container_0 = container_create(column_1);
    lv_obj_set_width(container_0, lv_pct(100));
    lv_obj_set_height(container_0, BASE_LIB_BLOCK_2);
    lv_obj_t * row_0 = row_create(container_0, 0, BASE_LIB_UNIT_XXXL, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_height(row_0, lv_pct(100));
    lv_obj_set_width(row_0, lv_pct(100));
    lv_obj_t * column_2 = column_create(row_0, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_START);
    lv_obj_set_height(column_2, lv_pct(100));
    checkbox_create(column_2);
    
    switch_create(column_2);
    
    lv_obj_t * panel_0 = panel_create(column_2, BASE_LIB_UNIT_XS);
    lv_obj_set_width(panel_0, LV_SIZE_CONTENT);
    value_setter_create(panel_0, &subject_test, "%d");
    
    lv_obj_t * roller_0 = roller_create(row_0, "Jan\nFeb\nMar\nApr\nMay\nJun\nJul\nAug\nSep\nOct\nNov\nDec");
    lv_obj_set_height(roller_0, lv_pct(100));
    
    lv_obj_t * chart_0 = chart_create(row_0, LV_CHART_TYPE_LINE, COLOR_ACCENT, 0, 100, 5, 10);
    lv_obj_set_height(chart_0, lv_pct(100));
    
    lv_obj_t * container_1 = container_create(column_1);
    lv_obj_set_width(container_1, lv_pct(100));
    lv_obj_t * row_1 = row_create(container_1, 0, BASE_LIB_UNIT_XL, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_width(row_1, lv_pct(100));
    lv_obj_t * column_3 = column_create(row_1, 0, BASE_LIB_UNIT_XL, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    arc_box_create(column_3, "Title", &subject_test, "%d");
    
    slider_box_create(column_3, "Title", &subject_test, "%d");
    
    lv_obj_t * list_container_0 = list_container_create(row_1);
    list_item_create(list_container_0, "Title", "Subtitle");
    
    list_item_create(list_container_0, "Title", "Subtitle");
    
    list_item_create(list_container_0, "Title", "Subtitle");

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

