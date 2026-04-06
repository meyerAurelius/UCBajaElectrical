/* ============================================================
 * sensor_reading_helpers.c
 * ============================================================ */
#include "sensor_reading_helpers.h"
#include <math.h>
#include <stdbool.h>
#include "esp_timer.h"

/*
 * This should already exist somewhere in your project.
 * It should write a timestamp string into the provided buffer.
 */
extern void get_tick_timestamp_string(char *buffer, size_t buffer_len);

/* ------------------------------------------------------------
 * internal shared storage
 * ------------------------------------------------------------ */
static char _sensor_reading_timestamp_buffers[SENSOR_READING_TIMESTAMP_BUFFER_COUNT][SENSOR_READING_TIMESTAMP_BUFFER_SIZE];
static uint8_t _sensor_reading_timestamp_buffer_index = 0;

/* ------------------------------------------------------------
 * public api
 * ------------------------------------------------------------ */
sensor_reading_time_data_t sensor_reading_helpers_create_time_data(void)
{
	sensor_reading_time_data_t out;

	_sensor_reading_timestamp_buffer_index =
		(_sensor_reading_timestamp_buffer_index + 1) % SENSOR_READING_TIMESTAMP_BUFFER_COUNT;

	get_tick_timestamp_string(
		_sensor_reading_timestamp_buffers[_sensor_reading_timestamp_buffer_index],
		sizeof(_sensor_reading_timestamp_buffers[_sensor_reading_timestamp_buffer_index])
	);

	out.timestamp = _sensor_reading_timestamp_buffers[_sensor_reading_timestamp_buffer_index];
	out.tickstamp_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

	return out;
}

bool sensor_reading_helpers_add_float_or_null(cJSON *array, float value)
{
	if (array == NULL)
	{
		return false;
	}

	if (isfinite(value))
	{
		cJSON_AddItemToArray(array, cJSON_CreateNumber((double)value));
	}
	else
	{
		cJSON_AddItemToArray(array, cJSON_CreateNull());
	}

	return true;
}

bool sensor_reading_helpers_add_double_or_null(cJSON *array, double value)
{
	if (array == NULL)
	{
		return false;
	}

	if (isfinite(value))
	{
		cJSON_AddItemToArray(array, cJSON_CreateNumber(value));
	}
	else
	{
		cJSON_AddItemToArray(array, cJSON_CreateNull());
	}

	return true;
}

bool sensor_reading_helpers_add_int_or_null(cJSON *array, int value, bool has_value)
{
	if (array == NULL)
	{
		return false;
	}

	if (has_value)
	{
		cJSON_AddItemToArray(array, cJSON_CreateNumber((double)value));
	}
	else
	{
		cJSON_AddItemToArray(array, cJSON_CreateNull());
	}

	return true;
}

bool sensor_reading_helpers_add_uint32_or_null(cJSON *array, uint32_t value, bool has_value)
{
	if (array == NULL)
	{
		return false;
	}

	if (has_value)
	{
		cJSON_AddItemToArray(array, cJSON_CreateNumber((double)value));
	}
	else
	{
		cJSON_AddItemToArray(array, cJSON_CreateNull());
	}

	return true;
}