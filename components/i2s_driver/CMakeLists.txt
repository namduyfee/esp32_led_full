idf_component_register(SRCS "i2s_driver.c"
                       REQUIRES esp_driver_i2s gpio_driver main
                       INCLUDE_DIRS ".")
