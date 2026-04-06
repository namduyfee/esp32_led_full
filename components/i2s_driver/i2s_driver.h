#ifndef I2S_DRIVER_H
#define I2S_DRIVER_H

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "main.h"

#include "driver/i2s_std.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "driver/gpio.h"

typedef struct {


} i2s_audio_t;

void i2s_audio_init(void);
#endif
