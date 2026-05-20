
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
#include "rmt_led_driver.h"
#include "i2s_driver.h"

void task_make_led_signal(void *param);
void task_make_audio_signal(void *param);

static const char *TAG = "MAIN";

struct {
    esp_chip_info_t chip_info;
    rmt_led_t led_rmt;
    i2s_audio_t audio_i2s;

} SLT;


QueueHandle_t xAudioBufferList;
QueueHandle_t xLedRequestList;

void my_init(void)
{
    /** start init */
    ESP_LOGI(TAG, "--- Start Init ---");
    
    if(rmt_led_init(&SLT.led_rmt, WS2812, USC1903) != ESP_OK) esp_restart();
    #if defined(I2S_PCM)
    if(i2s_init_pcm_tx(&SLT.audio_i2s) != ESP_OK) esp_restart();
    #elif   defined(I2S_PDM)
    if(i2s_init_pdm_tx(&SLT.audio_i2s) != ESP_OK) esp_restart();
    #endif

    ESP_LOGI(TAG, "--- End Init ---");
}

void app_main(void)
{
    /** init system */
    my_init();

    /** creat Queue */
    xAudioBufferList = xQueueCreate(10, sizeof(audio_buf_t));
    xLedRequestList =  xQueueCreate(10, sizeof(request_strip_led_t)); 

    /** creat Task */
    xTaskCreate(task_make_led_signal, "task_make_led_signal", 1024, NULL, 4, NULL);
    xTaskCreate(task_make_audio_signal, "task_make_audio_signal", 1024, NULL, 4, NULL);
    
}

/**
 * @brief   make signal rmt to control led
 * @details
 *  - send queue xLedRequestList to make request for this task
 * @note
 *  - rmt not copy value of payload, rmt save pointer to payload. So don't update value payload before transmited -> use rmt_tx_wait_all_done.
 */
void task_make_led_signal(void *param)
{
    ESP_LOGI(TAG, "task_strip_led run");

    while(1) 
    {
        request_strip_led_t req_tmp = {.channel0 = {.data = NULL, .tot_byte = 0}, .channel1 = {.data = NULL, .tot_byte = 0}};
        bool channel0_requested = false; bool channel1_requested = false;


        if(xQueueReceive(xLedRequestList, &req_tmp, portMAX_DELAY) == pdTRUE) {

            /** transmit channel0 if channel0 have new effect request */
            if(req_tmp.channel0.data != NULL) {

                esp_err_t ret = rmt_transmit(SLT.led_rmt.channel0.handl, 
                    SLT.led_rmt.channel0.encoder.handl, 
                    req_tmp.channel0.data, 
                    req_tmp.channel0.tot_byte, &SLT.led_rmt.channel0.trans_conf); 
                ESP_ERROR_CHECK(ret);
                channel0_requested = true;

            }

            /** transmit channel1 if channel1 have new effect request */
            if(req_tmp.channel1.data != NULL) {

                esp_err_t ret = rmt_transmit(SLT.led_rmt.channel1.handl, 
                    SLT.led_rmt.channel1.encoder.handl, 
                    req_tmp.channel1.data, 
                    req_tmp.channel1.tot_byte, &SLT.led_rmt.channel1.trans_conf); 
                ESP_ERROR_CHECK(ret);
                channel1_requested = true;

            }

            /** wait channel0 sent and free heap memory */
            if(channel0_requested == true) {
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
                if(req_tmp.channel0.data != NULL) {
                    free(req_tmp.channel0.data); 
                    req_tmp.channel0.data = NULL;
                }
                channel0_requested = false;
            }

            /** wait channel1 sent and free heap memory */
            if(channel1_requested == true) {
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
                if(req_tmp.channel1.data != NULL) {
                    free(req_tmp.channel1.data); 
                    req_tmp.channel1.data = NULL;
                }
                channel1_requested = false;
            }            

        }
        vTaskDelay(pdMS_TO_TICKS(1));        
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

                    int16_t *s = (int16_t*)((uint8_t*)(buf_tmp.data) + offset);

                    size_t sample_count = send / 2;
                    
                    /** swap data because i2s is big endian */
                    for (size_t i = 0; i + 1 < sample_count; i += 2)
                    {
                        int16_t t = s[i];

                        s[i] = s[i + 1];

                        s[i + 1] = t;
                    }

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
