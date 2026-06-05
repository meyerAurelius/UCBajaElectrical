/* ESPNOW + SD CSV Logger for ESP32-2432S028 / CYD
 *
 * Logs received ESP-NOW payload data to:
 *
 *     /sdcard/espnow_log.csv
 *
 * CSV format:
 *
 *     time_ms,source_mac,packet_type,sequence,temperature_c,speed
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_timer.h"

#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "espnow_example.h"

#include <errno.h>

#define ESPNOW_MAXDELAY 512

static const char *TAG = "espnow_example";

static bool s_espnow_sd_logger_started = false;

static QueueHandle_t s_example_espnow_queue = NULL;

static uint8_t s_example_broadcast_mac[6] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

static void example_espnow_deinit(example_espnow_send_param_t *send_param);


/* -------------------------------------------------------------------------- */
/*                           CYD SD CARD CSV LOGGER                           */
/* -------------------------------------------------------------------------- */

#define MOUNT_POINT "/sdcard"
#define CSV_PATH    MOUNT_POINT "/LOG.CSV"

/*
 * ESP32-2432S028 / CYD built-in microSD slot pins.
 *
 * SD card uses SPI:
 *
 * CS   -> GPIO 5
 * MOSI -> GPIO 23
 * MISO -> GPIO 19
 * SCK  -> GPIO 18
 */
#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK  18
#define SD_PIN_NUM_CS   5

static sdmmc_card_t *s_sd_card = NULL;
static bool s_sd_mounted = false;
/*
 * The LCD driver also initializes an SPI bus.  We keep the SDSPI default
 * initializer, but force the SD card to SPI3_HOST inside sd_card_mount()
 * so it does not collide with the LCD bus.
 */
static sdmmc_host_t s_sd_host = SDSPI_HOST_DEFAULT();

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static esp_err_t sd_card_mount(void)
{
    /*
     * Avoid colliding with the LCD SPI bus.
     * If your LCD code uses SPI3_HOST, change this to SPI2_HOST instead.
     */
    s_sd_host.slot = SPI3_HOST;

    ESP_LOGI(TAG, "Initializing SD card over SPI host %d...", s_sd_host.slot);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 1,
        .allocation_unit_size = 4 * 1024,
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(
        s_sd_host.slot,
        &bus_cfg,
        SDSPI_DEFAULT_DMA
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = s_sd_host.slot;

    ESP_LOGI(TAG, "Mounting SD card...");

    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &s_sd_host,
        &slot_config,
        &mount_config,
        &s_sd_card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        spi_bus_free(s_sd_host.slot);
        return ret;
    }

    s_sd_mounted = true;

    ESP_LOGI(TAG, "SD card mounted successfully");
    sdmmc_card_print_info(stdout, s_sd_card);

    return ESP_OK;
}

static void sd_card_unmount(void)
{
    if (s_sd_mounted && s_sd_card != NULL) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_sd_card);
        s_sd_card = NULL;
        s_sd_mounted = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }

    spi_bus_free(s_sd_host.slot);
}


static bool s_csv_ready = false;

static esp_err_t csv_logger_init(void)
{
    if (!s_sd_mounted) {
        ESP_LOGE(TAG, "SD card is not mounted");
        s_csv_ready = false;
        return ESP_FAIL;
    }

    bool exists = file_exists(CSV_PATH);

    FILE *f = fopen(CSV_PATH, "a");

    if (f == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to open CSV file for append: path=%s errno=%d (%s)",
            CSV_PATH,
            errno,
            strerror(errno)
        );
        return ESP_FAIL;
    }

    if (!exists) {
        fprintf(
            f,
            "time_ms,source_mac,packet_type,sequence,temperature_c,speed\n"
        );

        fflush(f);
        fsync(fileno(f));

        ESP_LOGI(TAG, "CSV header written");
    }

    fclose(f);

    s_csv_ready = true;
    ESP_LOGI(TAG, "CSV logger ready: %s", CSV_PATH);

    return ESP_OK;
}


