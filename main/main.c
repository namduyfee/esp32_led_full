
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

#define NUM_OF_LED 2000
#define NUM_OF_BYTE (NUM_OF_LED * 3)

void task_strip_led(void* param);

struct {
    esp_chip_info_t chip_info;
    rmt_led_t led_rmt;
} SLT;

void set_red_color(uint8_t* payload, uint32_t tot_byte_payload)
{
    for(int i = 0; i < tot_byte_payload; i += 3) {
        payload[i] = 0xFF;
        payload[i + 1] = 0;
        payload[i + 2] = 0;
    }
}

void set_white_color(uint8_t* payload, uint32_t tot_byte_payload)
{
    for(int i = 0; i < tot_byte_payload; i += 3) {
        payload[i] = 0xFF;
        payload[i + 1] = 0xFF;
        payload[i + 2] = 0xFF;
    }
}

void my_init(void)
{
    esp_chip_info(&SLT.chip_info);

    printf("Chip model: %d\n", SLT.chip_info.model);
    printf("CPU cores: %d\n",  SLT.chip_info.cores);
    printf("Revision: %d\n",   SLT.chip_info.revision);
    printf("Features: %lx\n",  SLT.chip_info.features);

    if(SLT.chip_info.model == 2 || SLT.chip_info.model == 5 || SLT.chip_info.model == 6) {
        if(rmt_led_init(&SLT.led_rmt, true) != ESP_OK) esp_restart();
    }
    else
        if(rmt_led_init(&SLT.led_rmt, false) != ESP_OK) esp_restart();

}

void app_main(void)
{

    my_init();
    xTaskCreate(task_strip_led, "task_strip_led", 1024, NULL, 4, NULL);

}

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
        
        set_red_color(led_strip_pixels, NUM_OF_BYTE);
        
        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));
        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,led_strip_pixels, NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}