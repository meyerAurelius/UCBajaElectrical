/* ============================================================
 * telemetry_payload_item.h
 * ============================================================ */
#ifndef TELEMETRY_PAYLOAD_ITEM_H
#define TELEMETRY_PAYLOAD_ITEM_H

#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct telemetry_payload_item telemetry_payload_item_t;

typedef cJSON *(*telemetry_payload_item_to_json_fn)(const telemetry_payload_item_t *item);

struct telemetry_payload_item
{
	const char *type;
	const char *sensor_id;
	const char *timestamp;
	uint32_t tickstamp_ms;

	telemetry_payload_item_to_json_fn to_json;
};

#ifdef __cplusplus
}
#endif

#endif