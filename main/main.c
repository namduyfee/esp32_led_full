
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_check.h"
#include "esp_log.h"

#include "main.h"
#include "i2s_driver.h"

void task_make_audio_signal(void *param);

static const char *TAG = "MAIN";

struct {
    esp_chip_info_t chip_info;
    i2s_audio_t audio_i2s;

} SLT;

QueueHandle_t xAudioBufferList;

void my_init(void)
{
    /** start init */
    ESP_LOGI(TAG, "--- Start Init ---");
    
    if(i2s_init_pdm_tx(&SLT.audio_i2s) != ESP_OK) esp_restart(); 

    ESP_LOGI(TAG, "--- End Init ---");
}

void app_main(void)
{
    /** init system */
    my_init();

    /** creat Queue */
    xAudioBufferList = xQueueCreate(10, sizeof(audio_buf_t));

    /** creat Task */
    xTaskCreate(task_make_audio_signal, "task_make_audio_signal", 1024, NULL, 4, NULL);
    
}

/**
 * @brief   handle audio with i2s
 * @note
 *  - both pdm , pcm in both stereo, mono mode when dma copy data, it copy 32bits in once.
 *  after that it use 16bits last before 16 bits first.
 *  ex, (int16_t)data[1] is use before (int16_t)data[0].
 *  -> actively swap pairs to get the correct result.
 */
void task_make_audio_signal(void *param)
{
    ESP_LOGI(TAG, "task_audio_i2s run");

    while(1)
    {
        audio_buf_t buf_tmp = {.data = NULL, .tot_byte = 0};

        if(xQueueReceive(xAudioBufferList, &buf_tmp, portMAX_DELAY) == pdTRUE) {

            if(buf_tmp.data != NULL) {
                size_t offset = 0;

                while (offset < buf_tmp.tot_byte)
                {
                    size_t remain = buf_tmp.tot_byte - offset;

                    size_t send = remain > buf_tmp.tot_byte ? buf_tmp.tot_byte : remain;

                    size_t written;

                    i2s_channel_write(SLT.audio_i2s.tx_handle, (uint8_t*)(buf_tmp.data) + offset,
                    send, &written, portMAX_DELAY);

                    offset += written;
                }

                free(buf_tmp.data); 
                buf_tmp.data = NULL; 
            }
                
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
