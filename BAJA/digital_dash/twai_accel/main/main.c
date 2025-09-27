#include <stdio.h>
#include <stdlib.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "driver/twai.h"

#define I2C_MASTER_SCL_IO           15     // SCL pin
#define I2C_MASTER_SDA_IO           13     // SDA pin
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define MPU6050_ADDR                0x68   // Change to 0x69 if AD0 is tied high
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B
#define MPU6050_WHO_AM_I            0x75
#define MPU6050_ACCEL_CONFIG        0x1C
#define ACCEL_RANGE_8G              0x10   // AFS_SEL=2
#define ACCEL_SCALE_FACTOR          4096.0f



static const char *TAG = "MPU6050";

// I2C scanner
static void i2c_scanner() {
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t res = i2c_master_write_to_device(I2C_MASTER_NUM, addr, NULL, 0, 10 / portTICK_PERIOD_MS);
        if (res == ESP_OK) {
            ESP_LOGI("SCAN", "Found device at 0x%02X", addr);
        }
    }
}

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

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized (SDA=13, SCL=15)");

    // Scan I2C bus
    i2c_scanner();

    // Wake up MPU6050
    ESP_ERROR_CHECK(mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x10));
    vTaskDelay(pdMS_TO_TICKS(100));


    // Verify WHO_AM_I
    uint8_t whoami = 0;
    if (mpu6050_read_bytes(MPU6050_WHO_AM_I, &whoami, 1) == ESP_OK) {
        ESP_LOGI(TAG, "WHO_AM_I register: 0x%02X", whoami);
        if (whoami != 0x68) {
            ESP_LOGE(TAG, "Unexpected WHO_AM_I! Check wiring or address (should be 0x68).");
        }
    } else {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I");
    }

    uint8_t data[6];
    int16_t accel_x, accel_y, accel_z;

    while (1) {
        if (mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, data, 6) == ESP_OK) {
            accel_x = (data[0] << 8) | data[1];
            accel_y = (data[2] << 8) | data[3];
            accel_z = (data[4] << 8) | data[5];

            ESP_LOGI(TAG, "%f, %f, %f", accel_x / ACCEL_SCALE_FACTOR, accel_y / ACCEL_SCALE_FACTOR, accel_z / ACCEL_SCALE_FACTOR);
        } else {
            ESP_LOGE(TAG, "Failed to read sensor data");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}