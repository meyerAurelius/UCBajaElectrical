/**
 * @file starter_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "starter_gen.h"

#if LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/*----------------
 * Translations
 *----------------*/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Fonts
 *----------------*/

lv_font_t * font_xs;
extern lv_font_t font_xs_data;
lv_font_t * font_sm;
extern lv_font_t font_sm_data;
lv_font_t * font_md;
extern lv_font_t font_md_data;
lv_font_t * font_md_bold;
extern lv_font_t font_md_bold_data;
lv_font_t * font_lg;
extern lv_font_t font_lg_data;
lv_font_t * font_lg_bold;
extern lv_font_t font_lg_bold_data;
lv_font_t * font_xl;
extern lv_font_t font_xl_data;
lv_font_t * font_xxl;
extern lv_font_t font_xxl_data;

/*----------------
 * Images
 *----------------*/

const void * icon_check;
extern const void * icon_check_data;
const void * icon_minus;
extern const void * icon_minus_data;
const void * icon_plus;
extern const void * icon_plus_data;
const void * icon_wifi;
extern const void * icon_wifi_data;
const void * image_alarm;
extern const void * image_alarm_data;
const void * image_weather_foggy;
extern const void * image_weather_foggy_data;

/*----------------
 * Global styles
 *----------------*/

lv_style_t style_light_bg;
lv_style_t style_light_panel;
lv_style_t style_light_button;
lv_style_t style_light_text;
lv_style_t style_light_text_accent;
lv_style_t style_light_button_pressed;
lv_style_t style_light_scrollbar;
lv_style_t style_dark_bg;
lv_style_t style_dark_panel;
lv_style_t style_dark_button;
lv_style_t style_dark_button_pressed;
lv_style_t style_dark_text;
lv_style_t style_dark_text_accent;
lv_style_t style_dark_scrollbar;

/*----------------
 * Subjects
 *----------------*/

