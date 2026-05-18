
#ifndef I2S_DRIVER_H
#define I2S_DRIVER_H


#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/i2s_std.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "driver/i2s_pdm.h"
#include "main.h"

#define I2S_PCM_TX_FREQ_HZ 48000
#define I2S_PDM_TX_FREQ_HZ 16000

typedef struct {

    i2s_chan_handle_t tx_handle;

} i2s_audio_t;


esp_err_t i2s_init_pcm_tx(i2s_audio_t *i2s_audio); 

esp_err_t i2s_init_pdm_tx(i2s_audio_t *i2s_audio);

#endif