static esp_err_t csv_logger_append_espnow(
    const uint8_t *mac_addr,
    int packet_type,
    uint16_t seq,
    float temperature,
    float speed
)
{
    if (!s_sd_mounted || !s_csv_ready) {
        ESP_LOGW(TAG, "CSV logger not ready, skipping log");
        return ESP_FAIL;
    }

    FILE *f = fopen(CSV_PATH, "a");
    if (f == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to open CSV file for append: path=%s errno=%d (%s)",
            CSV_PATH,
            errno,
            strerror(errno)
        );
        return ESP_FAIL;
    }

    uint32_t time_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

    fprintf(
        f,
        "%lu," MACSTR ",%d,%u,%.6f,%.6f\n",
        (unsigned long)time_ms,
        MAC2STR(mac_addr),
        packet_type,
        seq,
        temperature,
        speed
    );

    /*
     * Safe but slower:
     * fflush() clears the C stdio buffer.
     * fsync() asks the filesystem/storage layer to commit the write.
     *
     * If you log at a high packet rate, optimize this later by keeping
     * the file open and syncing every N packets instead.
     */
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    return ESP_OK;
}


/* -------------------------------------------------------------------------- */
/*                                  WIFI INIT                                 */
/* -------------------------------------------------------------------------- */

static void example_wifi_init(void)
{
    esp_err_t ret;

    /*
     * esp_netif_init() may already have been called elsewhere.
     * ESP_ERR_INVALID_STATE means it was already initialized, which is OK.
     */
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    /*
     * The default event loop can only be created once.
     * If it already exists, continue.
     */
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /*
     * Wi-Fi may already be initialized by another part of your project.
     */
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_INIT_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    /*
     * This is safe if you are only using ESP-NOW.
     * If another part of your app needs APSTA or STA with internet,
     * use WIFI_MODE_APSTA or keep the existing mode instead.
     */
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));

    /*
     * esp_wifi_start() returns ESP_ERR_WIFI_NOT_INIT if init failed,
     * but if Wi-Fi is already started this may return ESP_ERR_WIFI_CONN.
     * Usually ESP_OK is expected here.
     */
    ret = esp_wifi_start();
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_CONN) {
        ESP_ERROR_CHECK(ret);
    }

    ESP_ERROR_CHECK(esp_wifi_set_channel(
        CONFIG_ESPNOW_CHANNEL,
        WIFI_SECOND_CHAN_NONE
    ));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(
        ESPNOW_WIFI_IF,
        WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR
    ));
#endif
}

/* -------------------------------------------------------------------------- */
/*                              ESPNOW CALLBACKS                              */
/* -------------------------------------------------------------------------- */

/*
 * ESPNOW sending or receiving callback function is called in WiFi task.
 * Do not do lengthy operations from this task.
 * Instead, post data to a queue and handle it from a lower priority task.
 */
static void example_espnow_send_cb(
    const esp_now_send_info_t *tx_info,
    esp_now_send_status_t status
)
{
    example_espnow_event_t evt;
    example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (tx_info == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, tx_info->des_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;

    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send queue fail");
    }
}

static void example_espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data,
    int len
)
{
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    uint8_t *mac_addr = recv_info->src_addr;
    uint8_t *des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (IS_BROADCAST_ADDR(des_addr)) {
        ESP_LOGD(TAG, "Receive broadcast ESPNOW data");
    } else {
        ESP_LOGD(TAG, "Receive unicast ESPNOW data");
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);

    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }

    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;

    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}


/* -------------------------------------------------------------------------- */
/*                         ESPNOW DATA PARSE / PREPARE                       */
/* -------------------------------------------------------------------------- */

float recv_arr[2] = { -273.15f, 0.0f };
float send_data[1] = { -270.0f };

