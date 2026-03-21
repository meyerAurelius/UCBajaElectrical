/* ============================================================
 * gps_sensor_reading.h
 * ============================================================ */
#ifndef GPS_SENSOR_READING_H
#define GPS_SENSOR_READING_H

#include <stdint.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	telemetry_payload_item_t base;

	float lat;
	float lon;
	float alt;
	float speed;
	float satellites;
	float accuracy;
} gps_sensor_reading_t;

gps_sensor_reading_t gps_sensor_reading(const char *sensor_id);
cJSON *gps_sensor_reading_to_json(const telemetry_payload_item_t *item);

#ifdef __cplusplus
}
#endif

#endif