
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_check.h"
#include "esp_log.h"

#include "main.h"
#include "rmt_led_driver.h"
#include "i2s_driver.h"


void task_strip_led(void* param);
void task_audio_i2s(void* param);

struct {
    esp_chip_info_t chip_info;
    rmt_led_t led_rmt;
    i2s_audio_t audio_i2s;

} SLT;


void my_init(void)
{
    esp_chip_info(&SLT.chip_info);

    printf("Chip model: %d\n", SLT.chip_info.model);
    printf("CPU cores: %d\n",  SLT.chip_info.cores);
    printf("Revision: %d\n",   SLT.chip_info.revision);
    printf("Features: %lx\n",  SLT.chip_info.features);

    //if(rmt_led_init(&SLT.led_rmt) != ESP_OK) esp_restart();
    if(i2s_audio_init(&SLT.audio_i2s) != ESP_OK) esp_restart();
}

void app_main(void)
{
    my_init();
    //xTaskCreate(task_strip_led, "task_strip_led", 1024, NULL, 4, NULL);
    xTaskCreate(task_audio_i2s, "task_audio_i2s", 1024, NULL, 4, NULL);
}

#define NUM_OF_LED 255
#define NUM_OF_BYTE (NUM_OF_LED * 3)
/**
 * @brief   control led
 * @note
 *  - rmt note copy value of payload, rmt save pointer to payload. So don't update value payload before transmited -> use rmt_tx_wait_all_done.
 * 
 */
void task_strip_led(void* param)
{
    uint8_t* led_strip_pixels = (uint8_t*)malloc(NUM_OF_BYTE);
    
    while(1) 
    {
        memset(led_strip_pixels, 0x22, NUM_OF_BYTE);

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));
        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));

        memset(led_strip_pixels, 0x00, NUM_OF_BYTE);

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));
        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));        
    }

}

#define NUM_SLOT 15000
/**
 * @brief   handle audio with i2s
 * @note
 * 
 * 
 */
void task_audio_i2s(void* param)
{

    uint16_t *data = malloc(NUM_SLOT * sizeof(uint16_t));
    uint8_t *data_8bit = (uint8_t*)data;
    while(1)
    {
        size_t byte_written = 0; 
        size_t byte_loadded = 0;
        for(int j = 0; j < NUM_SLOT; j++) data[j] = j;
        while(byte_written < NUM_SLOT * sizeof(uint16_t)) {
            esp_err_t ret = i2s_channel_write(SLT.audio_i2s.tx_handle, data_8bit + byte_written, NUM_SLOT * sizeof(uint16_t) - byte_written, &byte_loadded, portMAX_DELAY);
            if(ret == ESP_OK) {
                byte_written += byte_loadded;
            }
        }
        I2S_CLEAR_DMA_MEM(SLT.audio_i2s.tx_handle, SLT.audio_i2s.chan_cfg.dma_desc_num, SLT.audio_i2s.chan_cfg.dma_frame_num);

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }

}

