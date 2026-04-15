/* ============================================================
 * pressure_sensor_reading.h
 * ============================================================ */
#ifndef PRESSURE_SENSOR_READING_H
#define PRESSURE_SENSOR_READING_H

#include <stdint.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	telemetry_payload_item_t base;

	float pressure;
	float temperature;
	float raw_value;
} pressure_sensor_reading_t;

pressure_sensor_reading_t pressure_sensor_reading(const char *sensor_id);
cJSON *pressure_sensor_reading_to_json(const telemetry_payload_item_t *item);

#ifdef __cplusplus
}
#endif

#endif