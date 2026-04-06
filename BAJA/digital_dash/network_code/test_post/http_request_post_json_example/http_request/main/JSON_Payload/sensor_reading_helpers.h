/* ============================================================
 * sensor_reading_helpers.h
 * ============================================================ */
#ifndef SENSOR_READING_HELPERS_H
#define SENSOR_READING_HELPERS_H

#include <stdint.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_READING_TIMESTAMP_BUFFER_COUNT 16
#define SENSOR_READING_TIMESTAMP_BUFFER_SIZE 40

typedef struct
{
	const char *timestamp;
	uint32_t tickstamp_ms;
} sensor_reading_time_data_t;

/* timestamp / tick helpers */
sensor_reading_time_data_t sensor_reading_helpers_create_time_data(void);

/* json helpers */
bool sensor_reading_helpers_add_float_or_null(cJSON *array, float value);
bool sensor_reading_helpers_add_double_or_null(cJSON *array, double value);
bool sensor_reading_helpers_add_int_or_null(cJSON *array, int value, bool has_value);
bool sensor_reading_helpers_add_uint32_or_null(cJSON *array, uint32_t value, bool has_value);

#ifdef __cplusplus
}
#endif

#endif