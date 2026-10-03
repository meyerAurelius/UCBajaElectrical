#include "spark_detector.h"

#include <string.h>

#define MIN_THRESHOLD_COUNTS 8      // Tune for the ADC counts at your input
#define NOISE_MULTIPLIER     5
#define WARMUP_SAMPLES       200     // 20 ms at 10 ksample/s
#define MIN_SPARK_SAMPLES    150     // 15 ms; rejects ringing around a spark
#define STOP_SAMPLES         5000    // Report zero after 500 ms without a spark

void spark_detector_reset(spark_detector_t *detector)
{
    memset(detector, 0, sizeof(*detector));
    detector->armed = true;
    detector->noise_q8 = 8 << 8;
}

int32_t spark_detector_baseline(const spark_detector_t *detector)
{
    return detector->baseline_q8 >> 8;
}

uint16_t spark_detector_threshold(const spark_detector_t *detector)
{
    int32_t threshold = (detector->noise_q8 >> 8) * NOISE_MULTIPLIER;
    if (threshold < MIN_THRESHOLD_COUNTS) {
        threshold = MIN_THRESHOLD_COUNTS;
    }
    if (threshold > 4095) {
        threshold = 4095;
    }
    return (uint16_t)threshold;
}

bool spark_detector_process(spark_detector_t *detector, uint16_t raw,
                            uint64_t sample_number)
{
    if (!detector->initialized) {
        detector->baseline_q8 = (int32_t)raw << 8;
        detector->initialized = true;
    }

    const int32_t deviation = (int32_t)raw - spark_detector_baseline(detector);
    const uint32_t magnitude = deviation < 0 ? -deviation : deviation;
    const uint16_t threshold = spark_detector_threshold(detector);

    // Update the baseline and the normal noise level only on ordinary samples.
    // An ignition transient must not pull its own reference toward itself.
    if (magnitude < threshold) {
        detector->baseline_q8 += (((int32_t)raw << 8) - detector->baseline_q8) >> 8;
        detector->noise_q8 += (((int32_t)magnitude << 8) - detector->noise_q8) >> 7;
    }
    if (detector->samples_seen < WARMUP_SAMPLES) {
        detector->samples_seen++;
        return false;
    }

    // Hysteresis rearms after the waveform returns close to its baseline.
    if (magnitude < threshold / 2) {
        detector->armed = true;
    }
    if (!detector->armed || magnitude < threshold) {
        return false;
    }
    detector->armed = false;
    if (detector->spark_count != 0 &&
        sample_number - detector->last_spark_sample < MIN_SPARK_SAMPLES) {
        return false;
    }

    if (detector->spark_count != 0) {
        const uint64_t elapsed = sample_number - detector->last_spark_sample;
        if (elapsed <= STOP_SAMPLES) {
            detector->period_samples[detector->period_next] = (uint32_t)elapsed;
            detector->period_next = (detector->period_next + 1) % 4;
            if (detector->periods_seen < 4) {
                detector->periods_seen++;
            }
        } else {
            detector->periods_seen = 0;
            detector->period_next = 0;
        }
    }
    detector->last_spark_sample = sample_number;
    detector->spark_count++;
    return true;
}

uint32_t spark_detector_rpm(const spark_detector_t *detector,
                            uint64_t sample_number, uint32_t sample_rate,
                            uint32_t sparks_per_rev)
{
    if (!detector->periods_seen || !sparks_per_rev ||
        sample_number - detector->last_spark_sample > STOP_SAMPLES) {
        return 0;
    }
    uint32_t period_sum = 0;
    for (uint8_t i = 0; i < detector->periods_seen; i++) {
        period_sum += detector->period_samples[i];
    }
    return (uint32_t)(((uint64_t)60 * sample_rate * detector->periods_seen +
                       period_sum * sparks_per_rev / 2) /
                      ((uint64_t)period_sum * sparks_per_rev));
}
