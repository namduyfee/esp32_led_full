idf_component_register(SRCS "main.c"
                        PRIV_REQUIRES
                            i2s_driver
                            gpio_driver
                       INCLUDE_DIRS "."
                       )
