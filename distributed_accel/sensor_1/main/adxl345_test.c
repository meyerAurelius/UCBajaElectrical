#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Shared SPI bus. */
#define ADXL345_SCLK_GPIO GPIO_NUM_6
#define ADXL345_MOSI_GPIO GPIO_NUM_5 /* ADXL345 SDA */
#define ADXL345_MISO_GPIO GPIO_NUM_4 /* ADXL345 SDO */

/* Chip select for the single ADXL345 under test. */
#define ADXL345_CS_GPIO GPIO_NUM_1

/* Supplied INT1 pin. This polling test does not require the interrupt line. */
#define ADXL345_INT1_GPIO GPIO_NUM_2

#define ADXL345_SPI_CLOCK_HZ (5 * 1000 * 1000)
#define ADXL345_SAMPLE_RATE_HZ 800
#define ADXL345_SAMPLE_PERIOD_US (1000000 / ADXL345_SAMPLE_RATE_HZ)

#define ADXL345_REG_DEVID 0x00
#define ADXL345_REG_BW_RATE 0x2C
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_INT_ENABLE 0x2E
#define ADXL345_REG_INT_MAP 0x2F
#define ADXL345_REG_DATA_FORMAT 0x31
#define ADXL345_REG_DATAX0 0x32

#define ADXL345_READ 0x80
#define ADXL345_MB 0x40
#define ADXL345_EXPECTED_DEVID 0xE5

static const char *TAG = "adxl345_test";

typedef struct {
    spi_device_handle_t spi;
    gpio_num_t cs_gpio;
    uint8_t devid;
} adxl345_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} adxl345_sample_t;

