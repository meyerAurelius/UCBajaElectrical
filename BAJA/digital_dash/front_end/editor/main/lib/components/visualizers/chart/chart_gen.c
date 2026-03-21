/**
 * @file chart_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "chart_gen.h"
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

lv_obj_t * chart_create(lv_obj_t * parent, lv_chart_type_t type, lv_color_t color, int32_t min_value, int32_t max_value, int32_t tick_count, int32_t point_count)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;
    static lv_style_t style_chart;
    static lv_style_t style_series_items;
    static lv_style_t style_series_indicator;
    static lv_style_t style_scale;
    static lv_style_t style_scale_indicator;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_width(&style_base, BASE_LIB_BLOCK_3);
        lv_style_set_height(&style_base, BASE_LIB_BLOCK_2);
        lv_style_set_pad_column(&style_base, 0);

        lv_style_init(&style_chart);
        lv_style_set_flex_grow(&style_chart, 1);
        lv_style_set_height(&style_chart, lv_pct(100));
        lv_style_set_pad_ver(&style_chart, 31);
        lv_style_set_pad_hor(&style_chart, BASE_LIB_UNIT_LG);
        lv_style_set_pad_column(&style_chart, BASE_LIB_UNIT_MD);
        lv_style_set_border_width(&style_chart, 0);
        lv_style_set_radius(&style_chart, BASE_LIB_RADIUS);
        lv_style_set_line_width(&style_chart, 1);

        lv_style_init(&style_series_items);
        lv_style_set_line_width(&style_series_items, BASE_LIB_BORDER_WIDTH);
        lv_style_set_radius(&style_series_items, 10);

        lv_style_init(&style_series_indicator);
        lv_style_set_width(&style_series_indicator, BASE_LIB_UNIT_SM);
        lv_style_set_height(&style_series_indicator, BASE_LIB_UNIT_SM);

        lv_style_init(&style_scale);
        lv_style_set_height(&style_scale, lv_pct(84));
        lv_style_set_pad_top(&style_scale, 32);
        lv_style_set_width(&style_scale, BASE_LIB_UNIT_XXL);
        lv_style_set_text_font(&style_scale, font_xs);

        lv_style_init(&style_scale_indicator);
        lv_style_set_line_color(&style_scale_indicator, COLOR_LIGHT_TEXT);
        lv_style_set_line_opa(&style_scale_indicator, 0);
        lv_style_set_length(&style_scale_indicator, BASE_LIB_UNIT_SM);
        lv_style_set_pad_left(&style_scale_indicator, BASE_LIB_BORDER_WIDTH);

        style_inited = true;
    }

    lv_obj_t * row_0 = row_create(parent, 0, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_name_static(row_0, "chart_#");
    lv_obj_set_style_pad_ver(row_0, BASE_LIB_UNIT_SM, 0);
    lv_obj_set_style_pad_column(row_0, -2, 0);

    lv_obj_add_style(row_0, &style_base, 0);
    lv_obj_t * lv_scale_0 = lv_scale_create(row_0);
    lv_scale_set_mode(lv_scale_0, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_scale_set_min_value(lv_scale_0, min_value);
    lv_scale_set_max_value(lv_scale_0, max_value);
    lv_scale_set_major_tick_every(lv_scale_0, 1);
    lv_scale_set_total_tick_count(lv_scale_0, tick_count);
    lv_obj_add_style(lv_scale_0, &style_scale, 0);
    lv_obj_add_style(lv_scale_0, &style_light_text, 0);
    lv_obj_add_style(lv_scale_0, &style_scale_indicator, LV_PART_INDICATOR);
    lv_obj_bind_style(lv_scale_0, &style_dark_text, 0, &subject_dark_mode, 1);
    
    lv_obj_t * lv_chart_0 = lv_chart_create(row_0);
    lv_obj_set_style_line_width(lv_chart_0, 4, 0);
    lv_chart_set_type(lv_chart_0, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(lv_chart_0, point_count);
    lv_chart_set_hor_div_line_count(lv_chart_0, tick_count);
    lv_chart_set_ver_div_line_count(lv_chart_0, 0);
    lv_obj_remove_style_all(lv_chart_0);
    lv_obj_add_style(lv_chart_0, &style_chart, 0);
    lv_obj_add_style(lv_chart_0, &style_light_panel, 0);
    lv_obj_add_style(lv_chart_0, &style_series_items, LV_PART_ITEMS);
    lv_obj_add_style(lv_chart_0, &style_series_indicator, LV_PART_INDICATOR);
    lv_obj_bind_style(lv_chart_0, &style_dark_panel, 0, &subject_dark_mode, 1);
    lv_chart_series_t * lv_chart_series_0 = lv_chart_add_series(lv_chart_0, color, LV_CHART_AXIS_PRIMARY_Y);
    static const int32_t lv_chart_0_values_0[] = {00, 20, 0, 40, 80, 90, 20, 60, 40, 70, 20, 30};
    lv_chart_set_series_values(lv_chart_0, lv_chart_series_0, lv_chart_0_values_0, 12);
    lv_chart_set_axis_min_value(lv_chart_0, LV_CHART_AXIS_PRIMARY_Y, min_value);
    lv_chart_set_axis_max_value(lv_chart_0, LV_CHART_AXIS_PRIMARY_Y, max_value);

    LV_TRACE_OBJ_CREATE("finished");

    return row_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

