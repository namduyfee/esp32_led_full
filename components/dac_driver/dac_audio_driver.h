
#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "main.h"

#include "driver/dac_types.h"
#include "driver/dac_continuous.h"

#define DAC_AUDIO_SAMPLE_RATE 48000
#define DAC_NUM_OF_DESC 5

typedef struct {

    dac_continuous_handle_t continuous_handle;
    QueueHandle_t even_data_q;

} dac_audio_t;

esp_err_t dac_audio_init(dac_audio_t* dac_audio);

#endif
