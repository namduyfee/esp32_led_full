idf_component_register(SRCS "gpio_driver.c"
                        REQUIRES 
                            main
                            esp_driver_gpio
                       INCLUDE_DIRS ".")
