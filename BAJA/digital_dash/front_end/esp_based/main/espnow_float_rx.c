#include "espnow_float_rx.h"

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define ESPNOW_FLOAT_RX_MAX_FLOATS 64

static const char *TAG = "espnow_float_rx";

static espnow_float_rx_cb_t s_user_cb = NULL;
static SemaphoreHandle_t s_rx_mutex = NULL;
static bool s_initialized = false;

static float s_last_values[ESPNOW_FLOAT_RX_MAX_FLOATS];
static size_t s_last_count = 0;
static uint8_t s_last_src_mac[ESP_NOW_ETH_ALEN] = {0};

static void espnow_float_rx_recv_cb(const esp_now_recv_info_t *recv_info,
                                    const uint8_t *data,
                                    int len)
{
    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGW(TAG, "Invalid RX callback args");
        return;
    }

    if ((len % (int)sizeof(float)) != 0) {
        ESP_LOGW(TAG, "Received %d bytes, not divisible by float size", len);
        return;
    }

    size_t count = (size_t)len / sizeof(float);
    if (count > ESPNOW_FLOAT_RX_MAX_FLOATS) {
        ESP_LOGW(TAG, "Received %u floats, truncating to %u",
                 (unsigned)count, (unsigned)ESPNOW_FLOAT_RX_MAX_FLOATS);
        count = ESPNOW_FLOAT_RX_MAX_FLOATS;
    }

    if (s_rx_mutex != NULL) {
        if (xSemaphoreTake(s_rx_mutex, 0) == pdTRUE) {
            memcpy(s_last_values, data, count * sizeof(float));
            s_last_count = count;
            memcpy(s_last_src_mac, recv_info->src_addr, ESP_NOW_ETH_ALEN);
            xSemaphoreGive(s_rx_mutex);
        }
    }

    if (s_user_cb != NULL) {
        s_user_cb(recv_info->src_addr, (const float *)data, count);
    }
}

static esp_err_t espnow_float_rx_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t espnow_float_rx_init(uint8_t wifi_channel, espnow_float_rx_cb_t cb)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(espnow_float_rx_init_nvs(), TAG, "nvs init failed");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "esp_wifi_set_storage failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE),
                        TAG, "esp_wifi_set_channel failed");

    ESP_RETURN_ON_ERROR(esp_now_init(), TAG, "esp_now_init failed");
    ESP_RETURN_ON_ERROR(esp_now_register_recv_cb(espnow_float_rx_recv_cb),
                        TAG, "esp_now_register_recv_cb failed");

    s_rx_mutex = xSemaphoreCreateMutex();
    if (s_rx_mutex == NULL) {
        esp_now_deinit();
        return ESP_ERR_NO_MEM;
    }

    s_user_cb = cb;
    s_last_count = 0;
    memset(s_last_values, 0, sizeof(s_last_values));
    memset(s_last_src_mac, 0, sizeof(s_last_src_mac));

    s_initialized = true;
    ESP_LOGI(TAG, "ESP-NOW float RX initialized on channel %u", wifi_channel);
    return ESP_OK;
}

void espnow_float_rx_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    esp_now_unregister_recv_cb();
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_rx_mutex != NULL) {
        vSemaphoreDelete(s_rx_mutex);
        s_rx_mutex = NULL;
    }

    s_user_cb = NULL;
    s_last_count = 0;
    s_initialized = false;
}

esp_err_t espnow_float_rx_get_last(float *out_values,
                                   size_t max_count,
                                   size_t *out_count,
                                   uint8_t out_src_mac[ESP_NOW_ETH_ALEN])
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (out_values == NULL || out_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_rx_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    size_t copy_count = s_last_count;
    if (copy_count > max_count) {
        copy_count = max_count;
    }

    memcpy(out_values, s_last_values, copy_count * sizeof(float));
    *out_count = copy_count;

    if (out_src_mac != NULL) {
        memcpy(out_src_mac, s_last_src_mac, ESP_NOW_ETH_ALEN);
    }

    xSemaphoreGive(s_rx_mutex);
    return ESP_OK;
}
