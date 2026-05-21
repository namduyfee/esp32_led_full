idf_component_register(SRCS "rmt_led_driver.c" "rmt_led_encoder.c"
                       REQUIRES esp_driver_rmt gpio_driver main
                       INCLUDE_DIRS ".")
