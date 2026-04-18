idf_component_register(SRCS "main.c"
                        PRIV_REQUIRES 
                            rmt_driver 
                            i2s_driver 
                            dac_driver 
                            gpio_driver
                       INCLUDE_DIRS ".")
