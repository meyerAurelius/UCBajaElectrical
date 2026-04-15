/* ============================================================
 * rpm_sensor_reading.h
 * ============================================================ */
#ifndef RPM_SENSOR_READING_H
#define RPM_SENSOR_READING_H

#include <stdint.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	telemetry_payload_item_t base;

	float rpm;
} rpm_sensor_reading_t;

rpm_sensor_reading_t rpm_sensor_reading(const char *sensor_id);
cJSON *rpm_sensor_reading_to_json(const telemetry_payload_item_t *item);

#ifdef __cplusplus
}
#endif

#endif