lv_subject_t subject_dark_mode;
lv_subject_t subject_hour;
lv_subject_t subject_minute;
lv_subject_t subject_day_index;
lv_subject_t subject_day_name;
lv_subject_t subject_month_name;
lv_subject_t subject_temp;
lv_subject_t subject_test;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void starter_init_gen(const char * asset_path)
{
    char buf[256];


    /*----------------
     * Fonts
     *----------------*/

    /* get font 'font_xs' from a C array */
    font_xs = &font_xs_data;
    /* get font 'font_sm' from a C array */
    font_sm = &font_sm_data;
    /* get font 'font_md' from a C array */
    font_md = &font_md_data;
    /* get font 'font_md_bold' from a C array */
    font_md_bold = &font_md_bold_data;
    /* get font 'font_lg' from a C array */
    font_lg = &font_lg_data;
    /* get font 'font_lg_bold' from a C array */
    font_lg_bold = &font_lg_bold_data;
    /* get font 'font_xl' from a C array */
    font_xl = &font_xl_data;
    /* get font 'font_xxl' from a C array */
    font_xxl = &font_xxl_data;


    /*----------------
     * Images
     *----------------*/
    icon_check = &icon_check_data;
    icon_minus = &icon_minus_data;
    icon_plus = &icon_plus_data;
    icon_wifi = &icon_wifi_data;
    image_alarm = &image_alarm_data;
    image_weather_foggy = &image_weather_foggy_data;

    /*----------------
     * Global styles
     *----------------*/

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_light_bg);
        lv_style_set_bg_color(&style_light_bg, COLOR_LIGHT_BG);
        lv_style_set_text_color(&style_light_bg, COLOR_LIGHT_TEXT);
        lv_style_set_image_recolor(&style_light_bg, COLOR_LIGHT_TEXT);
        lv_style_set_bg_opa(&style_light_bg, (255 * 100 / 100));

        lv_style_init(&style_light_panel);
        lv_style_set_bg_color(&style_light_panel, COLOR_LIGHT_PANEL);
        lv_style_set_border_color(&style_light_panel, COLOR_LIGHT_TEXT);
        lv_style_set_text_color(&style_light_panel, COLOR_LIGHT_TEXT);
        lv_style_set_line_color(&style_light_panel, COLOR_LIGHT_TEXT);
        lv_style_set_image_recolor(&style_light_panel, COLOR_LIGHT_TEXT);
        lv_style_set_bg_opa(&style_light_panel, (255 * 100 / 100));
        lv_style_set_border_opa(&style_light_panel, (255 * 20 / 100));
        lv_style_set_line_opa(&style_light_panel, (255 * 20 / 100));

        lv_style_init(&style_light_button);
        lv_style_set_bg_color(&style_light_button, COLOR_LIGHT_BUTTON);
        lv_style_set_text_color(&style_light_button, COLOR_LIGHT_BUTTON_TEXT);
        lv_style_set_image_recolor(&style_light_button, COLOR_LIGHT_BUTTON_TEXT);
        lv_style_set_bg_opa(&style_light_button, (255 * 100 / 100));

        lv_style_init(&style_light_text);
        lv_style_set_text_color(&style_light_text, COLOR_LIGHT_TEXT);

        lv_style_init(&style_light_text_accent);
        lv_style_set_text_color(&style_light_text_accent, COLOR_ACCENT);

        lv_style_init(&style_light_button_pressed);
        lv_style_set_recolor(&style_light_button_pressed, COLOR_LIGHT_TEXT);
        lv_style_set_recolor_opa(&style_light_button_pressed, (255 * 40 / 100));

        lv_style_init(&style_light_scrollbar);
        lv_style_set_width(&style_light_scrollbar, BASE_LIB_UNIT_SM);
        lv_style_set_radius(&style_light_scrollbar, BASE_LIB_UNIT_SM);
        lv_style_set_bg_color(&style_light_scrollbar, lv_color_hex3(0x888));
        lv_style_set_bg_opa(&style_light_scrollbar, (255 * 50 / 100));

        lv_style_init(&style_dark_bg);
        lv_style_set_bg_color(&style_dark_bg, COLOR_DARK_BG);
        lv_style_set_text_color(&style_dark_bg, COLOR_DARK_TEXT);
        lv_style_set_image_recolor(&style_dark_bg, COLOR_DARK_TEXT);
        lv_style_set_bg_opa(&style_dark_bg, (255 * 100 / 100));

        lv_style_init(&style_dark_panel);
        lv_style_set_bg_color(&style_dark_panel, COLOR_DARK_PANEL);
        lv_style_set_border_color(&style_dark_panel, COLOR_DARK_TEXT);
        lv_style_set_text_color(&style_dark_panel, COLOR_DARK_TEXT);
        lv_style_set_line_color(&style_dark_panel, COLOR_DARK_TEXT);
        lv_style_set_image_recolor(&style_dark_panel, COLOR_DARK_TEXT);
        lv_style_set_bg_opa(&style_dark_panel, (255 * 100 / 100));
        lv_style_set_border_opa(&style_dark_panel, (255 * 20 / 100));
        lv_style_set_line_opa(&style_dark_panel, (255 * 20 / 100));

        lv_style_init(&style_dark_button);
        lv_style_set_bg_color(&style_dark_button, COLOR_DARK_BUTTON);
        lv_style_set_text_color(&style_dark_button, COLOR_DARK_BUTTON_TEXT);
        lv_style_set_image_recolor(&style_dark_button, COLOR_DARK_BUTTON_TEXT);
        lv_style_set_bg_opa(&style_dark_button, (255 * 100 / 100));

        lv_style_init(&style_dark_button_pressed);
        lv_style_set_recolor(&style_dark_button_pressed, COLOR_DARK_BG);
        lv_style_set_recolor_opa(&style_dark_button_pressed, (255 * 40 / 100));

        lv_style_init(&style_dark_text);
        lv_style_set_text_color(&style_dark_text, COLOR_DARK_TEXT);

        lv_style_init(&style_dark_text_accent);
        lv_style_set_text_color(&style_dark_text_accent, COLOR_ACCENT);

        lv_style_init(&style_dark_scrollbar);
        lv_style_set_width(&style_dark_scrollbar, BASE_LIB_UNIT_SM);
        lv_style_set_radius(&style_dark_scrollbar, BASE_LIB_UNIT_SM);
        lv_style_set_bg_color(&style_dark_scrollbar, lv_color_hex3(0x888));
        lv_style_set_bg_opa(&style_dark_scrollbar, (255 * 50 / 100));

        style_inited = true;
    }

    /*----------------
     * Subjects
     *----------------*/
    lv_subject_init_int(&subject_dark_mode, 0);
    lv_subject_init_int(&subject_hour, 9);
    lv_subject_init_int(&subject_minute, 43);
    lv_subject_init_int(&subject_day_index, 12);
    static char subject_day_name_buf[UI_SUBJECT_STRING_LENGTH];
    static char subject_day_name_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&subject_day_name,
                           subject_day_name_buf,
                           subject_day_name_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "Wednesday"
                          );
    static char subject_month_name_buf[UI_SUBJECT_STRING_LENGTH];
    static char subject_month_name_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&subject_month_name,
                           subject_month_name_buf,
                           subject_month_name_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "Jan"
                          );
    lv_subject_init_int(&subject_temp, 23);
    lv_subject_init_int(&subject_test, 30);

    /*----------------
     * Translations
     *----------------*/

#if LV_USE_XML
    /* Register widgets */

    /* Register fonts */
    lv_xml_register_font(NULL, "font_xs", font_xs);
    lv_xml_register_font(NULL, "font_sm", font_sm);
    lv_xml_register_font(NULL, "font_md", font_md);
    lv_xml_register_font(NULL, "font_md_bold", font_md_bold);
    lv_xml_register_font(NULL, "font_lg", font_lg);
    lv_xml_register_font(NULL, "font_lg_bold", font_lg_bold);
    lv_xml_register_font(NULL, "font_xl", font_xl);
    lv_xml_register_font(NULL, "font_xxl", font_xxl);

    /* Register subjects */
    lv_xml_register_subject(NULL, "subject_dark_mode", &subject_dark_mode);
    lv_xml_register_subject(NULL, "subject_hour", &subject_hour);
    lv_xml_register_subject(NULL, "subject_minute", &subject_minute);
    lv_xml_register_subject(NULL, "subject_day_index", &subject_day_index);
    lv_xml_register_subject(NULL, "subject_day_name", &subject_day_name);
    lv_xml_register_subject(NULL, "subject_month_name", &subject_month_name);
    lv_xml_register_subject(NULL, "subject_temp", &subject_temp);
    lv_xml_register_subject(NULL, "subject_test", &subject_test);

    /* Register callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
    lv_xml_register_image(NULL, "icon_check", icon_check);
    lv_xml_register_image(NULL, "icon_minus", icon_minus);
    lv_xml_register_image(NULL, "icon_plus", icon_plus);
    lv_xml_register_image(NULL, "icon_wifi", icon_wifi);
    lv_xml_register_image(NULL, "image_alarm", image_alarm);
    lv_xml_register_image(NULL, "image_weather_foggy", image_weather_foggy);
#endif

#if LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manaully from XML using lv_xml_create() */
#endif
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/