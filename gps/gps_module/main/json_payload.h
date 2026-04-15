/* ============================================================
 * json_payload.h
 * ============================================================ */
#ifndef JSON_PAYLOAD_H
#define JSON_PAYLOAD_H

#include <stdbool.h>
#include <stddef.h>
#include "cJSON.h"
#include "telemetry_payload_item.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	const char *session_id;
	cJSON *logging_data;
} json_payload_t;

/* lifecycle */
bool json_payload_init(json_payload_t *payload, const char *session_id);
void json_payload_reset(json_payload_t *payload);
void json_payload_free(json_payload_t *payload);

/* data management */
bool json_payload_add_data(json_payload_t *payload, const telemetry_payload_item_t *item);
size_t json_payload_count(const json_payload_t *payload);

/* build helpers */
cJSON *json_payload_build_object(const json_payload_t *payload);
char *json_payload_build_string(const json_payload_t *payload);
char *json_payload_build_string_unformatted(const json_payload_t *payload);

#ifdef __cplusplus
}
#endif

#endif