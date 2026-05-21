
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

void task_test_audio(void *param);
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
    xTaskCreate(task_test_audio, "task_test_audio", 1024, NULL, 4, NULL);
    xTaskCreate(task_make_audio_signal, "task_make_audio_signal", 1024, NULL, 4, NULL);
    
}

#define MAX_SAMPLE 1024
#define BYTE_OF_EACH_SAMP 2
#define MAX_BYTE_OF_DATA (MAX_SAMPLE * BYTE_OF_EACH_SAMP)
extern const uint8_t pcm_start[] asm("_binary_dlb_pcm_start");
extern const uint8_t pcm_end[]   asm("_binary_dlb_pcm_end");

/**
 * @brief   take audio data and send to task make audio signal
 */
void task_test_audio(void *param)
{
    audio_buf_t buf_tem;

    const uint8_t *pcm = pcm_start;
    size_t pcm_size = pcm_end - pcm_start;
    
    int size_tem = pcm_size;

    bool sent = false;
    int16_t last_sample_value = 0;
    while(1) {
        while(size_tem > 0) {
            size_t tot_byte = size_tem > MAX_BYTE_OF_DATA ? MAX_BYTE_OF_DATA : size_tem;
            buf_tem.data = malloc(tot_byte);

            if(buf_tem.data != NULL) {
                buf_tem.tot_byte = tot_byte;
                memcpy(buf_tem.data, pcm + pcm_size - size_tem, tot_byte);
                if(xQueueSend(xAudioBufferList, &buf_tem, portMAX_DELAY) == pdTRUE) {
                    size_tem -= tot_byte;
                    memcpy(&last_sample_value, (uint8_t*)buf_tem.data + tot_byte - 2, 2); 
                }
            }
            sent = true;
            //vTaskDelay(pdMS_TO_TICKS(1));
        }
        if(sent == true) {
            int16_t* fade_buf = malloc(64 * sizeof(int16_t));
            for (int i = 0; i < 64; i++) {
                fade_buf[i] = (int16_t)(last_sample_value * (64 - i) / 64);
                printf("%d ", fade_buf[i]); 
            }
            
            buf_tem.data = fade_buf;
            buf_tem.tot_byte = 64 * sizeof(int16_t);
            xQueueSend(xAudioBufferList, &buf_tem, portMAX_DELAY);
            printf("last sample : %d\n", last_sample_value); 
            size_tem = pcm_size;
            sent = false;
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
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
