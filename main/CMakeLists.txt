idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES rmt_driver i2s_driver dac_driver spi_flash
                       INCLUDE_DIRS ".")
