// code for digital dash controller UART

#include "controller.h"

// from async_rxtxtasks example
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

          
#define RXD_PIN (CONFIG_EXAMPLE_UART_RXD)           //GPIO27
                                                    //set in menuconfig

void init(void) //driver install, communication parameters and pins
{
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_EXAMPLE_UART_BAUD_RATE,         //9600
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //signature: (uart_num, tx, rx, rts, cts)
}


static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);

    if (data == NULL) {                                                //
        ESP_LOGE(RX_TASK_TAG, "Failed to allocate RX buffer");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {            
            //button logic

            for (int i = 0; i < rxBytes; i++) {

                uint8_t button = data[i];

                button = button - '0';  //convert ASCII to number
                
                switch(button) {

                case 1: move_down(); break;

                case 2: select(); break;

                case 3: move_right(); break;

                case 4: move_left(); break;

                case 5: move_up(); break;

                default:
                    ESP_LOGW(RX_TASK_TAG, "Unidentified data: %d", button);
                    break;                 //if controller sends something not recognize

           } 
    
        } 

        }
    }
    free(data);                     //deallocates dynamically allocated memory
}


void app_main(void)                 
{
    init();                         //call communication parameters
    xTaskCreate(rx_task, "uart_rx_task", CONFIG_EXAMPLE_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

}


























