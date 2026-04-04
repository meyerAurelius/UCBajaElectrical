idf_component_register(SRCS demo.c lcd.c touch.c espnow_float_rx.c
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd driver esp_timer esp_wifi nvs_flash esp_netif
                    )
