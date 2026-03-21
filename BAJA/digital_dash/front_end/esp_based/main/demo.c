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

int cvt_temp = 30;
int vehicle_speed = 11;

static esp_err_t app_lvgl_main(void)
{
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return ESP_FAIL;
    }
   

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);

	lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));

    lv_obj_set_style_bg_color(lv_obj_0, lv_color_white(), 0);

    lv_obj_t * button_1 = lv_button_create(lv_obj_0);
    lv_obj_t * button_1_label = lv_label_create(button_1);
    lv_label_set_text(button_1_label, "Settings");
    lv_obj_center(button_1_label);

	lv_obj_set_x(button_1, 228);
    lv_obj_set_y(button_1, 5);
    
    lv_obj_t * temp_slide = lv_slider_create(lv_obj_0);

	lv_obj_set_x(temp_slide, 7);
    lv_obj_set_y(temp_slide, 190);
    lv_obj_set_width(temp_slide, 131);
    lv_obj_set_height(temp_slide, 18);
    lv_obj_set_style_transform_rotation(temp_slide, -900, 0);
    
    lv_obj_t * h4_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(h4_1, "CVT   Temperature");
    lv_obj_set_style_text_color(h4_1, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(h4_1, 10);
    lv_obj_set_y(h4_1, 210);
    
    lv_obj_t * arc_1 = lv_arc_create(lv_obj_0);

	lv_obj_set_x(arc_1, 144);
    lv_obj_set_y(arc_1, 55);
    lv_obj_set_width(arc_1, 170);
    lv_obj_set_height(arc_1, 162);
    
    lv_obj_t * h3_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(h3_1, "Speed");
    lv_obj_set_style_text_color(h3_1, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(h3_1, 200);
    lv_obj_set_y(h3_1, 210);
    
    lv_obj_t * temp_value = lv_label_create(lv_obj_0);
    lv_label_set_text(temp_value, "temp val");
    lv_obj_set_style_text_color(temp_value, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(temp_value, 38);
    lv_obj_set_y(temp_value, 160);
    lv_obj_set_width(temp_value, 79);
    lv_obj_set_height(temp_value, 48);

    
    
    lv_obj_t * km_value = lv_label_create(lv_obj_0);
    lv_label_set_text(km_value, "speed val");
    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(km_value, 178);
    lv_obj_set_y(km_value, 130);


    lv_screen_load(lv_obj_0); // remeber to actually load the screen!!!
    

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