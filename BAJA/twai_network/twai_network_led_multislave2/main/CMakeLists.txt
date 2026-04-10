idf_component_register(
    SRCS
        "twai_network_led_multislave2.c"
        "led_strip_encoder.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)