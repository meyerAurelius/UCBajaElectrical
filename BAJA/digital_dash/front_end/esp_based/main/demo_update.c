// UI design trial

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_check.h>

#include <lvgl.h>
#include <esp_lvgl_port.h>

#include "lcd.h"
//#include "touch.h"
#include "nvs_flash.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "espnow_example_main.c"

#include "esp_now.h"

#include "espnow_example.h"

#include "demo_update.h"

#include "controller.h"

// image declaration
LV_IMAGE_DECLARE(engine_temp_light);
LV_IMAGE_DECLARE(pressure_icon);
LV_IMAGE_DECLARE(flag_icon);

static lv_obj_t *lbl_counter = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t *temp_slide = NULL;
static lv_obj_t *lap_num = NULL;
static lv_obj_t *lap_time = NULL;
static lv_obj_t *brake_pressure = NULL;
static lv_obj_t *title_1 = NULL;

static lv_obj_t *speed_arc = NULL;
static lv_obj_t *speed_scale = NULL;

static lv_obj_t *km_value = NULL;

static lv_obj_t *endurance_screen = NULL;
static lv_obj_t *driver_screen = NULL;

int current_screen = 0;
volatile screen_t requested_screen = SCREEN_NONE;

// thermistor esp mac 94:A9:90:0B:2A:04
static uint8_t s_peer_mac[6] = { 0x94, 0xA9, 0x90, 0x0B, 0x2A, 0x04 };


