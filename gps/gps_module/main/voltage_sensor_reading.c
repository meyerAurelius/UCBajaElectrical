/* ============================================================
 * voltage_sensor_reading.c
 * ============================================================ */
#include "voltage_sensor_reading.h"
#include "sensor_reading_helpers.h"

#include <math.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "voltage_sensor_reading";

/* ------------------------------------------------------------
 * public api
 * ------------------------------------------------------------ */
voltage_sensor_reading_t voltage_sensor_reading(const char *sensor_id)
{
	voltage_sensor_reading_t reading;
	sensor_reading_time_data_t time_data;

	memset(&reading, 0, sizeof(reading));

	time_data = sensor_reading_helpers_create_time_data();

	reading.base.type = "voltage";
	reading.base.sensor_id = sensor_id;
	reading.base.timestamp = time_data.timestamp;
	reading.base.tickstamp_ms = time_data.tickstamp_ms;
	reading.base.to_json = voltage_sensor_reading_to_json;

	reading.voltage = NAN;

	return reading;
}

cJSON *voltage_sensor_reading_to_json(const telemetry_payload_item_t *item)
{
	const voltage_sensor_reading_t *reading;
	cJSON *root;
	cJSON *data;

	reading = (const voltage_sensor_reading_t *)item;

	if (reading == NULL)
	{
		ESP_LOGE(TAG, "reading is NULL");
		return NULL;
	}

	root = cJSON_CreateObject();
	if (root == NULL)
	{
		ESP_LOGE(TAG, "failed to create voltage json object");
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
		ESP_LOGE(TAG, "failed to create voltage data array");
		return NULL;
	}

	sensor_reading_helpers_add_float_or_null(data, reading->voltage);

	cJSON_AddItemToObject(root, "data", data);

	return root;
}