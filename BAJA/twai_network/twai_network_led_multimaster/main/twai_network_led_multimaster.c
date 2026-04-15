
/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*
 * The following example demonstrates a master node in a TWAI network. The master
 * node is responsible for initiating and stopping the transfer of data messages.
 * The example will execute multiple iterations, with each iteration the master
 * node will do the following:
 * 1) Start the TWAI driver
 * 2) Repeatedly send ping messages until a ping response from slave is received
 * 3) Send start command to slave and receive data messages from slave
 * 4) Send stop command to slave and wait for stop response from slave
 * 5) Stop the TWAI driver
 */
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define PING_PERIOD_MS          250
#define NO_OF_DATA_MSGS         50
#define NO_OF_ITERS             6
#define ITER_DELAY_MS           1000
#define RX_TASK_PRIO            9
#define TX_TASK_PRIO            8
#define CTRL_TSK_PRIO           10
#define TX_GPIO_NUM             21
#define RX_GPIO_NUM             22
#define EXAMPLE_TAG             "TWAI Master"

#define ID_MASTER_STOP_CMD      0x0A0
#define ID_MASTER_START_CMD     0x0A1
#define ID_MASTER_PING          0x0A2
#define ID_SLAVE_STOP_RESP      0x0B0
#define ID_SLAVE_DATA           0x0B1
#define ID_SLAVE_PING_RESP      0x0B2

#define ID_SLAVE_1              0x01
#define ID_SLAVE_2              0x02
#define ID_SLAVE_3              0x03

typedef enum {
    TX_SEND_PINGS,
    TX_SEND_START_CMD,
    TX_SEND_STOP_CMD,
    TX_TASK_EXIT,
} tx_task_action_t;

typedef enum {
    RX_RECEIVE_PING_RESP,
    RX_RECEIVE_DATA,
    RX_RECEIVE_STOP_RESP,
    RX_TASK_EXIT,
} rx_task_action_t;

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);



static const twai_message_t ping_message = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 1,                // Is single shot (won't retry on error or NACK)
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_MASTER_PING,
    .data_length_code = 0,
    .data = {0},
};



static const twai_message_t stop_message = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 0,                // Not single shot
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_MASTER_STOP_CMD,
    .data_length_code = 0,
    .data = {0},
};

static twai_message_t start_message = {
    .extd = 0,
    .rtr = 0,
    .ss = 0,
    .self = 0,
    .dlc_non_comp = 0,
    .identifier = ID_MASTER_START_CMD,
    .data_length_code = 3,   // 3 bytes: R,G,B
    .data = {0, 0, 0},
};

static twai_message_t start_message_multi = {
    .extd = 0,
    .rtr = 0,
    .ss = 0,
    .self = 0,
    .dlc_non_comp = 0,
    .identifier = ID_MASTER_START_CMD,
    .data_length_code = 4,   // 4 bytes: addr,R,G,B
    .data = {0, 0, 0, 0},
};

static QueueHandle_t tx_task_queue;
static QueueHandle_t rx_task_queue;
static SemaphoreHandle_t stop_ping_sem;
static SemaphoreHandle_t ctrl_task_sem;
static SemaphoreHandle_t done_sem;


static inline void send_start_with_color(uint8_t r, uint8_t g, uint8_t b) {
    start_message.data[0] = r;
    start_message.data[1] = g;
    start_message.data[2] = b;
    ESP_ERROR_CHECK(twai_transmit(&start_message, portMAX_DELAY));
    ESP_LOGI(EXAMPLE_TAG, "Sent START with color R=%u G=%u B=%u", r, g, b);
}

static void send_start_to_slave(uint8_t addr, uint8_t r, uint8_t g, uint8_t b)
{
    /*start_message_multi.data[0] = addr;
    start_message_multi.data[1] = r;
    start_message_multi.data[2] = g;
    start_message_multi.data[3] = b;

    tx_task_action_t tx_action = TX_SEND_START_CMD;
    xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
    ESP_LOGI(EXAMPLE_TAG,
             "Sent START to slave addr=0x%02X with color R=%u G=%u B=%u",
             addr, r, g, b);*/
    // Build a *local* message so each call has its own copy
    twai_message_t msg = {
        .extd = 0,
        .rtr = 0,
        .ss = 0,
        .self = 0,
        .dlc_non_comp = 0,
        .identifier = ID_MASTER_START_CMD,
        .data_length_code = 4,
        .data = { addr, r, g, b, 0, 0, 0, 0 },
    };
    ESP_ERROR_CHECK(twai_transmit(&msg, portMAX_DELAY));
    ESP_LOGI(EXAMPLE_TAG,
             "Sent START to slave addr=0x%02X with color R=%u G=%u B=%u",
             addr, r, g, b);
}



/* --------------------------- Tasks and Functions -------------------------- */