static esp_err_t adxl345_transfer(adxl345_t *device,
                                  const uint8_t *tx_data,
                                  uint8_t *rx_data,
                                  size_t length)
{
    spi_transaction_t transaction = {
        .length = length * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    return spi_device_polling_transmit(device->spi, &transaction);
}

static esp_err_t adxl345_write_register(adxl345_t *device,
                                        uint8_t address,
                                        uint8_t value)
{
    const uint8_t tx_data[2] = {address & 0x3F, value};
    return adxl345_transfer(device, tx_data, NULL, sizeof(tx_data));
}

static esp_err_t adxl345_read_register(adxl345_t *device,
                                       uint8_t address,
                                       uint8_t *value)
{
    const uint8_t tx_data[2] = {address | ADXL345_READ, 0};
    uint8_t rx_data[2] = {0};
    esp_err_t error = adxl345_transfer(device, tx_data, rx_data, sizeof(tx_data));

    if (error == ESP_OK) {
        *value = rx_data[1];
    }
    return error;
}

static esp_err_t adxl345_read_sample(adxl345_t *device,
                                     adxl345_sample_t *sample)
{
    const uint8_t tx_data[7] = {ADXL345_REG_DATAX0 | ADXL345_READ | ADXL345_MB};
    uint8_t rx_data[7] = {0};
    esp_err_t error = adxl345_transfer(device, tx_data, rx_data, sizeof(tx_data));

    if (error != ESP_OK) {
        return error;
    }

    sample->x = (int16_t)((uint16_t)rx_data[2] << 8 | rx_data[1]);
    sample->y = (int16_t)((uint16_t)rx_data[4] << 8 | rx_data[3]);
    sample->z = (int16_t)((uint16_t)rx_data[6] << 8 | rx_data[5]);
    return ESP_OK;
}

static esp_err_t adxl345_init(adxl345_t *device, spi_host_device_t host)
{
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = ADXL345_SPI_CLOCK_HZ,
        .mode = 3,
        .spics_io_num = device->cs_gpio,
        .queue_size = 1,
    };

    ESP_RETURN_ON_ERROR(spi_bus_add_device(host, &device_config, &device->spi),
                        TAG, "failed to add device on CS GPIO %d", device->cs_gpio);

    ESP_RETURN_ON_ERROR(adxl345_read_register(device, ADXL345_REG_DEVID, &device->devid),
                        TAG, "failed to read DEVID on CS GPIO %d", device->cs_gpio);

    if (device->devid != ADXL345_EXPECTED_DEVID) {
        ESP_LOGE(TAG,
                 "CS GPIO %d: unexpected DEVID 0x%02X (expected 0x%02X)",
                 device->cs_gpio,
                 device->devid,
                 ADXL345_EXPECTED_DEVID);
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* 800 Hz output-data rate, full-resolution +/-16 g, measurement mode. */
    ESP_RETURN_ON_ERROR(adxl345_write_register(device, ADXL345_REG_BW_RATE, 0x0C),
                        TAG, "failed to set 800 Hz rate on CS GPIO %d", device->cs_gpio);
    ESP_RETURN_ON_ERROR(adxl345_write_register(device, ADXL345_REG_DATA_FORMAT, 0x0B),
                        TAG, "failed to set data format on CS GPIO %d", device->cs_gpio);
    ESP_RETURN_ON_ERROR(adxl345_write_register(device, ADXL345_REG_POWER_CTL, 0x08),
                        TAG, "failed to enter measurement mode on CS GPIO %d", device->cs_gpio);

    /* Leave interrupt routing disabled; samples are read by the timed polling loop. */
    ESP_RETURN_ON_ERROR(adxl345_write_register(device, ADXL345_REG_INT_ENABLE, 0x00),
                        TAG, "failed to disable interrupts on CS GPIO %d", device->cs_gpio);
    ESP_RETURN_ON_ERROR(adxl345_write_register(device, ADXL345_REG_INT_MAP, 0x00),
                        TAG, "failed to configure interrupt map on CS GPIO %d", device->cs_gpio);

    ESP_LOGI(TAG, "CS GPIO %d: ADXL345 detected (DEVID=0x%02X)", device->cs_gpio, device->devid);
    return ESP_OK;
}

static void wait_until_us(int64_t target_time_us)
{
    int64_t remaining_us;

    /*
     * Never call vTaskDelay(0) here. With a 1 ms FreeRTOS tick, the old
     * conversion could return zero while more than 1 ms remained, causing
     * the task to spin on CPU 0 and starve IDLE0 long enough to trip the
     * task watchdog.
     */
    while ((remaining_us = target_time_us - esp_timer_get_time()) > 0) {
        if (remaining_us > 1000) {
            vTaskDelay(1);
        } else {
            /* Only busy-wait for the final sub-millisecond fraction. */
            esp_rom_delay_us((uint32_t)remaining_us);
            break;
        }
    }
}

void app_main(void)
{
    spi_bus_config_t bus_config = {
        .sclk_io_num = ADXL345_SCLK_GPIO,
        .mosi_io_num = ADXL345_MOSI_GPIO,
        .miso_io_num = ADXL345_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 7,
    };

    gpio_config_t int_config = {
        .pin_bit_mask = 1ULL << ADXL345_INT1_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&int_config));
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    adxl345_t module = {.cs_gpio = ADXL345_CS_GPIO};
    ESP_ERROR_CHECK(adxl345_init(&module, SPI2_HOST));

    ESP_LOGI(TAG,
             "Sampling the ADXL345 at %d Hz; printing every 50th sample (~16 Hz)",
             ADXL345_SAMPLE_RATE_HZ);
    ESP_LOGI(TAG, "INT1 GPIO %d is configured as an input but is not required for polling", ADXL345_INT1_GPIO);

    uint32_t sample_number = 0;
    int64_t next_sample_time_us = esp_timer_get_time();

    while (true) {
        adxl345_sample_t sample;

        ESP_ERROR_CHECK(adxl345_read_sample(&module, &sample));

        sample_number++;
        if (sample_number % 50 == 0) {
            ESP_LOGI(TAG,
                     "sample=%" PRIu32 " | raw [%6d %6d %6d] | acceleration [%.3f %.3f %.3f g]",
                     sample_number,
                     sample.x,
                     sample.y,
                     sample.z,
                     sample.x * 0.0039f,
                     sample.y * 0.0039f,
                     sample.z * 0.0039f);
        }

        next_sample_time_us += ADXL345_SAMPLE_PERIOD_US;
        wait_until_us(next_sample_time_us);
    }
}
