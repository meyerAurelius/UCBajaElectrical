/* Stamp-S3 ignition pickup experiment. */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_adc/adc_continuous.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "spark_detector.h"

#define ADC_GPIO             1       // Stamp-S3 G1 = ADC1 channel 0
#define ADC_ATTENUATION      ADC_ATTEN_DB_0 // More ADC codes for a millivolt-scale input
#define SAMPLE_RATE_HZ       10000   // Same 100 us spacing as scope_7.csv
#define READ_BYTES           512
#define STORE_BYTES          4096
#define REPORT_SAMPLES       (SAMPLE_RATE_HZ / 2)
#define SPARKS_PER_REV       2       // This engine fires twice per revolution

static const char *TAG = "tachometer";
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
        ESP_LOGE(TAG, "GPIO%d must be on ADC1", ADC_GPIO);
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
    ESP_ERROR_CHECK(adc_continuous_start(adc));

    spark_detector_t detector;
    spark_detector_reset(&detector);
    uint32_t seen_overflows = 0;
    uint32_t zero_samples = 0;
    uint32_t fullscale_samples = 0;
    uint16_t max_adc = 0;
    uint32_t last_report_sparks = 0;
    uint32_t previous_window_sparks = 0;
    bool have_previous_window = false;
    uint64_t sample_number = 0;
    uint64_t next_report = REPORT_SAMPLES;
    uint8_t bytes[READ_BYTES];

    ESP_LOGI(TAG, "ADC1 GPIO%d at %d samples/s, 0 dB attenuation; %d spark(s)/rev",
             ADC_GPIO, SAMPLE_RATE_HZ, SPARKS_PER_REV);
    while (true) {
        if (pool_overflows != seen_overflows) {
            seen_overflows = pool_overflows;
            ESP_LOGW(TAG, "ADC buffer overflow (%" PRIu32 "); restarting acquisition",
                     seen_overflows);
            ESP_ERROR_CHECK(adc_continuous_stop(adc));
            spark_detector_reset(&detector); // Sample time is no longer continuous.
            sample_number = 0;
            next_report = REPORT_SAMPLES;
            zero_samples = 0;
            fullscale_samples = 0;
            max_adc = 0;
            last_report_sparks = 0;
            previous_window_sparks = 0;
            have_previous_window = false;
            ESP_ERROR_CHECK(adc_continuous_start(adc));
            continue;
        }

        uint32_t length = 0;
        esp_err_t err = adc_continuous_read(adc, bytes, sizeof(bytes), &length, 100);
        if (err == ESP_ERR_TIMEOUT) {
            continue;
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(err));
            ESP_ERROR_CHECK(adc_continuous_stop(adc));
            spark_detector_reset(&detector);
            sample_number = 0;
            next_report = REPORT_SAMPLES;
            last_report_sparks = 0;
            previous_window_sparks = 0;
            have_previous_window = false;
            zero_samples = 0;
            fullscale_samples = 0;
            max_adc = 0;
            ESP_ERROR_CHECK(adc_continuous_start(adc));
            continue;
        }
        if (pool_overflows != seen_overflows) {
            continue; // Discard this frame; its timing may contain a gap.
        }

        for (uint32_t offset = 0; offset + SOC_ADC_DIGI_RESULT_BYTES <= length;
             offset += SOC_ADC_DIGI_RESULT_BYTES) {
            adc_digi_output_data_t result;
            memcpy(&result, bytes + offset, sizeof(result));
            if (result.type2.unit != unit || result.type2.channel != channel) {
                continue;
            }
            const uint16_t raw = result.type2.data;
            if (raw == 0) {
                zero_samples++;
            } else if (raw == 4095) {
                fullscale_samples++;
            }
            if (raw > max_adc) {
                max_adc = raw;
            }
            sample_number++;
            spark_detector_process(&detector, raw, sample_number);
            if (sample_number >= next_report) {
                const uint32_t interval_rpm = spark_detector_rpm(
                    &detector, sample_number, SAMPLE_RATE_HZ, SPARKS_PER_REV);
                const uint32_t window_sparks = detector.spark_count - last_report_sparks;
                last_report_sparks = detector.spark_count;
                uint32_t rpm = 0;
                if (detector.spark_count != 0 &&
                    sample_number - detector.last_spark_sample <= SAMPLE_RATE_HZ / 2) {
                    const uint32_t count = window_sparks +
                        (have_previous_window ? previous_window_sparks : 0);
                    const uint32_t windows = have_previous_window ? 2 : 1;
                    rpm = (uint32_t)(((uint64_t)count * 60 * SAMPLE_RATE_HZ +
                                      windows * REPORT_SAMPLES * SPARKS_PER_REV / 2) /
                                     (windows * REPORT_SAMPLES * SPARKS_PER_REV));
                }
                previous_window_sparks = window_sparks;
                have_previous_window = true;
                const uint32_t last_interval_us = interval_rpm == 0 ? 0 :
                    (uint32_t)(((uint64_t)detector.period_samples[
                        (detector.period_next + 3) % 4] * 1000000) / SAMPLE_RATE_HZ);
                const char *signal = zero_samples >= REPORT_SAMPLES / 2 ? "LOW_RAIL" :
                    (fullscale_samples >= REPORT_SAMPLES / 2 ? "HIGH_RAIL" : "OK");
                ESP_LOGI(TAG, "RPM=%" PRIu32 " interval_rpm=%" PRIu32 " sparks=%" PRIu32
                         " window_sparks=%" PRIu32 " last_interval_us=%" PRIu32
                         " baseline=%ld threshold=%u zero=%" PRIu32
                         " fullscale=%" PRIu32 " max_adc=%u signal=%s",
                         rpm, interval_rpm, detector.spark_count,
                         window_sparks, last_interval_us,
                         (long)spark_detector_baseline(&detector),
                         spark_detector_threshold(&detector), zero_samples,
                         fullscale_samples, max_adc, signal);
                zero_samples = 0;
                fullscale_samples = 0;
                max_adc = 0;
                next_report += REPORT_SAMPLES;
            }
        }
    }
}
