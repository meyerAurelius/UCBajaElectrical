#include <stdio.h>
#include <stdlib.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Slave TWAI Definitions and static variables ------------------ */
//Example Configuration
#define DATA_PERIOD_MS                  55
#define NO_OF_ITERS                     3
#define ITER_DELAY_MS                   1000
#define RX_TASK_PRIO                    8       //Receiving task priority
#define TX_TASK_PRIO                    9       //Sending task priority
#define CTRL_TSK_PRIO                   10      //Control task priority
#define TX_GPIO_NUM                     17
#define RX_GPIO_NUM                     16
#define EXAMPLE_TAG                     "TWAI Slave"

#define ID_MASTER_STOP_CMD              0x0A0
#define ID_MASTER_START_CMD             0x0A1
#define ID_MASTER_PING                  0x0A2
#define ID_SLAVE_STOP_RESP              0x0C0
#define ID_SLAVE_DATA                   0x0C1
#define ID_SLAVE_PING_RESP              0x0C2


/* --------------------- MPU6050 Definitions and static variables ------------------ */
#define I2C_MASTER_SCL_IO           22     // SCL pin
#define I2C_MASTER_SDA_IO           21     // SDA pin
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define MPU6050_ADDR                0x68   // Change to 0x69 if AD0 is tied high
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B
#define MPU6050_WHO_AM_I            0x75
#define MPU6050_ACCEL_CONFIG        0x1C
#define ACCEL_RANGE_8G              0x10   // AFS_SEL=2
#define ACCEL_SCALE_FACTOR          4096.0f




typedef enum {
    TX_SEND_PING_RESP,
    TX_SEND_DATA,
    TX_SEND_STOP_RESP,
    TX_TASK_EXIT,
} tx_task_action_t;

typedef enum {
    RX_RECEIVE_PING,
    RX_RECEIVE_START_CMD,
    RX_RECEIVE_STOP_CMD,
    RX_TASK_EXIT,
} rx_task_action_t;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

static const twai_message_t ping_resp = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 0,                // Not single shot
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_SLAVE_PING_RESP,
    .data_length_code = 0,
    .data = {0},
};

static const twai_message_t stop_resp = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 0,                // Not single shot
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_SLAVE_STOP_RESP,
    .data_length_code = 0,
    .data = {0},
};

// Data bytes of data message will be initialized in the transmit task
static twai_message_t data_message = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 0,                // Not single shot
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_SLAVE_DATA,
    .data_length_code = 4,
    .data = {1, 2, 3, 4},
};

static QueueHandle_t tx_task_queue;
static QueueHandle_t rx_task_queue;
static SemaphoreHandle_t ctrl_task_sem;
static SemaphoreHandle_t stop_data_sem;
static SemaphoreHandle_t done_sem;

/* --------------------- MPU6050 Initialization Stuff ------------------ */

// I2C initialization
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// Write single byte
static esp_err_t mpu6050_write_byte(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = { reg, data };
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, buffer, sizeof(buffer), 1000 / portTICK_PERIOD_MS);
}

