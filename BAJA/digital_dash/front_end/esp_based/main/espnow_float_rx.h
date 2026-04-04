#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_now.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*espnow_float_rx_cb_t)(const uint8_t src_mac[ESP_NOW_ETH_ALEN],
                                     const float *values,
                                     size_t count);

esp_err_t espnow_float_rx_init(uint8_t wifi_channel, espnow_float_rx_cb_t cb);
void espnow_float_rx_deinit(void);

esp_err_t espnow_float_rx_get_last(float *out_values,
                                   size_t max_count,
                                   size_t *out_count,
                                   uint8_t out_src_mac[ESP_NOW_ETH_ALEN]);

#ifdef __cplusplus
}
#endif
