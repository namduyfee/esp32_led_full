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

#define I2S_AUDIO_RATE 48000

/*!
 *  clear dma memory call in cpu task 
 */
#define I2S_CLEAR_DMA_MEM(tx_handle, dma_desc_num, dma_frame_num) do {    \
        size_t tot_byte_each_desc = dma_frame_num * 4; \
        uint8_t *data_clear = malloc(tot_byte_each_desc); \
        if(data_clear == NULL) { \
            printf("allocate error\n"); \
            break; \
        } \
        memset(data_clear, 0, tot_byte_each_desc); \
        size_t byte_cleared = 0; \
        while(byte_cleared < (dma_desc_num * tot_byte_each_desc)) { \
            size_t byte_write = tot_byte_each_desc < (dma_desc_num * tot_byte_each_desc) - byte_cleared ? \
                                tot_byte_each_desc : (dma_desc_num * tot_byte_each_desc) - byte_cleared; \
            esp_err_t ret = i2s_channel_write(tx_handle, data_clear, byte_write, &byte_loadded, portMAX_DELAY); \
            if(ret == ESP_OK) \
                byte_cleared += byte_loadded;   \
        } \
        if(data_clear != NULL) free(data_clear); \
        printf("clear success byte cleared : %d\n", byte_cleared); \
}while(0)


typedef struct {

    i2s_chan_handle_t tx_handle;

    i2s_chan_config_t chan_cfg; 

    i2s_std_config_t std_tx_cfg;

} i2s_audio_t;
esp_err_t i2s_audio_init(i2s_audio_t* i2s_audio);
#endif
