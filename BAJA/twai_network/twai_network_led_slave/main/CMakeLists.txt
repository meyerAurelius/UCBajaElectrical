idf_component_register(
    SRCS
        "twai_network_led_slave_main.c"
        "led_strip_encoder.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)