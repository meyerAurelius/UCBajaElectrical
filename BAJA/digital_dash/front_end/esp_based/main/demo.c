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
//#include "touch.h"
#include "nvs_flash.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"


#include "espnow_example_main.c"

#include "esp_now.h"

#include "espnow_example.h"


// image declaration
LV_IMAGE_DECLARE(engine_temp_light);

static lv_obj_t *lbl_counter = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t * temp_slide = NULL;

// thermistor esp mac 94:A9:90:0B:2A:04
static uint8_t s_peer_mac[6] =  { 0x94, 0xA9, 0x90, 0x0B, 0x2A, 0x04 };

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
    
    temp_slide = lv_slider_create(lv_obj_0);

	lv_obj_set_x(temp_slide, 7);
    lv_obj_set_y(temp_slide, 190);
    lv_obj_set_width(temp_slide, 131);
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

    // lv_obj_t * h4_1 = lv_label_create(lv_obj_0);
    // lv_label_set_text(h4_1, "CVT   Temperature");
    // lv_obj_set_style_text_color(h4_1, lv_color_black(), LV_PART_MAIN);

	// lv_obj_set_x(h4_1, 10);
    // lv_obj_set_y(h4_1, 210);

    lv_color_t orange_red = lv_color_make(245, 84, 66);

    
    // upper bound limit indicator on slider
    lv_obj_t *max_temp_label = lv_label_create(lv_obj_0);
    lv_label_set_text(max_temp_label, "125°C");
    lv_obj_set_style_text_color(max_temp_label, orange_red, LV_PART_MAIN);
    lv_obj_set_x(max_temp_label, 7);
    lv_obj_set_y(max_temp_label, 35);

    // lower bound
    lv_obj_t *min_temp_label = lv_label_create(lv_obj_0);
    lv_label_set_text(min_temp_label, "0°C");
    lv_obj_set_style_text_color(min_temp_label, orange_red, LV_PART_MAIN);
    lv_obj_set_x(min_temp_label, 7);
    lv_obj_set_y(min_temp_label, 205);


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
    
    temp_label = lv_label_create(lv_obj_0);
    char temp_val[12];
    snprintf(temp_val, sizeof(temp_val), "%.2f", recv_arr[0]);
    strcat(temp_val, " °C");
    lv_label_set_text(temp_label, temp_val);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_30, 0);

	lv_obj_set_x(temp_label, 38);
    lv_obj_set_y(temp_label, 80);
    lv_obj_set_width(temp_label, 130);
    lv_obj_set_height(temp_label, 48);

    
    
    lv_obj_t * km_value = lv_label_create(lv_obj_0);
    lv_label_set_text(km_value, "speed val");
    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(km_value, 178);
    lv_obj_set_y(km_value, 130);


    lv_screen_load(lv_obj_0); // remember to actually load the screen!!!
    

    lvgl_port_unlock();


    return ESP_OK;
}


static void comms_task(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    example_wifi_init();
    
    ESP_ERROR_CHECK(example_espnow_init());
}

static void gui_task(void* ){
        
        esp_lcd_panel_io_handle_t lcd_io = NULL;
        esp_lcd_panel_handle_t lcd_panel = NULL;
        //esp_lcd_touch_handle_t tp = NULL;
        //lvgl_port_touch_cfg_t touch_cfg = {0};
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

        //ESP_ERROR_CHECK(touch_init(&tp));

        //touch_cfg.disp = lvgl_display;
        //touch_cfg.handle = tp;
        //lvgl_port_add_touch(&touch_cfg);

        ESP_ERROR_CHECK(lcd_display_brightness_set(100));
        ESP_ERROR_CHECK(lcd_display_rotate(lvgl_display, LV_DISPLAY_ROTATION_90));

        ESP_ERROR_CHECK(app_lvgl_main());


        while (1) {
       
        snprintf(buf, sizeof(buf), "%04u", n++);

        if (lvgl_port_lock(0)) {
            // code related to update must be inside here to be threadsafe
            // watchdog timer triggers otherwise 

            if (lbl_counter != NULL) {
                lv_label_set_text(lbl_counter, buf);
            }
            
            // accessing recv_arr should be accessed in a threadsafe way (todo)
            if (recv_arr[0] == -1.0){
                lv_label_set_text(temp_label, "NOT CONNECTED!");
                lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
            } else{
                char temp_val[12];
                snprintf(temp_val, sizeof(temp_val), "%d", (int)recv_arr[0]);
                strcat(temp_val, " °C");
                lv_label_set_text(temp_label, temp_val);
                lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_30, 0);

                lv_slider_set_value(temp_slide, (int)recv_arr[0], LV_ANIM_ON);
            }
        

            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(125));
    }
}


void app_main(void)
{

    uint8_t mac[6];
    // ESP_MAC_WIFI_STA can be changed to ESP_MAC_BASE, ESP_MAC_BT, etc.
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
        if (ret == ESP_OK) {
            printf("STA MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else {
            printf("Failed to read STA MAC address\n");
        }

        ret = esp_read_mac(mac, ESP_MAC_BASE);

        if (ret == ESP_OK){
            printf("STA MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else {
            printf("Failed to read BASE MAC address\n");
        }



    ESP_ERROR_CHECK(espnow_sd_logger_start());
    
    comms_task();



    xTaskCreatePinnedToCore(gui_task, "gui_task", 12384, NULL, 5, NULL, 1);



}