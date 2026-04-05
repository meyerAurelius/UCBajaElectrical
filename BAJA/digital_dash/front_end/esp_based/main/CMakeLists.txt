idf_component_register(SRCS demo.c lcd.c touch.c espnow_example_main.c
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd driver esp_timer esp_wifi nvs_flash esp_netif esp_system
                    )