int example_espnow_data_parse(
    uint8_t *data,
    uint16_t data_len,
    uint8_t *state,
    uint16_t *seq,
    uint32_t *magic
)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)data;
    uint16_t crc = buf->crc;
    uint16_t crc_cal;

    if (data_len < sizeof(example_espnow_data_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len: %d", data_len);
        return -1;
    }

    *state = buf->state;
    *seq = buf->seq_num;
    *magic = buf->magic;

    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal != crc) {
        ESP_LOGW(TAG, "CRC mismatch");
        return -1;
    }

    /*
     * Your payload is expected to contain:
     *
     *     float[0] = temperature
     *     float[1] = speed
     */
    memcpy(recv_arr, buf->payload, 2 * sizeof(float));

    ESP_LOGI(
        TAG,
        "Received Temperature: %f, Speed: %f",
        recv_arr[0],
        recv_arr[1]
    );

    return buf->type;
}

/* Prepare ESPNOW data to be sent. */
void example_espnow_data_prepare(example_espnow_send_param_t *send_param)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(example_espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac)
        ? EXAMPLE_ESPNOW_DATA_BROADCAST
        : EXAMPLE_ESPNOW_DATA_UNICAST;

    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;

    /*
     * Fill payload with float array.
     */
    memcpy(buf->payload, send_data, sizeof(send_data));

    buf->crc = esp_crc16_le(
        UINT16_MAX,
        (uint8_t const *)buf,
        send_param->len
    );
}


/* -------------------------------------------------------------------------- */
/*                               ESPNOW TASK                                  */
/* -------------------------------------------------------------------------- */

