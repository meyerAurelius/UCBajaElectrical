#include <stdio.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

#include <lvgl.h>
#include <esp_lvgl_port.h>

#include "lcd.h"
#include "touch.h"

static const char *TAG = "demo";

static lv_obj_t *lbl_counter = NULL;

/* Button click event */
static void ui_event_button_1(lv_event_t *e)
{
    static uint8_t pos = 0;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        /* Cycle through a few built-in alignments */
        static const lv_align_t aligns[] = {
            LV_ALIGN_TOP_LEFT,
            LV_ALIGN_TOP_MID,
            LV_ALIGN_TOP_RIGHT,
            LV_ALIGN_LEFT_MID,
            LV_ALIGN_CENTER,
            LV_ALIGN_RIGHT_MID,
            LV_ALIGN_BOTTOM_LEFT,
            LV_ALIGN_BOTTOM_MID,
            LV_ALIGN_BOTTOM_RIGHT
        };

        lv_obj_align(btn, aligns[pos], 0, 0);
        pos = (pos + 1) % (sizeof(aligns) / sizeof(aligns[0]));
    }
}

static esp_err_t app_lvgl_main(void)
{
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return ESP_FAIL;
    }

    /* Create and load a new screen */
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_screen_load(scr);

    /* ---------------- Button: Settings ---------------- */
    lv_obj_t *button_1 = lv_button_create(scr);
    lv_obj_set_size(button_1, 90, 40);
    lv_obj_set_pos(button_1, 228, 5);
    lv_obj_add_event_cb(button_1, ui_event_button_1, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_label = lv_label_create(button_1);
    lv_label_set_text(button_label, "Settings");
    lv_obj_center(button_label);

    /* ---------------- Slider: Temperature ---------------- */
    lv_obj_t *temp_slide = lv_slider_create(scr);
    lv_obj_set_pos(temp_slide, 47, 199);
    lv_obj_set_size(temp_slide, 173, 23);
    lv_slider_set_range(temp_slide, 0, 100);
    lv_slider_set_value(temp_slide, 50, LV_ANIM_OFF);

    /* Rotate slider by -90.0 degrees
       LVGL uses 0.1 degree units, so -900 = -90.0 deg */
    lv_obj_set_style_transform_rotation(temp_slide, -900, 0);

    lv_obj_t *temp_label = lv_label_create(scr);
    lv_label_set_text(temp_label, "Temperature");
    lv_obj_set_pos(temp_label, 16, 208);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), 0);

    /* ---------------- Arc: Speed ---------------- */
    lv_obj_t *arc_1 = lv_arc_create(scr);
    lv_obj_set_pos(arc_1, 136, 59);
    lv_obj_set_size(arc_1, 153, 135);
    lv_arc_set_range(arc_1, 0, 180);
    lv_arc_set_value(arc_1, 80);
    lv_arc_set_rotation(arc_1, 135);
    lv_arc_set_bg_angles(arc_1, 0, 270);

    lv_obj_t *speed_label = lv_label_create(scr);
    lv_label_set_text(speed_label, "Speed");
    lv_obj_set_pos(speed_label, 183, 194);
    lv_obj_set_style_text_color(speed_label, lv_color_black(), 0);

    /* ---------------- Counter label ---------------- */
    lbl_counter = lv_label_create(scr);
    lv_label_set_text(lbl_counter, "0000");
    lv_obj_set_pos(lbl_counter, 10, 10);
    lv_obj_set_style_text_color(lbl_counter, lv_color_black(), 0);

    lvgl_port_unlock();
    return ESP_OK;
}

void app_main(void)
{
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;
    esp_lcd_touch_handle_t tp = NULL;
    lvgl_port_touch_cfg_t touch_cfg = {0};
    lv_display_t *lvgl_display = NULL;

    char buf[16];
    uint16_t n = 0;

    ESP_ERROR_CHECK(lcd_display_brightness_init());

    ESP_ERROR_CHECK(app_lcd_init(&lcd_io, &lcd_panel));

    lvgl_display = app_lvgl_init(lcd_io, lcd_panel);
    if (lvgl_display == NULL) {
        ESP_LOGE(TAG, "Fatal error in app_lvgl_init");
        esp_restart();
    }

    ESP_ERROR_CHECK(touch_init(&tp));

    touch_cfg.disp = lvgl_display;
    touch_cfg.handle = tp;
    lvgl_port_add_touch(&touch_cfg);

    ESP_ERROR_CHECK(lcd_display_brightness_set(100));
    ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_90));

    ESP_ERROR_CHECK(app_lvgl_main());

    while (1) {
        snprintf(buf, sizeof(buf), "%04u", n++);

        if (lvgl_port_lock(0)) {
            if (lbl_counter != NULL) {
                lv_label_set_text(lbl_counter, buf);
            }
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(125));
    }
}