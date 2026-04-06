/**
 * @file arc_box_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "arc_box_gen.h"
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

lv_obj_t * arc_box_create(lv_obj_t * parent, const char * title, lv_subject_t * subject, const char * format)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_base;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_base);
        lv_style_set_width(&style_base, BASE_LIB_BLOCK_2);
        lv_style_set_height(&style_base, BASE_LIB_BLOCK_1_5);

        style_inited = true;
    }

    lv_obj_t * panel_0 = panel_create(parent, BASE_LIB_UNIT_LG);
    lv_obj_set_name_static(panel_0, "arc_box_#");
    lv_obj_set_flex_flow(panel_0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_track_place(panel_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(panel_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_height(panel_0, LV_SIZE_CONTENT);

    lv_obj_add_style(panel_0, &style_base, 0);
    lv_obj_t * h3_0 = h3_create(panel_0, title);
    lv_obj_set_width(h3_0, lv_pct(100));
    
    lv_obj_t * arc_0 = arc_create(panel_0, subject);
    lv_obj_set_style_margin_bottom(arc_0, BASE_LIB_UNIT_MD, 0);
    lv_obj_t * lv_label_0 = lv_label_create(arc_0);
    lv_obj_set_style_text_font(lv_label_0, font_lg_bold, 0);
    lv_obj_set_align(lv_label_0, LV_ALIGN_CENTER);
    lv_label_bind_text(lv_label_0, subject, format);
    
    lv_obj_t * button_circle_0 = button_circle_create(panel_0, icon_minus);
    lv_obj_set_flag(button_circle_0, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
    lv_obj_set_align(button_circle_0, LV_ALIGN_BOTTOM_LEFT);
    lv_subject_increment_dsc_t * subject_increment_event_0 = lv_obj_add_subject_increment_event(button_circle_0, subject, LV_EVENT_CLICKED, -1);
    lv_subject_increment_dsc_t * subject_increment_event_1 = lv_obj_add_subject_increment_event(button_circle_0, subject, LV_EVENT_LONG_PRESSED_REPEAT, -1);
    
    lv_obj_t * button_circle_1 = button_circle_create(panel_0, icon_plus);
    lv_obj_set_flag(button_circle_1, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
    lv_obj_set_align(button_circle_1, LV_ALIGN_BOTTOM_RIGHT);
    lv_subject_increment_dsc_t * subject_increment_event_2 = lv_obj_add_subject_increment_event(button_circle_1, subject, LV_EVENT_CLICKED, 1);
    lv_subject_increment_dsc_t * subject_increment_event_3 = lv_obj_add_subject_increment_event(button_circle_1, subject, LV_EVENT_LONG_PRESSED_REPEAT, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return panel_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

