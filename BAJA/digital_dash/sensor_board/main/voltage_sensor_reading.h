/* ============================================================
 * voltage_sensor_reading.h
 * ============================================================ */
#ifndef VOLTAGE_SENSOR_READING_H
#define VOLTAGE_SENSOR_READING_H

#include <stdint.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	telemetry_payload_item_t base;

	float voltage;
} voltage_sensor_reading_t;

voltage_sensor_reading_t voltage_sensor_reading(const char *sensor_id);
cJSON *voltage_sensor_reading_to_json(const telemetry_payload_item_t *item);

#ifdef __cplusplus
}
#endif

#endif