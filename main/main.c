
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


void task_strip_led(void* param);
void task_audio_i2s(void* param);

static const char *TAG = "MAIN";

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
    my_init();
    xTaskCreate(task_strip_led, "task_strip_led", 1024, NULL, 4, NULL);
    xTaskCreate(task_audio_i2s, "task_audio_i2s", 1024, NULL, 4, NULL);
}

#define NUM_OF_LED 1024
#define NUM_OF_BYTE (NUM_OF_LED * 3)
/**
 * @brief   control led
 * @note
 *  - rmt not copy value of payload, rmt save pointer to payload. So don't update value payload before transmited -> use rmt_tx_wait_all_done.
 *  - heap memory is used to store effect for 2 channel 
 */
void task_strip_led(void* param)
{
    ESP_LOGI(TAG, "task_strip_led running");

    uint32_t free_heap = esp_get_free_heap_size();
    printf("Free heap size before: %ld bytes\n", free_heap);
    
    uint8_t* channel0_pixels = (uint8_t*)malloc(NUM_OF_BYTE);
    uint8_t* channel1_pixels = (uint8_t*)malloc(NUM_OF_BYTE);

    free_heap = esp_get_free_heap_size();
    printf("Free heap size after: %ld bytes\n", free_heap);

    while(1) 
    {

        memset(channel0_pixels, 0x22, NUM_OF_BYTE);
        memset(channel1_pixels, 0x22, NUM_OF_BYTE);

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,channel0_pixels, 
            NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,channel1_pixels, 
            NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));

        memset(channel0_pixels, 0x00, NUM_OF_BYTE);
        memset(channel1_pixels, 0x00, NUM_OF_BYTE);

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel0.handl, SLT.led_rmt.channel0.encoder.handl,channel0_pixels, 
            NUM_OF_BYTE, &SLT.led_rmt.channel0.trans_conf));

        ESP_ERROR_CHECK(rmt_transmit(SLT.led_rmt.channel1.handl, SLT.led_rmt.channel1.encoder.handl,channel1_pixels, 
            NUM_OF_BYTE, &SLT.led_rmt.channel1.trans_conf));
        
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel0.handl, portMAX_DELAY));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(SLT.led_rmt.channel1.handl, portMAX_DELAY));
        
        vTaskDelay(pdMS_TO_TICKS(500));        
    }

}

#define NUM_SLOT 50

/**
 * @brief   handle audio with i2s
 * @note
 *  - both pdm , pcm in both stereo, mono mode when dma copy data, it copy 32bits in once.
 *  after that it use 16bits last before 16 bits first.
 *  ex, (int16_t)data[1] is use before (int16_t)data[0].
 *  -> actively swap pairs to get the correct result.
 * 
 * 
 */
#if defined(I2S_PDM)

//#define TEST_SIN
#if defined(TEST_SIN)
#include "math.h"
#define SAMPLES_PER_CYCLE 100  // Số mẫu trong 1 chu kỳ (48kHz / 1kHz)
#define AMPLITUDE 32767

int16_t sin_wave[SAMPLES_PER_CYCLE];
void generate_sin_wave() {
    for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
        // Tính giá trị sin từ -1.0 đến 1.0, sau đó nhân với biên độ
        sin_wave[i] = (int16_t)(AMPLITUDE * sin(2 * M_PI * i / SAMPLES_PER_CYCLE));
    }
}
#else

extern const uint8_t hello_wav_start[] asm("_binary_hello_wav_start");
extern const uint8_t hello_wav_end[]   asm("_binary_hello_wav_end");


#endif

