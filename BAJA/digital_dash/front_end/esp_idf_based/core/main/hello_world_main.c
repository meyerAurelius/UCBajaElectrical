/*
 * ESP32 CYD / ILI9341 basic bring-up example
 * with simple full-screen color test
 *
 * Assumed board: ESP32-2432S028R ("Cheap Yellow Display")
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"

// -----------------------------
// CYD pin configuration
// -----------------------------
#define LCD_HOST                SPI2_HOST

#define LCD_MISO                12
#define LCD_MOSI                13
#define LCD_PCLK                14
#define LCD_CS                  15
#define LCD_DC                  2
#define LCD_RST                 -1      // Common on many CYD boards
#define LCD_BK_LIGHT            21

#define LCD_H_RES               240
#define LCD_V_RES               320
#define LCD_PIXEL_CLOCK_HZ      (40 * 1000 * 1000)

// Draw in chunks to avoid huge buffers
#define DRAW_BUF_LINES          20

static const char *TAG = "CYD_LCD";

typedef struct {
    int dummy;
} lcd_callback_ctx_t;

static lcd_callback_ctx_t example_callback_ctx = {0};

static bool example_callback(esp_lcd_panel_io_handle_t panel_io,
                             esp_lcd_panel_io_event_data_t *edata,
                             void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    (void)user_ctx;
    return false;
}

static void lcd_backlight_init(void)
{
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << LCD_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(LCD_BK_LIGHT, 1));
}

static void fill_screen_color(esp_lcd_panel_handle_t panel, uint16_t color)
{
    const int buf_pixels = LCD_H_RES * DRAW_BUF_LINES;
    uint16_t *buf = heap_caps_malloc(buf_pixels * sizeof(uint16_t), MALLOC_CAP_DMA);

    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        return;
    }

    for (int i = 0; i < buf_pixels; i++) {
        buf[i] = color;
    }

    for (int y = 0; y < LCD_V_RES; y += DRAW_BUF_LINES) {
        int lines = DRAW_BUF_LINES;
        if ((y + lines) > LCD_V_RES) {
            lines = LCD_V_RES - y;
        }

        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
            panel,
            0,
            y,
            LCD_H_RES,
            y + lines,
            buf));
    }

    free(buf);
}

void app_main(void)
{
    // -----------------------------
    // Backlight setup
    // -----------------------------
    lcd_backlight_init();

    // -----------------------------
    // Initialize SPI bus
    // -----------------------------
    ESP_LOGI(TAG, "Initialize SPI bus");

    spi_bus_config_t bus_config = {
        .sclk_io_num = LCD_PCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * DRAW_BUF_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // -----------------------------
    // Install panel IO
    // -----------------------------
    ESP_LOGI(TAG, "Install panel IO");

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_callback,
        .user_ctx = &example_callback_ctx,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST,
        &io_config,
        &io_handle));

    // -----------------------------
    // Install ILI9341 panel driver
    // -----------------------------
    ESP_LOGI(TAG, "Install ILI9341 panel driver");

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Orientation tweak for CYD
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "LCD initialized");

    // -----------------------------
    // Simple LCD color test
    // RGB565 colors
    // -----------------------------
    while (1) {
        ESP_LOGI(TAG, "Fill RED");
        fill_screen_color(panel_handle, 0xF800);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Fill GREEN");
        fill_screen_color(panel_handle, 0x07E0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Fill BLUE");
        fill_screen_color(panel_handle, 0x001F);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Fill WHITE");
        fill_screen_color(panel_handle, 0xFFFF);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Fill BLACK");
        fill_screen_color(panel_handle, 0x0000);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Print chip info once per cycle
        printf("Hello world!\n");

        esp_chip_info_t chip_info;
        uint32_t flash_size;

        esp_chip_info(&chip_info);

        printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
               CONFIG_IDF_TARGET,
               chip_info.cores,
               (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
               (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
               (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
               (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

        unsigned major_rev = chip_info.revision / 100;
        unsigned minor_rev = chip_info.revision % 100;
        printf("silicon revision v%d.%d, ", major_rev, minor_rev);

        if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
            printf("%" PRIu32 "MB %s flash\n",
                   flash_size / (uint32_t)(1024 * 1024),
                   (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        } else {
            printf("Get flash size failed\n");
        }

        printf("Minimum free heap size: %" PRIu32 " bytes\n",
               esp_get_minimum_free_heap_size());
    }
}