static void example_espnow_task(void *pvParameter)
{
    example_espnow_event_t evt;
    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    uint32_t recv_magic = 0;
    bool is_broadcast = false;
    int ret;

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Start sending broadcast data");

    example_espnow_send_param_t *send_param =
        (example_espnow_send_param_t *)pvParameter;

    if (esp_now_send(
            send_param->dest_mac,
            send_param->buffer,
            send_param->len
        ) != ESP_OK) {
        ESP_LOGE(TAG, "Send error");
        example_espnow_deinit(send_param);
        vTaskDelete(NULL);
    }

    while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case EXAMPLE_ESPNOW_SEND_CB:
            {
                example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
                is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

                ESP_LOGD(
                    TAG,
                    "Send data to " MACSTR ", status: %d",
                    MAC2STR(send_cb->mac_addr),
                    send_cb->status
                );

                /*
                 * Delay before sending next packet.
                 */
                if (send_param->delay > 0) {
                    vTaskDelay(send_param->delay / portTICK_PERIOD_MS);
                }

                ESP_LOGI(
                    TAG,
                    "send data to " MACSTR,
                    MAC2STR(send_cb->mac_addr)
                );

                memcpy(
                    send_param->dest_mac,
                    send_cb->mac_addr,
                    ESP_NOW_ETH_ALEN
                );

                example_espnow_data_prepare(send_param);

                if (esp_now_send(
                        send_param->dest_mac,
                        send_param->buffer,
                        send_param->len
                    ) != ESP_OK) {
                    ESP_LOGE(TAG, "Send error");
                    example_espnow_deinit(send_param);
                    vTaskDelete(NULL);
                }

                break;
            }

            case EXAMPLE_ESPNOW_RECV_CB:
            {
                example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

                ret = example_espnow_data_parse(
                    recv_cb->data,
                    recv_cb->data_len,
                    &recv_state,
                    &recv_seq,
                    &recv_magic
                );

                free(recv_cb->data);

                /*
                 * Log immediately after a valid packet is received and parsed.
                 */
                if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST ||
                    ret == EXAMPLE_ESPNOW_DATA_UNICAST) {

                    esp_err_t log_ret = csv_logger_append_espnow(
                        recv_cb->mac_addr,
                        ret,
                        recv_seq,
                        recv_arr[0],
                        recv_arr[1]
                    );

                    if (log_ret == ESP_OK) {
                        ESP_LOGI(TAG, "Logged ESP-NOW data to SD");
                    } else {
                        ESP_LOGE(TAG, "Failed to log ESP-NOW data to SD");
                    }
                }

                if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST) {
                    /*
                     * If MAC address does not exist in peer list,
                     * add it to peer list.
                     */
                    if (esp_now_is_peer_exist(recv_cb->mac_addr) == false) {
                        esp_now_peer_info_t *peer =
                            malloc(sizeof(esp_now_peer_info_t));

                        if (peer == NULL) {
                            ESP_LOGE(TAG, "Malloc peer information fail");
                            example_espnow_deinit(send_param);
                            vTaskDelete(NULL);
                        }

                        
                        memset(peer, 0, sizeof(esp_now_peer_info_t));
                        peer->channel = CONFIG_ESPNOW_CHANNEL;
                        peer->ifidx = ESPNOW_WIFI_IF;
                        peer->encrypt = true;
                        memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                        memcpy(peer->peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);

                        ret = esp_now_add_peer(peer);

                        if (ret == ESP_ERR_ESPNOW_EXIST) {
                            ESP_LOGW(
                                TAG,
                                "Peer already exists: " MACSTR,
                                MAC2STR(recv_cb->mac_addr)
                            );
                        } else if (ret != ESP_OK) {
                            ESP_LOGE(
                                TAG,
                                "Failed to add peer " MACSTR ": %s",
                                MAC2STR(recv_cb->mac_addr),
                                esp_err_to_name(ret)
                            );

                            free(peer);
                            example_espnow_deinit(send_param);
                            vTaskDelete(NULL);
                        }

                        free(peer);
                    }

                    /*
                     * Indicates that the device has received broadcast
                     * ESPNOW data.
                     */
                    if (send_param->state == 0) {
                        send_param->state = 1;
                    }

                    /*
                     * If receive broadcast ESPNOW data which indicates that
                     * the other device has received broadcast ESPNOW data and
                     * the local magic number is bigger than that in the
                     * received broadcast ESPNOW data, stop sending broadcast
                     * data and start sending unicast ESPNOW data.
                     */
                    if (recv_state == 1) {
                        if (send_param->unicast == false &&
                            send_param->magic >= recv_magic) {

                            ESP_LOGI(TAG, "Start sending unicast data");
                            ESP_LOGI(
                                TAG,
                                "send data to " MACSTR,
                                MAC2STR(recv_cb->mac_addr)
                            );

                            memcpy(
                                send_param->dest_mac,
                                recv_cb->mac_addr,
                                ESP_NOW_ETH_ALEN
                            );

                            example_espnow_data_prepare(send_param);

                            if (esp_now_send(
                                    send_param->dest_mac,
                                    send_param->buffer,
                                    send_param->len
                                ) != ESP_OK) {
                                ESP_LOGE(TAG, "Send error");
                                example_espnow_deinit(send_param);
                                vTaskDelete(NULL);
                            } else {
                                send_param->broadcast = false;
                                send_param->unicast = true;
                            }
                        }
                    }
                }
                else if (ret == EXAMPLE_ESPNOW_DATA_UNICAST) {
                    ESP_LOGI(
                        TAG,
                        "Receive %dth unicast data from: " MACSTR ", len: %d",
                        recv_seq,
                        MAC2STR(recv_cb->mac_addr),
                        recv_cb->data_len
                    );

                    /*
                     * If receive unicast ESPNOW data, also stop sending
                     * broadcast ESPNOW data.
                     */
                    send_param->broadcast = false;
                }
                else {
                    ESP_LOGI(
                        TAG,
                        "Receive error data from: " MACSTR,
                        MAC2STR(recv_cb->mac_addr)
                    );
                }

                break;
            }

            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}


/* -------------------------------------------------------------------------- */
/*                              ESPNOW INIT/DEINIT                            */
/* -------------------------------------------------------------------------- */