static lv_obj_t *endurance_display(void) // gps speed + temperature
{
    lv_obj_t *lv_obj_0 = lv_obj_create(NULL);

    lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_white(), 0);

    // temperature slider
    temp_slide = lv_slider_create(lv_obj_0);

    lv_obj_set_x(temp_slide, 7);
    lv_obj_set_y(temp_slide, 190);
    lv_obj_set_width(temp_slide, 150);
    lv_obj_set_height(temp_slide, 18);
    lv_obj_set_style_transform_rotation(temp_slide, -900, 0);
    lv_slider_set_range(temp_slide, 0, 125);

    // adjust knob size
    lv_obj_set_style_pad_left(temp_slide, -10, LV_PART_KNOB);
    lv_obj_set_style_pad_right(temp_slide, -10, LV_PART_KNOB);
    lv_obj_set_style_pad_top(temp_slide, -10, LV_PART_KNOB);
    lv_obj_set_style_pad_bottom(temp_slide, -10, LV_PART_KNOB);

    // temp icon
    lv_obj_t *temp_icon = lv_image_create(lv_obj_0);
    lv_image_set_src(temp_icon, &engine_temp_light);
    lv_obj_set_x(temp_icon, 40);
    lv_obj_set_y(temp_icon, 120);

    lv_color_t orange_red = lv_color_make(245, 84, 66);
    (void)orange_red;

    // temp scale
    lv_obj_t *temp_scale = lv_scale_create(lv_obj_0);
    lv_obj_set_size(temp_scale, 18, 150);
    lv_scale_set_range(temp_scale, 0, 125);
    lv_scale_set_mode(temp_scale, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_obj_align_to(temp_scale, temp_slide, LV_ALIGN_CENTER, -60, -82);
    lv_scale_set_total_tick_count(temp_scale, 26);
    lv_scale_set_major_tick_every(temp_scale, 5);
    lv_scale_set_label_show(temp_scale, true);

    lv_obj_set_style_line_color(temp_scale, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_line_color(temp_scale, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(temp_scale, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(temp_scale, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(temp_scale, lv_color_black(), LV_PART_ITEMS | LV_STATE_DEFAULT);

    // speedometer arc
    speed_arc = lv_arc_create(lv_obj_0);

    lv_obj_set_x(speed_arc, 155);
    lv_obj_set_y(speed_arc, 55);
    lv_obj_set_width(speed_arc, 150);
    lv_obj_set_height(speed_arc, 142);

    lv_arc_set_range(speed_arc, 0, 50);
    lv_arc_set_value(speed_arc, 0);

    lv_obj_set_style_arc_color(speed_arc, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_arc_color(speed_arc, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(speed_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(speed_arc, 6, LV_PART_INDICATOR);

    // speedometer label
    lv_obj_t *h3_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(h3_1, "Speed");
    lv_obj_set_style_text_color(h3_1, lv_color_black(), LV_PART_MAIN);

    lv_obj_set_x(h3_1, 200);
    lv_obj_set_y(h3_1, 210);

    // speedometer scale
    speed_scale = lv_scale_create(lv_obj_0);
    lv_obj_set_size(speed_scale, 150, 142);
    lv_obj_align_to(speed_scale, speed_arc, LV_ALIGN_CENTER, 0, 0);
    lv_scale_set_mode(speed_scale, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_label_show(speed_scale, true);
    lv_scale_set_total_tick_count(speed_scale, 11);
    lv_scale_set_major_tick_every(speed_scale, 5);
    lv_scale_set_range(speed_scale, 0, 50);

    lv_obj_set_style_line_color(speed_scale, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_line_color(speed_scale, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(speed_scale, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(speed_scale, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(speed_scale, lv_color_black(), LV_PART_ITEMS | LV_STATE_DEFAULT);

    // temperature value label
    temp_label = lv_label_create(lv_obj_0);

    char temp_val[16];
    snprintf(temp_val, sizeof(temp_val), "%.2f °C", recv_arr[0]);

    lv_label_set_text(temp_label, temp_val);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_22, 0);

    lv_obj_set_x(temp_label, 50);
    lv_obj_set_y(temp_label, 85);
    lv_obj_set_width(temp_label, 130);
    lv_obj_set_height(temp_label, 48);

    // speed value label
    km_value = lv_label_create(lv_obj_0);
    lv_label_set_text(km_value, "0 KM/H");
    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

    lv_obj_set_x(km_value, 178);
    lv_obj_set_y(km_value, 120);

    return lv_obj_0;
}


static lv_obj_t *driver_display(void) // lap number, race time, brake pressure
{
    lv_obj_t *lv_obj_1 = lv_obj_create(NULL);

    lv_obj_set_width(lv_obj_1, lv_pct(100));
    lv_obj_set_height(lv_obj_1, lv_pct(100));

    lv_obj_set_style_bg_color(lv_obj_1, lv_color_white(), 0);

    // brake pressure icon
    lv_obj_t *brake_icon = lv_image_create(lv_obj_1);
    lv_image_set_src(brake_icon, &pressure_icon);
    lv_obj_align(brake_icon, LV_ALIGN_BOTTOM_RIGHT, -90, -55);

    // lap number icon
    lv_obj_t *lap_icon = lv_image_create(lv_obj_1);
    lv_image_set_src(lap_icon, &flag_icon);
    lv_obj_align(lap_icon, LV_ALIGN_BOTTOM_LEFT, 60, -30);

    // race time labels
    lv_obj_t *title_1 = lv_label_create(lv_obj_1);
    lv_label_set_text(title_1, "TIME:");
    lv_obj_align(title_1, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_text_font(title_1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_1, lv_color_black(), LV_PART_MAIN);

    // race time
    lap_time = lv_label_create(lv_obj_1);
    lv_label_set_text(lap_time, "00:00");
    lv_obj_align(lap_time, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_font(lap_time, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(lap_time, lv_color_black(), LV_PART_MAIN);

    // lap number label
    lv_obj_t *title_2 = lv_label_create(lv_obj_1);
    lv_label_set_text(title_2, "LAP:");
    lv_obj_align(title_2, LV_ALIGN_BOTTOM_LEFT, 30, -60);
    lv_obj_set_style_text_font(title_2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_2, lv_color_black(), LV_PART_MAIN);

    // lap number
    lap_num = lv_label_create(lv_obj_1);
    lv_label_set_text(lap_num, "lap #");
    lv_obj_align(lap_num, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    lv_obj_set_style_text_font(lap_num, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lap_num, lv_color_black(), LV_PART_MAIN);

    // brake pressure label
    lv_obj_t *title_3 = lv_label_create(lv_obj_1);
    lv_label_set_text(title_3, "BRAKE:");
    lv_obj_align(title_3, LV_ALIGN_BOTTOM_RIGHT, -30, -60);
    lv_obj_set_style_text_font(title_3, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_3, lv_color_black(), LV_PART_MAIN);

    // brake pressure
    brake_pressure = lv_label_create(lv_obj_1);
    lv_label_set_text(brake_pressure, "0000.00 PSI");
    lv_obj_align(brake_pressure, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    lv_obj_set_style_text_font(brake_pressure, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(brake_pressure, lv_color_black(), LV_PART_MAIN);

    return lv_obj_1;
}


static void comms_task(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    example_wifi_init();

    ESP_ERROR_CHECK(example_espnow_init());
}


static void gui_task(void *arg)
{
    (void)arg;

    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;
    //esp_lcd_touch_handle_t tp = NULL;
    //lvgl_port_touch_cfg_t touch_cfg = {0};
    lv_display_t *lvgl_display = NULL;

    ESP_ERROR_CHECK(lcd_display_brightness_init());

    ESP_ERROR_CHECK(app_lcd_init(&lcd_io, &lcd_panel));

    lvgl_display = app_lvgl_init(lcd_io, lcd_panel);
    if (lvgl_display == NULL) {
        ESP_LOGE(TAG, "Fatal error in app_lvgl_init");
        esp_restart();
    }

    //ESP_ERROR_CHECK(touch_init(&tp));

    //touch_cfg.disp = lvgl_display;
    //touch_cfg.handle = tp;
    //lvgl_port_add_touch(&touch_cfg);

    ESP_ERROR_CHECK(lcd_display_brightness_set(100));

    if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
        ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_90));

        endurance_screen = endurance_display();
        driver_screen = driver_display();

        lv_screen_load(endurance_screen);
        current_screen = SCREEN_ENDURANCE;

        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL during display setup");
        esp_restart();
    }

    while (1) {
        if (lvgl_port_lock(0)) {

            if (requested_screen != SCREEN_NONE && requested_screen != current_screen) {
                switch (requested_screen) {
                    case SCREEN_ENDURANCE:
                        lv_screen_load(endurance_screen);
                        break;

                    case SCREEN_DRIVER:
                        lv_screen_load(driver_screen);
                        break;

                    case SCREEN_NONE:
                    default:
                        break;
                }

                current_screen = requested_screen;
                requested_screen = SCREEN_NONE;
            }

            if (
                current_screen == SCREEN_ENDURANCE &&
                temp_label != NULL &&
                temp_slide != NULL &&
                speed_arc != NULL &&
                km_value != NULL
            ) {
                if (recv_arr[0] == -1.0) {
                    lv_label_set_text(temp_label, "NOT CONNECTED!");
                    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);

                    lv_label_set_text(km_value, "NOT CONNECTED!");
                    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

                    lv_slider_set_value(temp_slide, 0, LV_ANIM_OFF);
                    lv_arc_set_value(speed_arc, 0);
                } else {
                    int temp = (int)recv_arr[0];
                    int speed = (int)recv_arr[1];

                    if (temp < 0) {
                        temp = 0;
                    } else if (temp > 125) {
                        temp = 125;
                    }

                    if (speed < 0) {
                        speed = 0;
                    } else if (speed > 50) {
                        speed = 50;
                    }

                    char temp_val[16];
                    snprintf(temp_val, sizeof(temp_val), "%d °C", temp);
                    lv_label_set_text(temp_label, temp_val);
                    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_30, 0);

                    lv_slider_set_value(temp_slide, temp, LV_ANIM_OFF);

                    char speed_val[16];
                    snprintf(speed_val, sizeof(speed_val), "%d KM/H", speed);
                    lv_label_set_text(km_value, speed_val);
                    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

                    lv_arc_set_value(speed_arc, speed);
                }
            }

            /*
            if (current_screen == SCREEN_DRIVER && lap_num != NULL) {
                // update driver display here later
            }
            */

            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(125));
    }
}


void app_main(void)
{
    uint8_t mac[6];

    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);

    if (ret == ESP_OK) {
        printf("STA MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        printf("Failed to read STA MAC address\n");
    }

    ret = esp_read_mac(mac, ESP_MAC_BASE);

    if (ret == ESP_OK) {
        printf("BASE MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        printf("Failed to read BASE MAC address\n");
    }

    comms_task();
    controller_start();

    ESP_ERROR_CHECK(espnow_sd_logger_start());

    xTaskCreatePinnedToCore(gui_task, "gui_task", 32768, NULL, 5, NULL, 1);
}
