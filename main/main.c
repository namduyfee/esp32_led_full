
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
#include "dac_audio_driver.h"
#include "dac_audio_test.h"


void task_strip_led(void* param);
void task_audio_dac_internal(void* param);

struct {
    esp_chip_info_t chip_info;
    rmt_led_t led_rmt;
    dac_audio_t dac_audio;

} SLT;


void my_init(void)
{
    esp_chip_info(&SLT.chip_info);

    printf("Chip model: %d\n", SLT.chip_info.model);
    printf("CPU cores: %d\n",  SLT.chip_info.cores);
    printf("Revision: %d\n",   SLT.chip_info.revision);
    printf("Features: %lx\n",  SLT.chip_info.features);

    if(rmt_led_init(&SLT.led_rmt) != ESP_OK) esp_restart();

    if(dac_audio_init(&SLT.dac_audio) != ESP_OK) esp_restart();
}

void app_main(void)
{

    my_init();
    xTaskCreate(task_strip_led, "task_strip_led", 1024, NULL, 4, NULL);
    xTaskCreate(task_audio_dac_internal, "task_audio_dac_internal", 1024, NULL, 4, NULL);

}


#define NUM_OF_LED 2000
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
    memset(led_strip_pixels, 0xff, NUM_OF_BYTE);

    while(1) 
    {
        

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));
        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}

/**
 * @brief   creat audio signals by dac internal
 * @note
 *  - dac descriptors is like ring buffer; not index. SO start from any descriptor is not important
 *    data copied into descriptors must continuous, that's important
 */
void task_audio_dac_internal(void* param) 
{
    size_t data_size = sizeof(audio_table);
    const uint8_t* data = audio_table;

    ESP_ERROR_CHECK(dac_continuous_start_async_writing(SLT.dac_audio.continuous_handle));

    while(1) 
    {
        size_t byte_written = 0;
        dac_event_data_t evt_data;
        while(byte_written < data_size) {
            xQueueReceive(SLT.dac_audio.even_data_q, &evt_data, portMAX_DELAY);
            size_t loaded_bytes = 0;
            ESP_ERROR_CHECK(dac_continuous_write_asynchronously(SLT.dac_audio.continuous_handle, evt_data.buf, evt_data.buf_size,
                            data + byte_written, data_size - byte_written, &loaded_bytes));
            byte_written += loaded_bytes;
        }
        /** set all desc to 0 */
        for (int i = 0; i < DAC_NUM_OF_DESC; i++) {
            xQueueReceive(SLT.dac_audio.even_data_q, &evt_data, portMAX_DELAY);
            memset(evt_data.buf, 0, evt_data.buf_size);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));        
    }
}
