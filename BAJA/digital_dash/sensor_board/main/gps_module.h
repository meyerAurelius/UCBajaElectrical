#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/_intsup.h>
#include <unistd.h>
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "hal/uart_types.h"
#include "portmacro.h"
#include "esp_log.h"
#include "string.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
//#include "protocol_examples_common.h"
#include <errno.h>
#include <time.h>
#include <sys/time.h>

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

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

void wifi_init_sta(void);
void get_tick_timestamp_string(char *buffer, size_t len);
static char *build_sample_json_payload(void);
static esp_err_t post_json_payload(const char *json_payload);
static void http_post_task(void *pvParameters);
void gps_start(void);
void raw_nmea(void *arg);
void gpstask(void *arg);