static void twai_receive_task(void *arg)
{
    
    ESP_LOGI(EXAMPLE_TAG,"Master Program Started!");
    
    while (1) {

        rx_task_action_t action;
        xQueueReceive(rx_task_queue, &action, portMAX_DELAY);

        if (action == RX_RECEIVE_PING_RESP) {
            bool slave1_found = false;
            bool slave2_found = false;
            bool slave3_found = false;
            TickType_t start_time = xTaskGetTickCount();

            //Listen for ping response from slave
            while (1) {
                while (!(slave1_found && slave2_found && slave3_found) && (xTaskGetTickCount() - start_time < pdMS_TO_TICKS(60000))){
                    twai_message_t rx_msg;
                    ESP_LOGI(EXAMPLE_TAG, "Looking for Interns");

                    if (twai_receive(&rx_msg, pdMS_TO_TICKS(500)) == ESP_OK){
                        ESP_LOGI(EXAMPLE_TAG, "RX frame: id=0x%03X dlc=%d", rx_msg.identifier, rx_msg.data_length_code);
                        if (rx_msg.identifier == ID_SLAVE_PING_RESP) {
                            slave1_found = true;
                            ESP_LOGI(EXAMPLE_TAG, "Slave 1 connected");

                        } else if (rx_msg.identifier == 0x0C2) {
                            slave2_found = true;
                            ESP_LOGI(EXAMPLE_TAG, "Slave 2 connected");

                        } else {
                            slave3_found = true;
                            ESP_LOGI(EXAMPLE_TAG, "Slave 3 connected");

                        }  
                    }
                }
                if (slave1_found || slave2_found || slave3_found){
                    xSemaphoreGive(stop_ping_sem);
                    xSemaphoreGive(ctrl_task_sem);
                    ESP_LOGI(EXAMPLE_TAG, "Ping phase complete (S1=%d, S2=%d, S3=%d)",
                        slave1_found, slave2_found, slave3_found);
                } else{
                    ESP_LOGW(EXAMPLE_TAG, "No slaves responded to ping");
                }            
                break;
            }
        } else if (action == RX_RECEIVE_DATA) {
            //Receive data messages from slave
            uint32_t data_msgs_rec = 0;
            while (data_msgs_rec < NO_OF_DATA_MSGS) {
                twai_message_t rx_msg;
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_SLAVE_DATA) {
                    uint32_t data = 0;
                    for (int i = 0; i < rx_msg.data_length_code; i++) {
                        data |= (rx_msg.data[i] << (i * 8));
                    }
                    ESP_LOGI(EXAMPLE_TAG, "Received data value %"PRIu32, data);
                    data_msgs_rec ++;
                }
            }
            xSemaphoreGive(ctrl_task_sem);
        } else if (action == RX_RECEIVE_STOP_RESP) {
            //Listen for stop response from slave
            while (1) {
                twai_message_t rx_msg;
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_SLAVE_STOP_RESP) {
                    xSemaphoreGive(ctrl_task_sem);
                    break;
                }
            }
        } else if (action == RX_TASK_EXIT) {
            break;
        }
    }
    vTaskDelete(NULL);
}

static void twai_transmit_task(void *arg)
{
    while (1) {
        tx_task_action_t action;
        xQueueReceive(tx_task_queue, &action, portMAX_DELAY);

        if (action == TX_SEND_PINGS) {
            //Repeatedly transmit pings
            ESP_LOGI(EXAMPLE_TAG, "Transmitting ping");
            while (xSemaphoreTake(stop_ping_sem, 0) != pdTRUE) {
                twai_transmit(&ping_message, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(PING_PERIOD_MS));
            }
        } /*else if (action == TX_SEND_START_CMD) {
            //Transmit start command to slave
            ESP_ERROR_CHECK(twai_transmit(&start_message_multi, portMAX_DELAY));
            ESP_LOGI(EXAMPLE_TAG, "Transmitted start command (LED color)");
        }*/ else if (action == TX_SEND_STOP_CMD) {
            //Transmit stop command to slave
            twai_transmit(&stop_message, portMAX_DELAY);
            ESP_LOGI(EXAMPLE_TAG, "Transmitted stop command");
        } /*else if (action == TX_SEND_LED){
            // Pick color here (RGB)
            send_led_color(255, 0, 0);
        }*/ else if (action == TX_TASK_EXIT) {
            break;
        }
        
        
    }
    vTaskDelete(NULL);
}

