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

#include <math.h>
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "Droid"
#define EXAMPLE_ESP_WIFI_PASS      "password123"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

#define GPS_UART_TX 16
#define GPS_UART_RX 17

#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

// Bullshit that prevents errors
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define EXAMPLE_H2E_IDENTIFIER "your_identifier"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFITAG = "wifi station";

static int s_retry_num = 0;



static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFITAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFITAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFITAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFITAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFITAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFITAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(WIFITAG, "UNEXPECTED EVENT");
    }

}

//Declare gps variables
static int BUFFER = 1024 * 2;
static char buf[1024 * 2];
char lat[12];
char lon[12];
float latitude;
float longitude;
int latDD = 0;
int lonDDD = 0;
float latM = 0;
float lonM = 0;
char NS;
char EW;
char STATUS = 86;
char timeIN[10];
float speedKnots = 0;
float degreesTrue, kmhr;
char date[6];
int day, month, year, second, minute, hour;
char timestamp[30];
int invert = 1;
int iteration = 0;
int quality, satCount;
float HDOP, altitude, undulation, TrueAltitude;
float accuracy = 0;


/* ---------------- User-configurable macros ---------------- */
#define POST_SERVER_HOST   "telemetry-api.ucbaja.ca"
#define POST_SERVER_PORT   "80"
#define POST_SERVER_PATH   "/ingest"
#define SESSION_ID         "test_log_one"
#define POST_INTERVAL_MS   10000

#define RECV_BUF_SIZE      256
#define JSON_BUF_SIZE      4096

static const char *HTTP_TAG = "http_post_json";


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

	snprintf(buffer, len, timestamp);
}


/*
 * Sample JSON Generator using payload system
 */
double tempC;
double gps_speed;

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
		ESP_LOGE(HTTP_TAG, "Failed to initialize json payload");
		return NULL;
	}



	/*
	 * imu entry #1

	imu_entry = imu_sensor_reading("imu_accel_1");
	imu_entry.x = 10.0f;
	imu_entry.y = 12.0f;
	imu_entry.z = 30.0f;
	imu_entry.gyro_y = 1.4444f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&imu_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add imu #1");
		json_payload_free(&payload);
		return NULL;
	}


	 * imu entry #2
	 * proves variable reuse is safe with new payload design

	imu_entry = imu_sensor_reading("imu_accel_1");
	imu_entry.x = 5.2f;
	imu_entry.y = 6.8f;
	imu_entry.z = 7.3f;
	imu_entry.gyro_x = 0.14f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&imu_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add imu entry #2");
		json_payload_free(&payload);
		return NULL;
	}
	*/
	/*
	 * gps
	 */
	gps_entry = gps_sensor_reading("gps_1");
	gps_entry.lat = latitude;
	gps_entry.lon = longitude;
	gps_entry.alt = altitude;
	gps_entry.speed = kmhr;
	gps_entry.satellites = satCount;
	gps_entry.accuracy = accuracy;

	gps_speed = kmhr;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&gps_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add gps");
		json_payload_free(&payload);
		return NULL;
	}

	/*
	 * temp
	 */
	temp_entry = temp_sensor_reading("eng_temp_1");
	temp_entry.temperature = round(tempC);

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&temp_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add temp");
		json_payload_free(&payload);
		return NULL;
	}

	/*
	 * pressure

	pressure_entry = pressure_sensor_reading("oil_pressure_1");
	pressure_entry.pressure = 410.0f;
	pressure_entry.temperature = 82.5f;
	pressure_entry.raw_value = 2876.0f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&pressure_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add pressure");
		json_payload_free(&payload);
		return NULL;
	}


	  rpm

	rpm_entry = rpm_sensor_reading("eng_rpm_1");
	rpm_entry.rpm = 3125.0f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&rpm_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add rpm");
		json_payload_free(&payload);
		return NULL;
	}


	 * voltage

	voltage_entry = voltage_sensor_reading("batt_volt_1");
	voltage_entry.voltage = 12.84f;

	if (!json_payload_add_data(&payload, (const telemetry_payload_item_t *)&voltage_entry))
	{
		ESP_LOGE(HTTP_TAG, "Failed to add voltage");
		json_payload_free(&payload);
		return NULL;
	}
	*/

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
		ESP_LOGE(HTTP_TAG, "DNS lookup failed err=%d res=%p", err, res);
		return ESP_FAIL;
	}

	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	sock = socket(res->ai_family, res->ai_socktype, 0);
	if (sock < 0) {
		ESP_LOGE(HTTP_TAG, "Failed to allocate socket: errno=%d", errno);
		freeaddrinfo(res);
		return ESP_FAIL;
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(HTTP_TAG, "Socket connect failed: errno=%d", errno);
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
		ESP_LOGE(HTTP_TAG, "HTTP request buffer too small");
		close(sock);
		return ESP_ERR_NO_MEM;
	}

	if (write(sock, request_buf, request_len) < 0) {
		ESP_LOGE(HTTP_TAG, "Socket send failed: errno=%d", errno);
		close(sock);
		return ESP_FAIL;
	}

	ESP_LOGI(HTTP_TAG, "POST sent successfully");


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
			ESP_LOGI(HTTP_TAG, "Done reading response. Last read=%d errno=%d", r, errno);
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
			ESP_LOGE(HTTP_TAG, "Failed to build JSON payload");
			vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
			continue;
		}

		if (strlen(json_payload) >= JSON_BUF_SIZE) {
			ESP_LOGE(HTTP_TAG, "JSON payload exceeds JSON_BUF_SIZE");
			cJSON_free(json_payload);
			json_payload = NULL;
			vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
			continue;
		}

		ESP_LOGI(HTTP_TAG, "JSON payload:\n%s", json_payload);

		if (post_json_payload(json_payload) != ESP_OK) {
			ESP_LOGE(HTTP_TAG, "POST request failed");
		}

		cJSON_free(json_payload);
		json_payload = NULL;

		vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
	}
}



