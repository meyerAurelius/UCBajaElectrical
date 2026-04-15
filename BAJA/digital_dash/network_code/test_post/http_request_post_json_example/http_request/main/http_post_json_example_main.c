#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "cJSON.h"

#include "telemetry_payload_item.h"
#include "json_payload.h"
#include "sensor_reading_helpers.h"
#include "imu_sensor_reading.h"
#include "gps_sensor_reading.h"
#include "temp_sensor_reading.h"
#include "pressure_sensor_reading.h"
#include "rpm_sensor_reading.h"
#include "voltage_sensor_reading.h"

/* ---------------- User-configurable macros ---------------- */
#define POST_SERVER_HOST   "baja.403587.xyz"
#define POST_SERVER_PORT   "80"
#define POST_SERVER_PATH   "/ingest"
#define SESSION_ID         "test_log_one"
#define POST_INTERVAL_MS   10000

#define RECV_BUF_SIZE      256
#define JSON_BUF_SIZE      4096

static const char *TAG = "http_post_json";


/* 
 * TIMESTAMP FUNCTION REQUIRED BY JSON_Payload SYSTEM
 */
void get_tick_timestamp_string(char *buffer, size_t len)
{
	uint32_t ms;
	uint32_t sec;
	uint32_t rem_ms;

	ms = (uint32_t)((uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
	sec = ms / 1000;
	rem_ms = ms % 1000;

	snprintf(buffer, len, "%lu.%03lu", (unsigned long)sec, (unsigned long)rem_ms);
}


/* 
 * Sample JSON Generator using payload system
 */
static char *build_sample_json_payload(void)
{
	json_payload_t payload;

	imu_sensor_reading_t imu_entry;
	gps_sensor_reading_t gps_entry;
	temp_sensor_reading_t temp_entry;
	pressure_sensor_reading_t pressure_entry;
	rpm_sensor_reading_t rpm_entry;
	voltage_sensor_reading_t voltage_entry;

	char *json_string = NULL;

	if (!json_payload_init(&payload, SESSION_ID))
	{
		ESP_LOGE(TAG, "Failed to initialize json payload");
		return NULL;
	}

	/* 
	 * imu entry #1
	 */
	imu_entry = imu_sensor_reading("imu_accel_1");
	imu_entry.x = 10.0f;
	imu_entry.y = 12.0f;
	imu_entry.z = 30.0f;
	imu_entry.gyro_y = 1.4444f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&imu_entry))
	{
		ESP_LOGE(TAG, "Failed to add imu #1");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * imu entry #2
	 * proves variable reuse is safe with new payload design
	 */
	imu_entry = imu_sensor_reading("imu_accel_1");
	imu_entry.x = 5.2f;
	imu_entry.y = 6.8f;
	imu_entry.z = 7.3f;
	imu_entry.gyro_x = 0.14f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&imu_entry))
	{
		ESP_LOGE(TAG, "Failed to add imu entry #2");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * gps
	 */
	gps_entry = gps_sensor_reading("gps_1");
	gps_entry.lat = 51.0447f;
	gps_entry.lon = -114.0719f;
	gps_entry.alt = 1048.5f;
	gps_entry.speed = 12.3f;
	gps_entry.satellites = 9.0f;
	gps_entry.accuracy = 1.8f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&gps_entry))
	{
		ESP_LOGE(TAG, "Failed to add gps");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * temp
	 */
	temp_entry = temp_sensor_reading("eng_temp_1");
	temp_entry.temperature = 96.0f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&temp_entry))
	{
		ESP_LOGE(TAG, "Failed to add temp");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * pressure
	 */
	pressure_entry = pressure_sensor_reading("oil_pressure_1");
	pressure_entry.pressure = 410.0f;
	pressure_entry.temperature = 82.5f;
	pressure_entry.raw_value = 2876.0f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&pressure_entry))
	{
		ESP_LOGE(TAG, "Failed to add pressure");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * rpm
	 */
	rpm_entry = rpm_sensor_reading("eng_rpm_1");
	rpm_entry.rpm = 3125.0f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&rpm_entry))
	{
		ESP_LOGE(TAG, "Failed to add rpm");
		json_payload_free(&payload);
		return NULL;
	}

	/* 
	 * voltage
	 */
	voltage_entry = voltage_sensor_reading("batt_volt_1");
	voltage_entry.voltage = 12.84f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&voltage_entry))
	{
		ESP_LOGE(TAG, "Failed to add voltage");
		json_payload_free(&payload);
		return NULL;
	}

	json_string = json_payload_build_string_unformatted(&payload);
	json_payload_free(&payload);

	return json_string;
}


