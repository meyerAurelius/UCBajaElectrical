/**
 * @file starter_gen.h
 */

#ifndef STARTER_GEN_H
#define STARTER_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "src/core/lv_obj_class_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/src/core/lv_obj_class_private.h"
#endif



/*********************
 *      DEFINES
 *********************/

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_XS 2

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_SM 4

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_MD 8

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_LG 16

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_XL 24

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_XXL 32

/**
 * Use this as base unit to set spaces and sizes
 */
#define BASE_LIB_UNIT_XXXL 40

/**
 * Used as radius in styles
 */
#define BASE_LIB_RADIUS 20

/**
 * Used as border width styles
 */
#define BASE_LIB_BORDER_WIDTH 2

/**
 * When setting up size sizes it's 0.5 unit
 */
#define BASE_LIB_BLOCK_0_5 50

/**
 * When setting up size sizes it's 1 unit
 */
#define BASE_LIB_BLOCK_1 100

/**
 * When setting up size sizes it's 0.5 unit
 */
#define BASE_LIB_BLOCK_1_25 125

/**
 * When setting up size sizes it's 1.5 unit
 */
#define BASE_LIB_BLOCK_1_5 150

/**
 * When setting up size sizes it's 2 units
 */
#define BASE_LIB_BLOCK_2 200

/**
 * When setting up size sizes it's 3 units
 */
#define BASE_LIB_BLOCK_3 300

#define COLOR_LIGHT_BG lv_color_hex(0xD2D0F2)

#define COLOR_LIGHT_PANEL lv_color_hex(0xF4EDF8)

#define COLOR_LIGHT_TEXT lv_color_hex(0x161019)

#define COLOR_LIGHT_BUTTON lv_color_hex(0xD2BAFF)

#define COLOR_LIGHT_BUTTON_TEXT lv_color_hex(0x5C428A)

#define COLOR_DARK_BG lv_color_hex(0x161019)

#define COLOR_DARK_PANEL lv_color_hex(0x26212C)

#define COLOR_DARK_TEXT lv_color_hex(0xD1CBD7)

#define COLOR_DARK_BUTTON lv_color_hex(0xD9C2FF)

#define COLOR_DARK_BUTTON_TEXT lv_color_hex(0x462182)

#define COLOR_ACCENT lv_color_hex(0x70609C)

#define COLOR_ACCENT_TEXT lv_color_hex3(0xFFF)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

extern lv_style_t style_light_bg;
extern lv_style_t style_light_panel;
extern lv_style_t style_light_button;
extern lv_style_t style_light_text;
extern lv_style_t style_light_text_accent;
extern lv_style_t style_light_button_pressed;
extern lv_style_t style_light_scrollbar;
extern lv_style_t style_dark_bg;
extern lv_style_t style_dark_panel;
extern lv_style_t style_dark_button;
extern lv_style_t style_dark_button_pressed;
extern lv_style_t style_dark_text;
extern lv_style_t style_dark_text_accent;
extern lv_style_t style_dark_scrollbar;

/*----------------
 * Fonts
 *----------------*/

extern lv_font_t * font_xs;

extern lv_font_t * font_sm;

extern lv_font_t * font_md;

extern lv_font_t * font_md_bold;

extern lv_font_t * font_lg;

extern lv_font_t * font_lg_bold;

extern lv_font_t * font_xl;

extern lv_font_t * font_xxl;

/*----------------
 * Images
 *----------------*/

extern const void * icon_check;
extern const void * icon_minus;
extern const void * icon_plus;
extern const void * icon_wifi;
extern const void * image_alarm;
extern const void * image_weather_foggy;

/*----------------
 * Subjects
 *----------------*/

extern lv_subject_t subject_dark_mode;
extern lv_subject_t subject_hour;
extern lv_subject_t subject_minute;
extern lv_subject_t subject_day_index;
extern lv_subject_t subject_day_name;
extern lv_subject_t subject_month_name;
extern lv_subject_t subject_temp;
extern lv_subject_t subject_test;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void starter_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widgets, components and screens of this library*/
#include "components/buttons/button_circle/button_circle_gen.h"
#include "components/buttons/button/button_gen.h"
#include "components/cards/card_clock/card_clock_gen.h"
#include "components/cards/card_empty/card_empty_gen.h"
#include "components/cards/card_info/card_info_gen.h"
#include "components/cards/card_weather/card_weather_gen.h"
#include "components/controls/arc_box/arc_box_gen.h"
#include "components/controls/arc/arc_gen.h"
#include "components/controls/checkbox/checkbox_gen.h"
#include "components/controls/roller/roller_gen.h"
#include "components/controls/slider_box/slider_box_gen.h"
#include "components/controls/slider/slider_gen.h"
#include "components/controls/switch/switch_gen.h"
#include "components/controls/value_setter/value_setter_gen.h"
#include "components/header/header/header_gen.h"
#include "components/header/icon/icon_gen.h"
#include "components/headings/h1/h1_gen.h"
#include "components/headings/h2/h2_gen.h"
#include "components/headings/h3/h3_gen.h"
#include "components/headings/h4/h4_gen.h"
#include "components/layout/column/column_gen.h"
#include "components/layout/container/container_gen.h"
#include "components/layout/panel/panel_gen.h"
#include "components/layout/row/row_gen.h"
#include "components/layout/spacer/spacer_gen.h"
#include "components/list/list_container/list_container_gen.h"
#include "components/list/list_item/list_item_gen.h"
#include "components/visualizers/chart/chart_gen.h"
#include "components/visualizers/date/date_gen.h"
#include "components/visualizers/image_mono/image_mono_gen.h"
#include "components/visualizers/image/image_gen.h"
#include "components/visualizers/theme/theme_gen.h"
#include "components/visualizers/time/time_gen.h"
#include "components/visualizers/value_large/value_large_gen.h"
#include "components/visualizers/value_medium/value_medium_gen.h"
#include "components/visualizers/value_small/value_small_gen.h"
#include "screens/screen_home_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*STARTER_GEN_H*/