static esp_err_t example_espnow_init(void)
{
    esp_err_t ret;
    example_espnow_send_param_t *send_param;

    if (s_example_espnow_queue != NULL) {
        ESP_LOGW(TAG, "ESP-NOW queue already exists, logger task likely already started");
        return ESP_OK;
    }

    s_example_espnow_queue = xQueueCreate(
        ESPNOW_QUEUE_SIZE,
        sizeof(example_espnow_event_t)
    );

    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    /*
     * ESP-NOW may already have been initialized by another part of the app.
     * Do not abort on duplicate initialization.
     */
    ret = esp_now_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ESP-NOW initialized");
    } else {
        ESP_LOGW(TAG, "esp_now_init returned %s, continuing", esp_err_to_name(ret));
    }

    ret = esp_now_register_send_cb(example_espnow_send_cb);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_register_send_cb returned %s", esp_err_to_name(ret));
    }

    ret = esp_now_register_recv_cb(example_espnow_recv_cb);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_register_recv_cb returned %s", esp_err_to_name(ret));
    }

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ret = esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_set_wake_window returned %s", esp_err_to_name(ret));
    }

    ret = esp_wifi_connectionless_module_set_wake_interval(
        CONFIG_ESPNOW_WAKE_INTERVAL
    );
    if (ret != ESP_OK) {
        ESP_LOGW(
            TAG,
            "esp_wifi_connectionless_module_set_wake_interval returned %s",
            esp_err_to_name(ret)
        );
    }
#endif

    ret = esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_set_pmk returned %s", esp_err_to_name(ret));
    }

    /*
     * Add broadcast peer information to peer list.
     * Tolerate duplicates because another module may already have added it.
     */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));

    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
        return ESP_FAIL;
    }

    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);

    if (esp_now_is_peer_exist(s_example_broadcast_mac)) {
        ESP_LOGW(TAG, "Broadcast peer already exists, skipping add");
    } else {
        ret = esp_now_add_peer(peer);

        if (ret == ESP_ERR_ESPNOW_EXIST) {
            ESP_LOGW(TAG, "Broadcast peer already exists");
        } else if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Failed to add broadcast peer: %s",
                esp_err_to_name(ret)
            );

            free(peer);
            vQueueDelete(s_example_espnow_queue);
            s_example_espnow_queue = NULL;
            return ret;
        }
    }

    free(peer);

    /*
     * Initialize sending parameters.
     */
    send_param = malloc(sizeof(example_espnow_send_param_t));

    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
        return ESP_FAIL;
    }

    memset(send_param, 0, sizeof(example_espnow_send_param_t));

    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param->len = CONFIG_ESPNOW_SEND_LEN;

    send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);

    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
        return ESP_FAIL;
    }

    memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    example_espnow_data_prepare(send_param);

    BaseType_t task_ret = xTaskCreate(
        example_espnow_task,
        "example_espnow_task",
        4096,
        send_param,
        4,
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ESP-NOW task");
        free(send_param->buffer);
        free(send_param);
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void example_espnow_deinit(example_espnow_send_param_t *send_param)
{
    if (send_param != NULL) {
        free(send_param->buffer);
        free(send_param);
    }

    if (s_example_espnow_queue != NULL) {
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
    }

    esp_now_deinit();
}


esp_err_t espnow_sd_logger_start(void)
{
    esp_err_t ret;

    if (s_espnow_sd_logger_started) {
        ESP_LOGW(TAG, "ESP-NOW SD logger already started");
        return ESP_OK;
    }

    /*
     * NVS may already be initialized by the main app.
     */
    ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    /*
     * Wi-Fi must exist before ESP-NOW.
     * This version tolerates repeated esp_netif/event loop/Wi-Fi init.
     */
    example_wifi_init();

    /*
     * Mount SD card before ESP-NOW starts receiving.
     * If SD fails, continue without CSV logging.
     */
    ret = sd_card_mount();

    if (ret == ESP_OK) {
        ret = csv_logger_init();

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize CSV logger. Continuing without CSV logging.");
            s_csv_ready = false;
        }
    } else {
        ESP_LOGE(TAG, "SD card unavailable. Continuing without CSV logging.");
        s_sd_mounted = false;
        s_csv_ready = false;
    }

    /*
     * Start ESP-NOW.
     */
    ret = example_espnow_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW: %s", esp_err_to_name(ret));
        return ret;
    }

    s_espnow_sd_logger_started = true;

    ESP_LOGI(TAG, "ESP-NOW SD logger started");

    return ESP_OK;
}