/* 
 * POST FUNCTION
 */
static esp_err_t post_json_payload(const char *json_payload)
{
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *res = NULL;
	struct in_addr *addr;
	int sock = -1;
	int err;
	char recv_buf[RECV_BUF_SIZE];
	char request_buf[JSON_BUF_SIZE + 512];
	size_t json_len = strlen(json_payload);

	err = getaddrinfo(POST_SERVER_HOST, POST_SERVER_PORT, &hints, &res);
	if (err != 0 || res == NULL) {
		ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
		return ESP_FAIL;
	}

	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	sock = socket(res->ai_family, res->ai_socktype, 0);
	if (sock < 0) {
		ESP_LOGE(TAG, "Failed to allocate socket: errno=%d", errno);
		freeaddrinfo(res);
		return ESP_FAIL;
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(TAG, "Socket connect failed: errno=%d", errno);
		close(sock);
		freeaddrinfo(res);
		return ESP_FAIL;
	}
	freeaddrinfo(res);

	int request_len = snprintf(request_buf, sizeof(request_buf),
		"POST " POST_SERVER_PATH " HTTP/1.1\r\n"
		"Host: " POST_SERVER_HOST ":" POST_SERVER_PORT "\r\n"
		"User-Agent: esp-idf/1.0 esp32\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %u\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		(unsigned int)json_len,
		json_payload);


	if (request_len < 0 || request_len >= (int)sizeof(request_buf)) {
		ESP_LOGE(TAG, "HTTP request buffer too small");
		close(sock);
		return ESP_ERR_NO_MEM;
	}

	if (write(sock, request_buf, request_len) < 0) {
		ESP_LOGE(TAG, "Socket send failed: errno=%d", errno);
		close(sock);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "POST sent successfully");


	struct timeval receiving_timeout = {
		.tv_sec = 5,
		.tv_usec = 0,
	};
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout));

	do {
		memset(recv_buf, 0, sizeof(recv_buf));
		int r = read(sock, recv_buf, sizeof(recv_buf) - 1);
		if (r > 0) {
			printf("%s", recv_buf);
		} else {
			ESP_LOGI(TAG, "Done reading response. Last read=%d errno=%d", r, errno);
			break;
		}
	} while (1);

	close(sock);
	return ESP_OK;
}


/* 
 * UPDATED TASK
 */
static void http_post_task(void *pvParameters)
{
	char *json_payload = NULL;

	(void)pvParameters;

	while (1) {
		json_payload = build_sample_json_payload();

		if (json_payload == NULL) {
			ESP_LOGE(TAG, "Failed to build JSON payload");
			vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
			continue;
		}

		if (strlen(json_payload) >= JSON_BUF_SIZE) {
			ESP_LOGE(TAG, "JSON payload exceeds JSON_BUF_SIZE");
			cJSON_free(json_payload);
			json_payload = NULL;
			vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
			continue;
		}

		ESP_LOGI(TAG, "JSON payload:\n%s", json_payload);

		if (post_json_payload(json_payload) != ESP_OK) {
			ESP_LOGE(TAG, "POST request failed");
		}

		cJSON_free(json_payload);
		json_payload = NULL;

		vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
	}
}


/* 
 * APP MAIN
 */
void app_main(void)
{
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	/* Configure Wi-Fi or Ethernet in menuconfig. */
	ESP_ERROR_CHECK(example_connect());

	xTaskCreate(&http_post_task, "http_post_task", 8192, NULL, 5, NULL);
}