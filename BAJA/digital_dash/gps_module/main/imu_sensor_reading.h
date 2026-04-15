/* ============================================================
 * imu_sensor_reading.h
 * ============================================================ */
#ifndef IMU_SENSOR_READING_H
#define IMU_SENSOR_READING_H

#include <stdint.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	telemetry_payload_item_t base;

	float x;
	float y;
	float z;

	float gyro_x;
	float gyro_y;
	float gyro_z;
} imu_sensor_reading_t;

imu_sensor_reading_t imu_sensor_reading(const char *sensor_id);
cJSON *imu_sensor_reading_to_json(const telemetry_payload_item_t *item);

#ifdef __cplusplus
}
#endif

#endif