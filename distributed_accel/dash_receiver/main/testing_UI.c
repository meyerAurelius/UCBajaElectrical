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

#include <sys/stat.h>


static lv_obj_t *testing_screen = NULL;
static lv_obj_t *time_label = NULL;

static lv_obj_t *results_screen = NULL;
static lv_obj_t *results_title = NULL;
static lv_obj_t *size_label = NULL;


static void stop_btn_event(lv_event_t *e)       //when stop buttons are pressed
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;       //if the button wasn't clicked, do nothing
    const char *which_test = (const char *)lv_event_get_user_data(e);       //so we can print which test completed
    
    lv_label_set_text_fmt(results_title, "%s stopped", which_test);

    //READING SESSION SIZE
    struct stat st;
    if (stat("/sdcard/LOG.CSV", &st) == 0) {
    lv_label_set_text_fmt(size_label, "Session size: %lu KB", (unsigned long)(st.st_size / 1024));
    }
    
    lv_screen_load(results_screen);
}



static void return_btn_event(lv_event_t *e)     //return button on results screen to go back to main display
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_screen_load(testing_screen);
}



static lv_obj_t *testing_display(void) // CVT + accel tests
{
    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);

	lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));

    lv_obj_set_style_bg_color(lv_obj_0, lv_color_white(), 0);

    //Time elapsed label
    time_label = lv_label_create(lv_obj_0);
    lv_label_set_text(time_label, "Elapsed Time: --");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(time_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_width(time_label, 140);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 8);


    //Accel start button
    lv_obj_t * a_start = lv_button_create(lv_obj_0);
    lv_obj_t * a_start_label = lv_label_create(a_start);
    lv_label_set_text(a_start_label, "Accel Start"); 
    lv_obj_center(a_start_label);
    lv_obj_set_size(a_start, 140, 80);
    lv_obj_align(a_start, LV_ALIGN_TOP_LEFT, 15, 45);
	

    //Accel stop button
    lv_obj_t * a_stop = lv_button_create(lv_obj_0);
    lv_obj_t * a_stop_label = lv_label_create(a_stop);
    lv_label_set_text(a_stop_label, "Accel Stop");
    lv_obj_center(a_stop_label);
    lv_obj_set_size(a_stop, 140, 80);
    lv_obj_align(a_stop, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    lv_obj_add_event_cb(a_stop, stop_btn_event, LV_EVENT_CLICKED, (void *)"Accel");


    //CVT start button
    lv_obj_t * CVT_start = lv_button_create(lv_obj_0);
    lv_obj_t * CVT_start_label = lv_label_create(CVT_start);
    lv_label_set_text(CVT_start_label, "CVT Start");
    lv_obj_center(CVT_start_label);
    lv_obj_set_size(CVT_start, 140, 80);
    lv_obj_align(CVT_start, LV_ALIGN_TOP_RIGHT, -15, 45);


    //CVT stop button
    lv_obj_t * CVT_stop = lv_button_create(lv_obj_0);
    lv_obj_t * CVT_stop_label = lv_label_create(CVT_stop);
    lv_label_set_text(CVT_stop_label, "CVT stop");
    lv_obj_center(CVT_stop_label);
    lv_obj_set_size(CVT_stop, 140, 80);
    lv_obj_align(CVT_stop, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
    lv_obj_add_event_cb(CVT_stop, stop_btn_event, LV_EVENT_CLICKED, (void *)"CVT");
    

    return lv_obj_0;
}

static lv_obj_t *results_display(void) // Results screen
{ 
    lv_obj_t *lv_obj_1 = lv_obj_create(NULL); 
    lv_obj_set_style_bg_color(lv_obj_1, lv_color_white(), 0);

    //results title
    results_title = lv_label_create(lv_obj_1);
    lv_label_set_text(results_title, "Test Complete");
    lv_obj_set_style_text_font(results_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(results_title, lv_color_black(), 0);
    lv_obj_align(results_title, LV_ALIGN_TOP_MID, 0, 40);

    //size_label
    size_label = lv_label_create(lv_obj_1);
    lv_label_set_text(size_label, "Session Size: --");
    lv_obj_set_style_text_font(size_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(size_label, lv_color_black(), 0);
    lv_obj_align(size_label, LV_ALIGN_CENTER, 0, -10);

    //return to main display button
    lv_obj_t * return_btn = lv_button_create(lv_obj_1);
    lv_obj_t * return_btn_label = lv_label_create(return_btn);
    lv_label_set_text(return_btn_label, "RETURN"); 
    lv_obj_center(return_btn_label);
    lv_obj_set_size(return_btn, 140, 80);
    lv_obj_align(return_btn, LV_ALIGN_BOTTOM_MID, 5, -5);
    lv_obj_add_event_cb(return_btn, return_btn_event, LV_EVENT_CLICKED, NULL);


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
       
        testing_screen = testing_display();
        results_screen = results_display();
        lv_screen_load(testing_screen);
    
        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL during display setup");
        esp_restart();
    }


    vTaskDelay(pdMS_TO_TICKS(5000));          // look at the main screen for 5 seconds

    if (lvgl_port_lock(0)) {
        lv_label_set_text(results_title, "Accel stopped");   // fake the callback's text
        lv_label_set_text(size_label, "Session size: 12 KB");
        lv_screen_load(results_screen);
        lvgl_port_unlock();
    }

   
    while (1) {
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
    

    ESP_ERROR_CHECK(espnow_sd_logger_start());

    xTaskCreatePinnedToCore(gui_task, "gui_task", 32768, NULL, 5, NULL, 1);
}
