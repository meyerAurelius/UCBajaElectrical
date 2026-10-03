#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t baseline_q8;
    int32_t noise_q8;
    uint64_t last_spark_sample;
    uint32_t period_samples[4];
    uint32_t spark_count;
    uint32_t samples_seen;
    uint8_t periods_seen;
    uint8_t period_next;
    bool initialized;
    bool armed;
} spark_detector_t;

void spark_detector_reset(spark_detector_t *detector);
bool spark_detector_process(spark_detector_t *detector, uint16_t raw,
                            uint64_t sample_number);
uint32_t spark_detector_rpm(const spark_detector_t *detector,
                            uint64_t sample_number, uint32_t sample_rate,
                            uint32_t sparks_per_rev);
int32_t spark_detector_baseline(const spark_detector_t *detector);
uint16_t spark_detector_threshold(const spark_detector_t *detector);
