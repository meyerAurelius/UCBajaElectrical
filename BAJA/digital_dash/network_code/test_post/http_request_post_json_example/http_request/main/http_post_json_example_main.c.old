#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "esp_sntp.h"
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

/* ---------------- User-configurable macros ---------------- */
#define POST_SERVER_HOST   "baja.403587.xyz"
#define POST_SERVER_PORT   "80"
#define POST_SERVER_PATH   "/ingest"
#define LOGGING_EVENT_NAME "test_log_one"
#define POST_INTERVAL_MS   10000

#define JSON_BUF_SIZE      2048
#define RECV_BUF_SIZE      256

static const char *TAG = "http_post_json";

typedef struct {
    float x;
    float y;
    float z;
} imu_data_t;

typedef struct {
    int oil_kpa;
    int fuel_kpa;
    int raw_adc;
} pressure_data_t;

typedef struct {
    imu_data_t imu;
    float engine_temp_c;
    pressure_data_t pressure;
    int rpm;
    float battery_voltage;
} sensor_snapshot_t;



static void populate_demo_sensor_data(sensor_snapshot_t *s)
{
    /* Replace these assignments with real sensor reads. */
    s->imu.x = 12.0f;
    s->imu.y = -7.8f;
    s->imu.z = 18.2f;

    s->engine_temp_c = 37.0f;

    s->pressure.oil_kpa = 250;
    s->pressure.fuel_kpa = 80;
    s->pressure.raw_adc = 512;

    s->rpm = 6750;
    s->battery_voltage = 19.1f;
}

static uint32_t get_tick_timestamp_ms_safe(void)
{
    return (uint32_t)( (uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS );
}

static void get_tick_timestamp_string(char *buffer, size_t len)
{
    uint32_t ms = get_tick_timestamp_ms_safe();
    uint32_t sec = ms / 1000;
    uint32_t rem_ms = ms % 1000;

    snprintf(buffer, len, "%lu.%03lu", sec, rem_ms);
}


static int build_json_payload(char *json_buf, size_t json_buf_len, const sensor_snapshot_t *s)
{
    char timestamp[40]; 
    get_tick_timestamp_string(timestamp, sizeof(timestamp));

    return snprintf(json_buf, json_buf_len,
        "{"
            "\"Logging_Event\":\"%s\","
            "\"Logging_Data\":["
                "{"
                    "\"timestamp\":\"%s\","
                    "\"type\":\"imu\","
                    "\"device_id\":\"imu_accel_1\","
                    "\"data\":[%.2f,%.2f,%.2f]"
                "},"
                "{"
                    "\"timestamp\":\"%s\","
                    "\"type\":\"temp\","
                    "\"device_id\":\"eng_temp_1\","
                    "\"data\":[%.2f]"
                "},"
                "{"
                    "\"timestamp\":\"%s\","
                    "\"type\":\"pressure\","
                    "\"device_id\":\"oil_pressure_1\","
                    "\"data\":[%d,%d,%d]"
                "},"
                "{"
                    "\"timestamp\":\"%s\","
                    "\"type\":\"rpm\","
                    "\"device_id\":\"eng_rpm_1\","
                    "\"data\":[%d]"
                "},"
                "{"
                    "\"timestamp\":\"%s\","
                    "\"type\":\"voltage\","
                    "\"device_id\":\"batt_volt_1\","
                    "\"data\":[%.2f]"
                "}"
            "]"
        "}",
        LOGGING_EVENT_NAME,
        timestamp, s->imu.x, s->imu.y, s->imu.z,
        timestamp, s->engine_temp_c,
        timestamp, s->pressure.oil_kpa, s->pressure.fuel_kpa, s->pressure.raw_adc,
        timestamp, s->rpm,
        timestamp, s->battery_voltage);
}

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

static void http_post_task(void *pvParameters)
{
    char json_payload[JSON_BUF_SIZE];
    sensor_snapshot_t sensors;

    while (1) {
        populate_demo_sensor_data(&sensors);

        int json_len = build_json_payload(json_payload, sizeof(json_payload), &sensors);
        if (json_len < 0 || json_len >= (int)sizeof(json_payload)) {
            ESP_LOGE(TAG, "JSON payload buffer too small");
            vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
            continue;
        }

        ESP_LOGI(TAG, "JSON payload:\n%s", json_payload);

        if (post_json_payload(json_payload) != ESP_OK) {
            ESP_LOGE(TAG, "POST request failed");
        }

        vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Configure Wi-Fi or Ethernet in menuconfig. */
    ESP_ERROR_CHECK(example_connect());


    xTaskCreate(&http_post_task, "http_post_task", 8192, NULL, 5, NULL);
}
