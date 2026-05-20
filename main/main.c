
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

#include "esp_clk_tree.h" // Thư viện cây xung nhịp mới

void task_test_eff_led(void *param);
void task_make_led_signal(void *param);
void task_test_audio(void *param);
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
    /** check chip infor and frequency APB */
    uint32_t apb_freq_hz = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &apb_freq_hz);
    printf("APB Clock tu Clock Tree: %lu Hz\n", apb_freq_hz);

    esp_chip_info(&SLT.chip_info);

    printf("Chip model: %d\n", SLT.chip_info.model);
    printf("CPU cores: %d\n",  SLT.chip_info.cores);
    printf("Revision: %d\n",   SLT.chip_info.revision);
    printf("Features: %lx\n",  SLT.chip_info.features);

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
    xTaskCreate(task_test_eff_led, "task_test_eff_led", 1024, NULL, 4, NULL);
    xTaskCreate(task_make_led_signal, "task_make_led_signal", 1024, NULL, 4, NULL);

    xTaskCreate(task_test_audio, "task_test_audio", 1024, NULL, 4, NULL);
    xTaskCreate(task_make_audio_signal, "task_make_audio_signal", 1024, NULL, 4, NULL);
    
}

#define NUM_OF_LED 50
#define NUM_OF_BYTE (NUM_OF_LED * 3)
/**
 * @brief test effect led
 */
void task_test_eff_led(void *param) 
{
    
    uint8_t i = 0; 
    while(1) 
    {
        request_strip_led_t request_tmp = {.channel0 = {.data = NULL, .tot_byte = 0}, .channel1 = {.data = NULL, .tot_byte = 0}};

        uint8_t* channel0_pixels = (uint8_t*)malloc(NUM_OF_BYTE);
        uint8_t* channel1_pixels = (uint8_t*)malloc(NUM_OF_BYTE);
        if(i % 2 == 0) {
            memset(channel0_pixels, 0x00, NUM_OF_BYTE);
            memset(channel1_pixels, 0x00, NUM_OF_BYTE);
        }
        else {
            memset(channel0_pixels, 0x22, NUM_OF_BYTE);
            memset(channel1_pixels, 0x22, NUM_OF_BYTE);
        }

        request_tmp.channel0.data = channel0_pixels; request_tmp.channel0.tot_byte = NUM_OF_BYTE;
        request_tmp.channel1.data = channel1_pixels; request_tmp.channel1.tot_byte = NUM_OF_BYTE;

        xQueueSend(xLedRequestList, &request_tmp, portMAX_DELAY);

        i = (i + 1) % 2; 
        vTaskDelay(pdMS_TO_TICKS(500));   
    }
}

/**
 * @brief   make signal rmt to control led
 * @details
 *  - task receive queue 
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

#define MAX_SAMPLE 1024
#define BYTE_OF_EACH_SAMP 2
#define MAX_BYTE_OF_DATA (MAX_SAMPLE * BYTE_OF_EACH_SAMP)
extern const uint8_t intro_wav_start[] asm("_binary_pp2_wav_start");
extern const uint8_t intro_wav_end[]   asm("_binary_pp2_wav_end");

/**
 * @brief   take audio data and send to task make audio signal
 */
void task_test_audio(void *param)
{
    audio_buf_t buf_tem;

    const uint8_t *wav_ptr = intro_wav_start;
    const uint8_t *pcm = wav_ptr + 44;
    size_t wav_size = intro_wav_end - intro_wav_start;
    size_t pcm_size = wav_size - 44;
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
 * 
 * 
 */
#if defined(I2S_PDM)
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
        //vTaskDelay(pdMS_TO_TICKS(1));
    }

}
#elif defined(I2S_PCM)

#define NUM_SLOT_TEST 50

void task_audio_i2s(void* param)
{
    ESP_LOGI(TAG, "task_audio_i2s run");

    uint16_t *data = malloc(NUM_SLOT_TEST * sizeof(uint16_t));
    uint8_t *data_8bit = (uint8_t*)data;

    for(int j = 0; j < NUM_SLOT_TEST; j++) data[j] = j;
    for(int j = 0; j < NUM_SLOT_TEST; j++)
        if(j % 2 == 0) {
            uint16_t tem = data[j]; 
            data[j] = data[j + 1]; 
            data[j + 1] = tem;
        }
    for(int i = 0; i < NUM_SLOT_TEST; i++) 
        printf("%d ", data[i]);
    printf("\n");
        
    while(1)
    {
        size_t byte_written = 0; 
        size_t byte_loadded = 0;

        while(byte_written < NUM_SLOT_TEST * sizeof(uint16_t)) {
            esp_err_t ret = i2s_channel_write(SLT.audio_i2s.tx_handle, 
                data_8bit + byte_written, NUM_SLOT_TEST * sizeof(uint16_t) - byte_written,
                 &byte_loadded, portMAX_DELAY);
            if(ret == ESP_OK) {
                byte_written += byte_loadded;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }

}
#endif