void task_audio_i2s(void* param)
{

    ESP_LOGI(TAG, "task_audio_i2s running");
    
    #if defined(TEST_SIN)
    generate_sin_wave();

    uint8_t pulse_num =15; 

    int16_t *data = malloc(SAMPLES_PER_CYCLE * pulse_num * sizeof(int16_t));
    int8_t* data_i8 = (int8_t*)data;

    for(int i = 0; i < SAMPLES_PER_CYCLE * pulse_num; i++) {
        data[i] = sin_wave[i % SAMPLES_PER_CYCLE];
    }
    for(int i = 0; i < SAMPLES_PER_CYCLE * pulse_num; i++) {
        if(i % 2 == 0) {
            int16_t tem = data[i];
            data[i] = data[i + 1];
            data[i + 1] = tem;
        }
    }
    for(int i = 0; i < SAMPLES_PER_CYCLE * pulse_num; i++) 
        printf("%d ", data[i]);
    printf("\n");
    while(1)
    {
        size_t byte_load = 0;
        size_t byte_written = 0;
        while(byte_written < SAMPLES_PER_CYCLE * pulse_num * sizeof(int16_t)) {
            if(i2s_channel_write(SLT.audio_i2s.tx_handle, data_i8 + byte_written,
            SAMPLES_PER_CYCLE * pulse_num * sizeof(int16_t) - byte_written, &byte_load, portMAX_DELAY) == ESP_OK) {
                byte_written += byte_load;
            }
            printf("%d %d\n", data[0], byte_load);
        }
        printf("written\n");
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
    #else
    const uint8_t *wav_ptr = hello_wav_start;

    size_t wav_size = hello_wav_end - hello_wav_start;

    const uint8_t *pcm = wav_ptr + 44;
    size_t pcm_size = wav_size - 44;

    int size_tem = pcm_size;

    // int16_t* sound[6];
    // int index = 0;

    // do {

    //     int size_allocate = size_tem > 50000 ? 50000 : size_tem;

    //     sound[index] = malloc(size_allocate); 
    //     memcpy(sound[index], &wav_ptr[pcm_size - size_tem], size_allocate);

    //     size_tem -= size_allocate;
    //     index++;

    // } while(size_tem > 0);

    // printf("size pcm : %d\n", pcm_size);
    // for(int i = 0;i < index; i++) {

    // }
    // for(int i = 0; i < pcm_size / sizeof(int16_t); i++) {
    //     if(i % 2 == 0) {
    //         int16_t tem = hello_pcm[i];
    //         hello_pcm[i] = hello_pcm[i + 1];
    //         hello_pcm[i + 1] = tem;
    //     }
    // }

    printf("\n");

    // for(int i = 0; i < sizeof(hello_pcm) / sizeof(int16_t); i++) {
    //     if(i % 2 == 0) {
    //         int16_t tem = hello_pcm[i];
    //         hello_pcm[i] = hello_pcm[i + 1];
    //         hello_pcm[i + 1] = tem;
    //     }
    // } 
    // for(int i = 0; i < sizeof(hello_pcm) / sizeof(int16_t); i++) 
    //     printf("%d ", hello_pcm[i]);
    // printf("\n");
    // printf("totbyte : %d ; totelement : %d\n", sizeof(hello_pcm), sizeof(hello_pcm) / sizeof(int16_t));

    uint8_t* temp = malloc(1024);

    while(1)
    {
        size_t offset = 0;

        while (offset < pcm_size)
        {
            size_t remain = pcm_size - offset;

            size_t send =
                remain > sizeof(temp)
                ? sizeof(temp)
                : remain;

            memcpy(temp, pcm + offset, send);

            int16_t *s = (int16_t *)temp;

            size_t sample_count = send / 2;

            for (size_t i = 0; i + 1 < sample_count; i += 2)
            {
                int16_t t = s[i];

                s[i] = s[i + 1];

                s[i + 1] = t;
            }

            size_t written;

            i2s_channel_write(SLT.audio_i2s.tx_handle, temp,
            send, &written, portMAX_DELAY);

            offset += send;
        }        
        // size_t byte_load = 0;
        // size_t byte_written = 0;
        // int8_t* data_i8 = (int8_t*)hello_pcm;
        // while(byte_written < sizeof(hello_pcm)) {
        //     if(i2s_channel_write(SLT.audio_i2s.tx_handle, data_i8 + byte_written,
        //     pcm_size - byte_written, &byte_load, portMAX_DELAY) == ESP_OK) {
        //         byte_written += byte_load;
        //     }
        //     printf("%d\n", byte_load);
        // }     
        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
    #endif

}
#elif defined(I2S_PCM)
void task_audio_i2s(void* param)
{
    ESP_LOGI(TAG, "task_audio_i2s running");

    uint16_t *data = malloc(NUM_SLOT * sizeof(uint16_t));
    uint8_t *data_8bit = (uint8_t*)data;

    for(int j = 0; j < NUM_SLOT; j++) data[j] = j;
    for(int j = 0; j < NUM_SLOT; j++)
        if(j % 2 == 0) {
            uint16_t tem = data[j]; 
            data[j] = data[j + 1]; 
            data[j + 1] = tem;
        }
    for(int i = 0; i < NUM_SLOT; i++) 
        printf("%d ", data[i]);
    printf("\n");
        
    while(1)
    {
        size_t byte_written = 0; 
        size_t byte_loadded = 0;

        while(byte_written < NUM_SLOT * sizeof(uint16_t)) {
            esp_err_t ret = i2s_channel_write(SLT.audio_i2s.tx_handle, 
                data_8bit + byte_written, NUM_SLOT * sizeof(uint16_t) - byte_written,
                 &byte_loadded, portMAX_DELAY);
            if(ret == ESP_OK) {
                byte_written += byte_loadded;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }

}
#endif