static void twai_control_task(void *arg)
{
    xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);
    tx_task_action_t tx_action;
    rx_task_action_t rx_action;

    for (int iter = 0; iter < NO_OF_ITERS; iter++) {
        ESP_ERROR_CHECK(twai_start());
        ESP_LOGI(EXAMPLE_TAG, "Driver started");

        //Start transmitting pings, and listen for ping response
        tx_action = TX_SEND_PINGS;
        rx_action = RX_RECEIVE_PING_RESP;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
        xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);

        /*// ------------------------ Single ESP Block -------------------------------- 
        // Send START command with desired color (e.g. solid red)
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);  // from ping phase
        start_message.data[0] = 0;  // R
        start_message.data[1] = 255;    // G
        start_message.data[2] = 0;    // B
        tx_action = TX_SEND_START_CMD;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);

        // Give the slave some time to light the LED before we send STOP
        vTaskDelay(pdMS_TO_TICKS(5000));
        // ----------------------------------------------------------------------------
        */
       // ---------------------------- Multi ESP Block --------------------------------
       // Wait here until RX task signals that both slaves answered pings
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        // Simple sequence: light slaves one-at-a-time
        
        // Step 1: Slave 1 ON (green), Slave 2 OFF
        /*send_start_to_slave(ID_SLAVE_1, 0, 255, 0);   // S1 = green
        send_start_to_slave(ID_SLAVE_2, 0, 0, 0);     // S2 = off
        vTaskDelay(pdMS_TO_TICKS(3000));

        // Step 2: Slave 1 OFF, Slave 2 ON (blue)
        send_start_to_slave(ID_SLAVE_1, 0, 0, 0);     // S1 = off
        send_start_to_slave(ID_SLAVE_2, 0, 0, 255);   // S2 = blue
        vTaskDelay(pdMS_TO_TICKS(3000));*/
        // Decide colors for this iteration
        uint8_t s1_r = 0, s1_g = 0, s1_b = 0;
        uint8_t s2_r = 0, s2_g = 0, s2_b = 0;
        uint8_t s3_r = 0, s3_g = 0, s3_b = 0;
            
        if (iter % 3 == 0) {
            // Slave 1 Red
            s1_r = 255;   s1_g = 0;   s1_b = 0;
            s2_r = 0;     s2_g = 0;   s2_b = 0;
            s3_r = 0;     s3_g = 0;   s3_b = 0;
        } else if (iter % 3 == 1){
            // Slave 2 Green
            s1_r = 0;   s1_g = 0;     s1_b = 0;
            s2_r = 0;   s2_g = 255;   s2_b = 0;
            s3_r = 0;   s3_g = 0;     s3_b = 0;
        } else if (iter % 3 == 2){
            // Slave 3 Blue
            s1_r = 0;   s1_g = 0;     s1_b = 0;
            s2_r = 0;   s2_g = 0;     s2_b = 0;
            s3_r = 0;   s3_g = 0;     s3_b = 255;

        }
        
        // Send exactly ONE START to each slave this iteration
        send_start_to_slave(ID_SLAVE_1, s1_r, s1_g, s1_b);
        send_start_to_slave(ID_SLAVE_2, s2_r, s2_g, s2_b);
        send_start_to_slave(ID_SLAVE_3, s3_r, s3_g, s3_b);
        
        // Let LEDs stay on for a bit
        vTaskDelay(pdMS_TO_TICKS(3000));
        

        tx_action = TX_SEND_STOP_CMD;
        rx_action = RX_RECEIVE_STOP_RESP;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
        xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);

        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);
        ESP_ERROR_CHECK(twai_stop());
        ESP_LOGI(EXAMPLE_TAG, "Driver stopped");
        vTaskDelay(pdMS_TO_TICKS(ITER_DELAY_MS));
    }
    //Stop TX and RX tasks
    tx_action = TX_TASK_EXIT;
    rx_action = RX_TASK_EXIT;
    xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
    xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);

    //Delete Control task
    xSemaphoreGive(done_sem);
    vTaskDelete(NULL);
}

void app_main(void)
{
    //Create tasks, queues, and semaphores
    rx_task_queue = xQueueCreate(1, sizeof(rx_task_action_t));
    tx_task_queue = xQueueCreate(1, sizeof(tx_task_action_t));
    ctrl_task_sem = xSemaphoreCreateBinary();
    stop_ping_sem = xSemaphoreCreateBinary();
    done_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_control_task, "TWAI_ctrl", 4096, NULL, CTRL_TSK_PRIO, NULL, tskNO_AFFINITY);

    //Install TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(EXAMPLE_TAG, "Driver installed");

    xSemaphoreGive(ctrl_task_sem);              //Start control task
    xSemaphoreTake(done_sem, portMAX_DELAY);    //Wait for completion

    //Uninstall TWAI driver
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(EXAMPLE_TAG, "Driver uninstalled");

    //Cleanup
    vQueueDelete(rx_task_queue);
    vQueueDelete(tx_task_queue);
    vSemaphoreDelete(ctrl_task_sem);
    vSemaphoreDelete(stop_ping_sem);
    vSemaphoreDelete(done_sem);
}
