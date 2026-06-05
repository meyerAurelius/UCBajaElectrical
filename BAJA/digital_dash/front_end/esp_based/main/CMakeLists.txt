idf_component_register(SRCS lcd.c touch.c demo_update.c controller.c images/engine-temp-light.c images/flag_icon.c images/pressure_icon.c
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd driver esp_timer esp_wifi nvs_flash esp_netif esp_system fatfs sdmmc
                    )





#removed for testing: espnow_example_main.c, demo.c
#added for testing: demo_update.c, controller.c, flag_icon.c, pressure_icon.c