// Read bytes
static esp_err_t mpu6050_read_bytes(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

/* --------------------------- Tasks and Functions -------------------------- */

static void twai_receive_task(void *arg)
{
    printf("Slave CAN Bus program started!");
    while (1) {
        rx_task_action_t action;
        xQueueReceive(rx_task_queue, &action, portMAX_DELAY);
        if (action == RX_RECEIVE_PING) {
            //Listen for pings from master
            twai_message_t rx_msg;
            while (1) {
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_MASTER_PING) {
                    xSemaphoreGive(ctrl_task_sem);
                    break;
                }
            }
        } else if (action == RX_RECEIVE_START_CMD) {
            //Listen for start command from master
            twai_message_t rx_msg;
            while (1) {
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_MASTER_START_CMD) {
                    xSemaphoreGive(ctrl_task_sem);
                    break;
                }
            }
        } else if (action == RX_RECEIVE_STOP_CMD) {
            //Listen for stop command from master
            twai_message_t rx_msg;
            while (1) {
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_MASTER_STOP_CMD) {
                    xSemaphoreGive(stop_data_sem);
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

        if (action == TX_SEND_PING_RESP) {
            //Transmit ping response to master
            twai_transmit(&ping_resp, portMAX_DELAY);
            ESP_LOGI(EXAMPLE_TAG, "Transmitted ping response");
            xSemaphoreGive(ctrl_task_sem);
        } else if (action == TX_SEND_DATA) {
            //Transmit data messages until stop command is received
            ESP_LOGI(EXAMPLE_TAG, "Start transmitting data");
            while (1) {

                //FreeRTOS tick count used to simulate sensor data
                /*uint32_t sensor_data = xTaskGetTickCount();
                for (int i = 0; i < 4; i++) {
                    data_message.data[i] = (sensor_data >> (i * 8)) & 0xFF;
                }*/
                
                //Read Data from MPU6050
                uint8_t data[6];
                int16_t accel_x, accel_y, accel_z;
                if (mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, data, 6) == ESP_OK) {
                    accel_x = (data[0] << 8) | data[1];
                    accel_y = (data[2] << 8) | data[3];
                    accel_z = (data[4] << 8) | data[5];
                    
                    

                    // Example: Pack accel_x into the CAN message (lower 2 bytes)
                    data_message.data[0] = accel_x & 0xFF;
                    data_message.data[1] = (accel_x >> 8) & 0xFF;

                    // Pack accel_y
                    data_message.data[2] = accel_y & 0xFF;
                    data_message.data[3] = (accel_y >> 8) & 0xFF;

                    // If you want Z as well, increase DLC and pack 6 bytes
                    data_message.data_length_code = 6;
                    data_message.data[4] = accel_z & 0xFF;
                    data_message.data[5] = (accel_z >> 8) & 0xFF;

                /*twai_transmit(&data_message, portMAX_DELAY);
                ESP_LOGI(EXAMPLE_TAG, "Transmitted data value %"PRIu32, sensor_data);
                vTaskDelay(pdMS_TO_TICKS(DATA_PERIOD_MS));
                if (xSemaphoreTake(stop_data_sem, 0) == pdTRUE) {
                    break;
                }*/

                    //Transmit MPU6050 data
                    twai_transmit(&data_message, portMAX_DELAY);
                    ESP_LOGI("MPU6050", "Sent accel X=%d Y=%d Z=%d", accel_x, accel_y, accel_z);
                    vTaskDelay(pdMS_TO_TICKS(DATA_PERIOD_MS));
                    if (xSemaphoreTake(stop_data_sem, 0) == pdTRUE) {
                        break;
                    }
                } else {
                    ESP_LOGE("MPU6050", "Failed to read sensor data");
                }

            }
        } else if (action == TX_SEND_STOP_RESP) {
            //Transmit stop response to master
            twai_transmit(&stop_resp, portMAX_DELAY);
            ESP_LOGI(EXAMPLE_TAG, "Transmitted stop response");
            xSemaphoreGive(ctrl_task_sem);
        } else if (action == TX_TASK_EXIT) {
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

    while(1) {
        ESP_ERROR_CHECK(twai_start());
        ESP_LOGI(EXAMPLE_TAG, "Driver started");

        //Listen of pings from master
        rx_action = RX_RECEIVE_PING;
        xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        //Send ping response
        tx_action = TX_SEND_PING_RESP;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        //Listen for start command
        rx_action = RX_RECEIVE_START_CMD;
        xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        //Start sending data messages and listen for stop command
        tx_action = TX_SEND_DATA;
        rx_action = RX_RECEIVE_STOP_CMD;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
        xQueueSend(rx_task_queue, &rx_action, portMAX_DELAY);
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        //Send stop response
        tx_action = TX_SEND_STOP_RESP;
        xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
        xSemaphoreTake(ctrl_task_sem, portMAX_DELAY);

        //Wait for bus to become free
        twai_status_info_t status_info;
        twai_get_status_info(&status_info);
        while (status_info.msgs_to_tx > 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            twai_get_status_info(&status_info);
        }

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
    // Initialize I2C and MPU6050
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("MPU6050", "I2C initialized (SDA=13, SCL=15)");

    // Wake up MPU6050 (set clock source, clear sleep bit)
    ESP_ERROR_CHECK(mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

     // Verify device ID
    uint8_t whoami = 0;
    if (mpu6050_read_bytes(MPU6050_WHO_AM_I, &whoami, 1) == ESP_OK) {
        ESP_LOGI("MPU6050", "WHO_AM_I: 0x%02X", whoami);
        if (whoami != 0x68) {
            ESP_LOGE("MPU6050", "Unexpected WHO_AM_I (should be 0x68).");
        }
    } else {
        ESP_LOGE("MPU6050", "Failed to read WHO_AM_I");
    }


    //Add short delay to allow master it to initialize first
    for (int i = 3; i > 0; i--) {
        printf("Slave starting in %d\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

   
    //Create semaphores and tasks
    tx_task_queue = xQueueCreate(1, sizeof(tx_task_action_t));
    rx_task_queue = xQueueCreate(1, sizeof(rx_task_action_t));
    ctrl_task_sem = xSemaphoreCreateBinary();
    stop_data_sem  = xSemaphoreCreateBinary();
    done_sem  = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_control_task, "TWAI_ctrl", 4096, NULL, CTRL_TSK_PRIO, NULL, tskNO_AFFINITY);

    //Install TWAI driver, trigger tasks to start
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(EXAMPLE_TAG, "Driver installed");

    xSemaphoreGive(ctrl_task_sem);              //Start Control task
    xSemaphoreTake(done_sem, portMAX_DELAY);    //Wait for tasks to complete

    //Uninstall TWAI driver
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(EXAMPLE_TAG, "Driver uninstalled");

    //Cleanup
    vSemaphoreDelete(ctrl_task_sem);
    vSemaphoreDelete(stop_data_sem);
    vSemaphoreDelete(done_sem);
    vQueueDelete(tx_task_queue);
    vQueueDelete(rx_task_queue);
}
