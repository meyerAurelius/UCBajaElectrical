/* Capture raw Stamp-S3 ADC samples before printing them over serial. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_adc/adc_continuous.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#define ADC_GPIO             1
#define ADC_ATTENUATION      ADC_ATTEN_DB_0
#define SAMPLE_RATE_HZ       10000
#define CAPTURE_SAMPLES      2000  // 200 ms, matching scope_7.csv
#define READ_BYTES           512
#define STORE_BYTES          4096

static const char *TAG = "raw_adc";
static uint16_t capture[CAPTURE_SAMPLES];
static volatile uint32_t pool_overflows;

static bool on_pool_overflow(adc_continuous_handle_t handle,
                             const adc_continuous_evt_data_t *event,
                             void *user_data)
{
    (void)handle;
    (void)event;
    (void)user_data;
    pool_overflows++;
    return false;
}

void app_main(void)
{
    adc_unit_t unit;
    adc_channel_t channel;
    ESP_ERROR_CHECK(adc_continuous_io_to_channel(ADC_GPIO, &unit, &channel));
    if (unit != ADC_UNIT_1) {
        ESP_LOGE(TAG, "GPIO%d is not on ADC1", ADC_GPIO);
        return;
    }

    adc_continuous_handle_t adc;
    const adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = STORE_BYTES,
        .conv_frame_size = READ_BYTES,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &adc));

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTENUATION,
        .channel = channel,
        .unit = unit,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    };
    const adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .adc_pattern = &pattern,
        .sample_freq_hz = SAMPLE_RATE_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc, &adc_config));
    const adc_continuous_evt_cbs_t callbacks = {
        .on_pool_ovf = on_pool_overflow,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc, &callbacks, NULL));

    ESP_LOGI(TAG, "GPIO%d, ADC1, 0 dB, %d samples/s, %d samples/capture",
             ADC_GPIO, SAMPLE_RATE_HZ, CAPTURE_SAMPLES);
    printf("capture,time_us,raw\n");
    uint32_t capture_id = 0;
    uint8_t bytes[READ_BYTES];

    while (true) {
        const uint32_t overflow_before = pool_overflows;
        uint32_t count = 0;
        bool valid = true;
        ESP_ERROR_CHECK(adc_continuous_start(adc));

        while (count < CAPTURE_SAMPLES) {
            uint32_t length = 0;
            esp_err_t err = adc_continuous_read(adc, bytes, sizeof(bytes),
                                                &length, 500);
            if (err != ESP_OK || pool_overflows != overflow_before) {
                ESP_LOGW(TAG, "capture dropped: %s, overflows=%" PRIu32,
                         esp_err_to_name(err), pool_overflows - overflow_before);
                valid = false;
                break;
            }
            for (uint32_t offset = 0;
                 offset + SOC_ADC_DIGI_RESULT_BYTES <= length && count < CAPTURE_SAMPLES;
                 offset += SOC_ADC_DIGI_RESULT_BYTES) {
                adc_digi_output_data_t result;
                memcpy(&result, bytes + offset, sizeof(result));
                if (result.type2.unit == unit && result.type2.channel == channel) {
                    capture[count++] = result.type2.data;
                }
            }
        }

        ESP_ERROR_CHECK(adc_continuous_stop(adc));
        ESP_ERROR_CHECK(adc_continuous_flush_pool(adc));
        if (valid && pool_overflows == overflow_before) {
            for (uint32_t i = 0; i < CAPTURE_SAMPLES; i++) {
                printf("%" PRIu32 ",%" PRIu32 ",%u\n", capture_id,
                       i * (1000000 / SAMPLE_RATE_HZ), capture[i]);
            }
            fflush(stdout);
            capture_id++;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
