#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "driver/gpio.h"

/** define PIN I2S */
#define I2S_MCLK_PIN GPIO_NUM_NC
#define I2S_BCLK_PIN GPIO_NUM_18
#define I2S_WS_PIN GPIO_NUM_25
#define I2S_DOUT_PIN GPIO_NUM_26
#define I2S_DIN_PIN GPIO_NUM_NC

/** define PIN RMT LED */
#define RMT_CHANNEL0_GPIO_NUM   GPIO_NUM_2
#define RMT_CHANNEL1_GPIO_NUM   GPIO_NUM_4

#endif
