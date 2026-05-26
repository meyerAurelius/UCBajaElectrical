idf_component_register(SRCS
                        espnow_example_main.c 
                        thermistor_data.c

                        "gps_module.c"
                        "json_payload.c"
                        "imu_sensor_reading.c"
                        "gps_sensor_reading.c"
                        "temp_sensor_reading.c"
                        "pressure_sensor_reading.c"
                        "rpm_sensor_reading.c"
                        "voltage_sensor_reading.c"
                        "sensor_reading_helpers.c"

                        INCLUDE_DIRS ".")

