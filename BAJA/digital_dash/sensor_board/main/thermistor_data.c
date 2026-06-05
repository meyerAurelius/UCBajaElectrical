// this is the main file
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define THERM_CHANNEL ADC_CHANNEL_6
#define ADC_UNIT_USED ADC_UNIT_1
#define VCC 3.3
#define ADC_MAX 4095.0
#define SERIES_RESISTOR 10000.0
#define NOMINAL_RESISTANCE 10000.0
#define NOMINAL_TEMPERATURE 25.0
#define BETA_COEFFICIENT 3892.0

#include "espnow_example.h"
#include "espnow_example_main.c"

#include "esp_mac.h"

//#include "gps_module.h"
#include "gps_module.c"


//static uint8_t s_peer_mac[ESP_NOW_ETH_ALEN] =  { 0xEC, 0xE3, 0x34, 0xD6, 0x2B, 0xAC };



void app_main(void){
    uint8_t mac[6];

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

    

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_USED,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, THERM_CHANNEL, &config);

    
    ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    //wifi_init_sta();

    example_wifi_init();
    
    example_espnow_init();
    

    xTaskCreatePinnedToCore(&gpstask, "GPS", 4092*2, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&http_post_task, "http_post_task", 4092*2, NULL, 5, NULL, 0);


    while(1){

        int adc_raw = 0;
        int total = 0;

        for(int i = 0; i < 32; i++){
            adc_oneshot_read(adc_handle, THERM_CHANNEL, &adc_raw);
            total += adc_raw;
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        adc_raw = total /32;

        if(adc_raw <=0 || adc_raw >= ADC_MAX){
            printf("ADC reading out of range\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        double voltage = ((double)adc_raw / ADC_MAX) * VCC;

        double therm_res = SERIES_RESISTOR * ((VCC / voltage) - 1.0);

        double tempK = 1.0 / (
            (1.0 / (NOMINAL_TEMPERATURE + 273.15)) +
            (log(therm_res / NOMINAL_RESISTANCE) / BETA_COEFFICIENT)
        );

        tempC = tempK - 273.15;

        printf("ADC= %d Voltage= %.3f V Resistance=%.1f ohms Temp%.2f C \n",
        adc_raw, voltage, therm_res, tempC);

        
        send_arr[0] = tempC;
        send_arr[1] = gps_speed;
        
        //espnow_float_tx_send_array(val_arr, sizeof(val_arr));

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}