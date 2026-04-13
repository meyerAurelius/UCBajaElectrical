//UI design trial

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
#include "nvs_flash.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"


#include "espnow_example_main.c"

#include "esp_now.h"

#include "espnow_example.h"

#include "demo_update.h"

#include "controller.h"

/* typedef enum {                  //set up screen switching
    ENDURANCE_SCREEN
    DRIVER_SCREEN
} screen_t; */

// image declaration
LV_IMAGE_DECLARE(engine_temp_light);

static lv_obj_t *lbl_counter = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t * temp_slide = NULL;
static lv_obj_t *lap_num = NULL;
static lv_obj_t *lap_time = NULL;
static lv_obj_t *brake_pressure = NULL;
static lv_obj_t *title_1 = NULL;

static lv_obj_t *endurance_screen = NULL;
static lv_obj_t *driver_screen = NULL;
int current_screen = 0;
bool request_screen_switch = false;

// thermistor esp mac 94:A9:90:0B:2A:04
static uint8_t s_peer_mac[6] =  { 0x94, 0xA9, 0x90, 0x0B, 0x2A, 0x04 };



static lv_obj_t *endurance_display(void)                     //gps speed, temperatures, maybe headin
{             

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);

	lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));

    lv_obj_set_style_bg_color(lv_obj_0, lv_color_white(), 0);

    //temperature

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

    // lv_obj_t * h4_1 = lv_label_create(lv_obj_0);
    // lv_label_set_text(h4_1, "CVT   Temperature");
    // lv_obj_set_style_text_color(h4_1, lv_color_black(), LV_PART_MAIN);

	// lv_obj_set_x(h4_1, 10);
    // lv_obj_set_y(h4_1, 210);

    lv_color_t orange_red = lv_color_make(245, 84, 66);

 /*    
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
    lv_obj_set_y(min_temp_label, 205); */


    //temp scale
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
    


    //speedometer
    lv_obj_t * arc_1 = lv_arc_create(lv_obj_0);           

	lv_obj_set_x(arc_1, 144);
    lv_obj_set_y(arc_1, 55);
    lv_obj_set_width(arc_1, 170);
    lv_obj_set_height(arc_1, 162);

    lv_obj_set_style_arc_color(arc_1, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_1, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_1, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_1, 6, LV_PART_INDICATOR);

    
    //speedometer label
    lv_obj_t * h3_1 = lv_label_create(lv_obj_0);     
    lv_label_set_text(h3_1, "Speed");
    lv_obj_set_style_text_color(h3_1, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(h3_1, 200);
    lv_obj_set_y(h3_1, 210);


    //speedometer scale
    lv_obj_t *speed_scale = lv_scale_create(lv_obj_0);            
    lv_obj_set_size(speed_scale, 170, 162);
    lv_obj_align_to(speed_scale, arc_1, LV_ALIGN_CENTER, 0, 0);
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
   
    
    //estimated range (50km/h)

    
    temp_label = lv_label_create(lv_obj_0);                     //temperature values
    char temp_val[12];
    snprintf(temp_val, sizeof(temp_val), "%.2f", recv_temp[0]);
    strcat(temp_val, " °C");
    lv_label_set_text(temp_label, temp_val);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_22, 0);

	lv_obj_set_x(temp_label, 50);
    lv_obj_set_y(temp_label, 85);
    lv_obj_set_width(temp_label, 130);
    lv_obj_set_height(temp_label, 48);

     
    lv_obj_t * km_value = lv_label_create(lv_obj_0);                    //speed values
    lv_label_set_text(km_value, "speed val");
    lv_obj_set_style_text_color(km_value, lv_color_black(), LV_PART_MAIN);

	lv_obj_set_x(km_value, 178);
    lv_obj_set_y(km_value, 130);


    return lv_obj_0;
}


static lv_obj_t *driver_display(void)                //lap number, race time, brake pressure
{
    lv_obj_t *lv_obj_1 = lv_obj_create(NULL);

	lv_obj_set_width(lv_obj_1, lv_pct(100));
    lv_obj_set_height(lv_obj_1, lv_pct(100));

    lv_obj_set_style_bg_color(lv_obj_1, lv_color_white(), 0);

    title_1 = lv_label_create(lv_obj_1);
    lv_label_set_text(title_1, "TIME:");
    lv_obj_align(title_1, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_text_font(title_1, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title_1, lv_color_black(), LV_PART_MAIN);
    

    lap_time = lv_label_create(lv_obj_1);             //race time
    lv_label_set_text(lap_time, "00:00");
    lv_obj_align(lap_time, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_font(lap_time, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(lap_time, lv_color_black(), LV_PART_MAIN);

    lap_num = lv_label_create(lv_obj_1);                  //lap tracking
    lv_label_set_text(lap_num, "lap #"); //add actual lap tracking
    lv_obj_align(lap_num, LV_ALIGN_BOTTOM_LEFT, 30, -60);
    lv_obj_set_style_text_font(lap_num, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lap_num, lv_color_black(), LV_PART_MAIN);


    brake_pressure = lv_label_create(lv_obj_1);       //brake pressre (PSI, 2 dec)
    lv_label_set_text(brake_pressure, "0000.00 PSI");
    lv_obj_align(brake_pressure, LV_ALIGN_BOTTOM_RIGHT, -30, -60);
    lv_obj_set_style_text_font(brake_pressure, &lv_font_montserrat_22, 0);    
    lv_obj_set_style_text_color(brake_pressure, lv_color_black(), LV_PART_MAIN);  
    
    return lv_obj_1;

}

void switch_screens(void)               //switching between two screens
{
    if (current_screen == 0) {

        lv_screen_load(driver_screen);
        current_screen = 1;

    } else {

        lv_screen_load(endurance_screen);
        current_screen = 0;
    }
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

static void gui_task(void* )
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


    if (lvgl_port_lock(0)) {

        endurance_screen = endurance_display();
        driver_screen = driver_display();

        lv_screen_load(endurance_screen);
        current_screen = 0;

        lvgl_port_unlock();

    }


    while (1) {
    
        snprintf(buf, sizeof(buf), "%04u", n++);

        if (lvgl_port_lock(0)) {
            // code related to update must be inside here to be threadsafe
            // watchdog timer triggers otherwise 

            if (request_screen_switch) {
                switch_screens();
                request_screen_switch = false;
            }
/* 
            if (current_screen == 0 && temp_label != NULL) {                //update endurance display only

            // accessing recv_temp should be accessed in a threadsafe way (todo)
                if (recv_temp[0] == -1.0){
                    lv_label_set_text(temp_label, "NOT CONNECTED!");
                    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
            }   else {
                    char temp_val[12];
                    snprintf(temp_val, sizeof(temp_val), "%d", (int)recv_temp[0]);
                    strcat(temp_val, " °C");
                    lv_label_set_text(temp_label, temp_val);
                    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_30, 0);

                    lv_slider_set_value(temp_slide, (int)recv_temp[0], LV_ANIM_ON);
            }

            }

            if (current_screen == 1 && lap_num != NULL) {              //update driver display

                if (lap_num != NULL) {

                    //real data
                }

                if (lap_time != NULL) {

                    //real data
                }

                if (brake_pressure != NULL) {

                    //real data
                }
            } */
        
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




    
    comms_task();
    controller_start();

    xTaskCreatePinnedToCore(gui_task, "gui_task", 32768, NULL, 5, NULL, 1);



}

