//GPS HANDLING


static const char *GPSTAG = "GPS_MODULE";


void gps_start(void)
{
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };


    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, GPS_UART_TX, GPS_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUFFER, 0, 0, NULL, 0));

	printf("\t\t\tStarted GPS...\n");
}

void raw_nmea(void)
{
    memset(buf, 0, BUFFER);
    uart_read_bytes(UART_NUM_2, buf, BUFFER, portMAX_DELAY);

}


//IMPORTANT : Always invoke gps_start() and then raw_nmea()

void gpstask(void *arg)
{
    gps_start(); //avoid using gps_start() again until and unless driver is not de-inited properly.



    while(1)
    {
        //intake data
        raw_nmea();
        char *GPRMCDATA = strstr(buf, "$GPRMC");
		ESP_LOGI(GPSTAG, "%s", GPRMCDATA);
        sscanf(GPRMCDATA, "$GPRMC,%9s,%c,%2d%9f,%c,%3d%8f,%c,%f,%f,%6s", timeIN, &STATUS, &latDD, &latM, &NS, &lonDDD, &lonM, &EW, &speedKnots, &degreesTrue, date);
		//STATUS != 86
        if(1){

        //Aquire accurate long and lat values
        if(EW == 87){
            invert = -1;
        }else{
            invert = 1;
        }
        longitude = invert * (lonDDD + lonM/60);
        if(NS == 83){
            invert = -1;
        }else{
            invert = 1;
        }
        latitude = invert * (latDD + latM/60);

        //Format timestamp to ISO standard
        sscanf(date, "%2d%2d%2d", &day, &month, &year);
        sscanf(timeIN, "%2d%2d%2d", &hour, &minute, &second);
        snprintf(timestamp, 30, "20%02d-%02d-%02dT%02d:%02d:%02d-00:00", year, month, day, hour, minute, second);

        //Format speed and heading
        kmhr = speedKnots * 1.852;
            //HEADING DOESNT NEED FORMAT

        //Accuracy and altitude

        char *GPGGADATA = strstr(buf, "$GPGGA");
        sscanf(GPGGADATA, "$GPGGA,%*f,%*f,%*c,%*f,%*c,%d,%d,%f,%f,%*c,%f", &quality, &satCount, &HDOP, &altitude, &undulation);

        //Accuracy of latlon data
        switch(quality){
            case 1:
                accuracy = 7.5 * HDOP;
                break;
            case 2:
                accuracy = 2.5 * HDOP;
                break;
                accuracy = 0.5 * HDOP;
                break;
            case 5:
                accuracy = 0.1 * HDOP;
                break;
        }
        //Altitude accuracy correction
        TrueAltitude = altitude - undulation;

        ESP_LOGI(GPSTAG, "%d,%f,%f,%f,%f,%f,%d,%d,%f,%f,%s", iteration, latitude, longitude, altitude, kmhr, degreesTrue, satCount, quality, HDOP, accuracy, timestamp);

        } else {
        //If GPS data is invalid (i.e. no signal)
        ESP_LOGI(GPSTAG, "%d,,,,,,,,,", iteration);

        }
        iteration++;
    }
}

// use uart_driver_delete(UART_NUM_0) when done with UART to avoid collision with other periphrals.


// void app_main(void)
// {

// 	ESP_ERROR_CHECK(esp_netif_init());
//     //Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
//         /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
//          * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
//         esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
//     }

//     ESP_LOGI(HTTP_TAG, "ESP_WIFI_MODE_STA");
//     wifi_init_sta();

//     xTaskCreatePinnedToCore(&gpstask, "GPS", 4092*2, NULL, 5, NULL, 1);
//     xTaskCreatePinnedToCore(&http_post_task, "http_post_task", 4092*2, NULL, 5, NULL, 0);

// }