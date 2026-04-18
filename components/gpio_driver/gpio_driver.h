#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "driver/gpio.h"
#include "main.h"

/** define PIN RMT LED */
#define RMT_CHANNEL0_GPIO_NUM   GPIO_NUM_4
#define RMT_CHANNEL1_GPIO_NUM   GPIO_NUM_12

/** define PIN I2S */
//#define I2S_MCLK_PIN GPIO_NUM_0

#if defined(I2S_PCM)
#define I2S_MCLK_PIN GPIO_NUM_NC
#define I2S_BCLK_PIN GPIO_NUM_27
#define I2S_WS_PIN GPIO_NUM_32
#define I2S_DOUT_PIN GPIO_NUM_33
#define I2S_DIN_PIN GPIO_NUM_NC
#elif defined(I2S_PDM)
#define I2S_CLK_PIN GPIO_NUM_27
#define I2S_DOUT_PIN GPIO_NUM_33
#endif

/** define PIN SPI */
#define SPI_MOSI_PIN GPIO_NUM_23
#define SPI_MISO_PIN GPIO_NUM_19
#define SPI_SCK_PIN GPIO_NUM_18
#define SPI_CS_PIN GPIO_NUM_5

#endif
