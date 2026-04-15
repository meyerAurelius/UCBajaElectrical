/* ============================================================
 * json_payload.c
 * ============================================================ */
#include "json_payload.h"

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "json_payload";

/* ------------------------------------------------------------
 * protected helpers
 * ------------------------------------------------------------ */
static bool _json_payload_validate(const json_payload_t *payload)
{
	if (payload == NULL)
	{
		ESP_LOGE(TAG, "payload is NULL");
		return false;
	}

	if (payload->session_id == NULL || payload->session_id[0] == '\0')
	{
		ESP_LOGE(TAG, "session_id is NULL or empty");
		return false;
	}

	if (payload->logging_data == NULL)
	{
		ESP_LOGE(TAG, "logging_data array is NULL");
		return false;
	}

	if (!cJSON_IsArray(payload->logging_data))
	{
		ESP_LOGE(TAG, "logging_data is not an array");
		return false;
	}

	return true;
}

/* ------------------------------------------------------------
 * public api
 * ------------------------------------------------------------ */
bool json_payload_init(json_payload_t *payload, const char *session_id)
{
	if (payload == NULL)
	{
		return false;
	}

	payload->session_id = session_id;
	payload->logging_data = cJSON_CreateArray();

	if (payload->logging_data == NULL)
	{
		ESP_LOGE(TAG, "failed to create logging_data array");
		return false;
	}

	return true;
}

void json_payload_reset(json_payload_t *payload)
{
	if (payload == NULL)
	{
		return;
	}

	if (payload->logging_data != NULL)
	{
		cJSON_Delete(payload->logging_data);
		payload->logging_data = NULL;
	}

	payload->logging_data = cJSON_CreateArray();

	if (payload->logging_data == NULL)
	{
		ESP_LOGE(TAG, "failed to recreate logging_data array during reset");
	}
}

void json_payload_free(json_payload_t *payload)
{
	if (payload == NULL)
	{
		return;
	}

	if (payload->logging_data != NULL)
	{
		cJSON_Delete(payload->logging_data);
		payload->logging_data = NULL;
	}

	payload->session_id = NULL;
}

bool json_payload_add_data(json_payload_t *payload, const telemetry_payload_item_t *item)
{
	cJSON *item_json;

	if (!_json_payload_validate(payload))
	{
		return false;
	}

	if (item == NULL)
	{
		ESP_LOGE(TAG, "item is NULL");
		return false;
	}

	if (item->to_json == NULL)
	{
		ESP_LOGE(TAG, "item->to_json is NULL");
		return false;
	}

	item_json = item->to_json(item);
	if (item_json == NULL)
	{
		ESP_LOGE(TAG, "item->to_json failed");
		return false;
	}

	cJSON_AddItemToArray(payload->logging_data, item_json);

	return true;
}

size_t json_payload_count(const json_payload_t *payload)
{
	if (payload == NULL || payload->logging_data == NULL)
	{
		return 0;
	}

	return (size_t)cJSON_GetArraySize(payload->logging_data);
}

cJSON *json_payload_build_object(const json_payload_t *payload)
{
	cJSON *root;
	cJSON *logging_data_copy;

	if (!_json_payload_validate(payload))
	{
		return NULL;
	}

	root = cJSON_CreateObject();
	if (root == NULL)
	{
		ESP_LOGE(TAG, "failed to create root object");
		return NULL;
	}

	cJSON_AddStringToObject(root, "Logging_Event", payload->session_id);
	cJSON_AddStringToObject(root, "session_id", payload->session_id);
	cJSON_AddNumberToObject(root, "ticks_reference", (double)(esp_timer_get_time() / 1000ULL));

	logging_data_copy = cJSON_Duplicate(payload->logging_data, 1);
	if (logging_data_copy == NULL)
	{
		ESP_LOGE(TAG, "failed to duplicate logging_data");
		cJSON_Delete(root);
		return NULL;
	}

	cJSON_AddItemToObject(root, "Logging_Data", logging_data_copy);

	return root;
}

char *json_payload_build_string(const json_payload_t *payload)
{
	cJSON *root;
	char *json_string;

	root = json_payload_build_object(payload);
	if (root == NULL)
	{
		return NULL;
	}

	json_string = cJSON_Print(root);
	cJSON_Delete(root);

	return json_string;
}

char *json_payload_build_string_unformatted(const json_payload_t *payload)
{
	cJSON *root;
	char *json_string;

	root = json_payload_build_object(payload);
	if (root == NULL)
	{
		return NULL;
	}

	json_string = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return json_string;
}