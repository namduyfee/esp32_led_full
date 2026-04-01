#include "dac_audio_driver.h"

static const char *TAG = "DAC_AUDIO";

static bool dac_on_convert_done_callback(dac_continuous_handle_t handle, const dac_event_data_t *event, void *user_data);

esp_err_t dac_audio_init(dac_audio_t* dac_audio)
{
    ESP_LOGI(TAG, "Start Init dma asynch");
    ESP_LOGI(TAG, "--------------------------------------");

    esp_err_t ret = ESP_OK;

    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_ALL,
        .desc_num = DAC_NUM_OF_DESC,
        .buf_size = 2048,
        .freq_hz = DAC_AUDIO_SAMPLE_RATE,
        .offset = 0,
        .clk_src = DAC_DIGI_CLK_SRC_APLL,   // Using APLL as clock source to get a wider frequency range
        .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };

    /* Allocate continuous channels */
    ret = dac_continuous_new_channels(&cont_cfg, &dac_audio->continuous_handle);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK) return ESP_FAIL;

    /* Create a queue to transport the interrupt event data */
    dac_audio->even_data_q = xQueueCreate(20, sizeof(dac_event_data_t));
    if (dac_audio->even_data_q == NULL) return ESP_FAIL;

    dac_event_callbacks_t cbs = {
        .on_convert_done = dac_on_convert_done_callback,
        .on_stop = NULL,
    };
    /* Must register the callback if using asynchronous writing */
    ret = dac_continuous_register_event_callback(dac_audio->continuous_handle, &cbs, dac_audio->even_data_q);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK) return ESP_FAIL;

    /* Enable the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(dac_audio->continuous_handle));
    ESP_LOGI(TAG, "DAC initialized success, DAC DMA is ready");
    return ESP_OK;
}

IRAM_ATTR
static bool dac_on_convert_done_callback(dac_continuous_handle_t handle, const dac_event_data_t *event, void *user_data)
{
    QueueHandle_t que = (QueueHandle_t)user_data;
    BaseType_t need_awoke;
    /* When the queue is full, drop the oldest item */
    if (xQueueIsQueueFullFromISR(que)) {
        dac_event_data_t dummy;
        xQueueReceiveFromISR(que, &dummy, &need_awoke);
    }
    /* Send the event from callback */
    xQueueSendFromISR(que, event, &need_awoke);
    return need_awoke;
}
