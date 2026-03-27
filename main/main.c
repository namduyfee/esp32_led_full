/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_check.h"

#include "rmt_led_driver.h"


#define NUM_OF_LED 10
#define NUM_OF_BYTE (NUM_OF_LED * 3)
rmt_led_t slt_rmt;

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

void app_main(void)
{
    rmt_led_init(&slt_rmt);
    uint8_t led_strip_pixels[NUM_OF_BYTE];

    while(1) 
    {
        set_red_color(led_strip_pixels, NUM_OF_BYTE);
        ESP_ERROR_CHECK(rmt_transmit(slt_rmt.channel1.handl, slt_rmt.channel1.encoder.handl,led_strip_pixels, sizeof(led_strip_pixels), &slt_rmt.channel1.trans_conf));
        ESP_ERROR_CHECK(rmt_transmit(slt_rmt.channel0.handl, slt_rmt.channel0.encoder.handl,led_strip_pixels, sizeof(led_strip_pixels), &slt_rmt.channel0.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(slt_rmt.channel1.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(slt_rmt.channel0.handl, portMAX_DELAY));
        

        // vTaskDelay(pdMS_TO_TICKS(1000));

        // set_white_color(led_strip_pixels, NUM_OF_BYTE);

        // ESP_ERROR_CHECK(rmt_transmit(slt_rmt.channel0.handl, slt_rmt.encoder.handl,led_strip_pixels, sizeof(led_strip_pixels), &slt_rmt.trans_conf));
        // ESP_ERROR_CHECK(rmt_tx_wait_all_done(slt_rmt.channel0.handl, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}
