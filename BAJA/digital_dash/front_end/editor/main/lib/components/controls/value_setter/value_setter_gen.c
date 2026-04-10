/**
 * @file value_setter_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "value_setter_gen.h"
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

lv_obj_t * value_setter_create(lv_obj_t * parent, lv_subject_t * subject, const char * format)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_main;
    static lv_style_t style_label;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_main);
        lv_style_set_width(&style_main, BASE_LIB_BLOCK_1_5);

        lv_style_init(&style_label);
        lv_style_set_text_font(&style_label, font_lg_bold);

        style_inited = true;
    }

    lv_obj_t * row_0 = row_create(parent, BASE_LIB_UNIT_SM, BASE_LIB_UNIT_MD, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_name_static(row_0, "value_setter_#");

    lv_obj_add_style(row_0, &style_main, 0);
    lv_obj_t * button_circle_0 = button_circle_create(row_0, icon_minus);
    lv_subject_increment_dsc_t * subject_increment_event_0 = lv_obj_add_subject_increment_event(button_circle_0, subject, LV_EVENT_CLICKED, -1);
    lv_subject_increment_dsc_t * subject_increment_event_1 = lv_obj_add_subject_increment_event(button_circle_0, subject, LV_EVENT_LONG_PRESSED_REPEAT, -1);
    
    lv_obj_t * lv_label_0 = lv_label_create(row_0);
    lv_label_bind_text(lv_label_0, subject, format);
    lv_obj_add_style(lv_label_0, &style_label, 0);
    
    lv_obj_t * button_circle_1 = button_circle_create(row_0, icon_plus);
    lv_subject_increment_dsc_t * subject_increment_event_2 = lv_obj_add_subject_increment_event(button_circle_1, subject, LV_EVENT_CLICKED, 1);
    lv_subject_increment_dsc_t * subject_increment_event_3 = lv_obj_add_subject_increment_event(button_circle_1, subject, LV_EVENT_LONG_PRESSED_REPEAT, 1);

    LV_TRACE_OBJ_CREATE("finished");

    return row_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

