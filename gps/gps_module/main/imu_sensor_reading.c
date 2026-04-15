/* ============================================================
 * imu_sensor_reading.c
 * ============================================================ */
#include "imu_sensor_reading.h"
#include "sensor_reading_helpers.h"

#include <math.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "imu_sensor_reading";

/* ------------------------------------------------------------
 * protected helpers
 * ------------------------------------------------------------ */
static void _imu_sensor_reading_set_defaults(imu_sensor_reading_t *reading)
{
	reading->x = NAN;
	reading->y = NAN;
	reading->z = NAN;

	reading->gyro_x = NAN;
	reading->gyro_y = NAN;
	reading->gyro_z = NAN;
}

/* ------------------------------------------------------------
 * public api
 * ------------------------------------------------------------ */
imu_sensor_reading_t imu_sensor_reading(const char *sensor_id)
{
	imu_sensor_reading_t reading;
	sensor_reading_time_data_t time_data;

	memset(&reading, 0, sizeof(reading));

	time_data = sensor_reading_helpers_create_time_data();

	reading.base.type = "imu";
	reading.base.sensor_id = sensor_id;
	reading.base.timestamp = time_data.timestamp;
	reading.base.tickstamp_ms = time_data.tickstamp_ms;
	reading.base.to_json = imu_sensor_reading_to_json;

	_imu_sensor_reading_set_defaults(&reading);

	return reading;
}

cJSON *imu_sensor_reading_to_json(const telemetry_payload_item_t *item)
{
	const imu_sensor_reading_t *reading;
	cJSON *root;
	cJSON *data;

	reading = (const imu_sensor_reading_t *)item;

	if (reading == NULL)
	{
		ESP_LOGE(TAG, "reading is NULL");
		return NULL;
	}

	root = cJSON_CreateObject();
	if (root == NULL)
	{
		ESP_LOGE(TAG, "failed to create imu json object");
		return NULL;
	}

	cJSON_AddStringToObject(root, "timestamp", reading->base.timestamp);
	cJSON_AddNumberToObject(root, "tickstamp", (double)reading->base.tickstamp_ms);
	cJSON_AddStringToObject(root, "type", reading->base.type);
	cJSON_AddStringToObject(root, "device_id", reading->base.sensor_id);

	data = cJSON_CreateArray();
	if (data == NULL)
	{
		cJSON_Delete(root);
		ESP_LOGE(TAG, "failed to create imu data array");
		return NULL;
	}

	sensor_reading_helpers_add_float_or_null(data, reading->x);
	sensor_reading_helpers_add_float_or_null(data, reading->y);
	sensor_reading_helpers_add_float_or_null(data, reading->z);
	sensor_reading_helpers_add_float_or_null(data, reading->gyro_x);
	sensor_reading_helpers_add_float_or_null(data, reading->gyro_y);
	sensor_reading_helpers_add_float_or_null(data, reading->gyro_z);

	cJSON_AddItemToObject(root, "data", data);

	return root;
}