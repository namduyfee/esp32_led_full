
#include "i2s_driver.h"
#include "gpio_driver.h"

static const char *TAG = "I2S_AUDIO_DRIVER";

#if defined(I2S_PCM)
esp_err_t i2s_init_pcm_tx(i2s_audio_t* i2s_audio) 
{
    esp_err_t ret;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;     // auto clear Auto clear the legacy data in the DMA buffer

    ret = i2s_new_channel(&chan_cfg, &i2s_audio->tx_handle, NULL); 
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK)   return ESP_FAIL;
    i2s_audio->chan_cfg = chan_cfg;

    i2s_std_config_t std_tx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_PIN,
            .bclk = I2S_BCLK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din  = I2S_DIN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    i2s_audio->std_tx_cfg = std_tx_cfg;
    /* Initialize the channel */
    ret = i2s_channel_init_std_mode(i2s_audio->tx_handle, &std_tx_cfg);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK)   return ESP_FAIL;

    ret = i2s_channel_enable(i2s_audio->tx_handle);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK)   return ESP_FAIL;
    ESP_LOGI(TAG, "PCM Init Succes");
    return ESP_OK;
}

#elif   defined(I2S_PDM)
esp_err_t i2s_init_pdm_tx(i2s_audio_t* i2s_audio)
{
    esp_err_t ret;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    ret = i2s_new_channel(&chan_cfg, &i2s_audio->tx_handle, NULL);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK) return ESP_FAIL;
    i2s_audio->chan_cfg = chan_cfg;

    i2s_pdm_tx_config_t pdm_tx_cfg = {
        .clk_cfg = I2S_PDM_TX_CLK_DAC_DEFAULT_CONFIG(I2S_PDM_TX_FREQ_HZ),
        /* The data bit-width of PDM mode is fixed to 16 */
        .slot_cfg = I2S_PDM_TX_SLOT_PCM_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = I2S_CLK_PIN,
            .dout = I2S_DOUT_PIN,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    //pdm_tx_cfg.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;
    i2s_audio->pdm_tx_cfg = pdm_tx_cfg;

    ret = i2s_channel_init_pdm_tx_mode(i2s_audio->tx_handle, &i2s_audio->pdm_tx_cfg);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK)   return ESP_FAIL;

    ret = i2s_channel_enable(i2s_audio->tx_handle);
    ESP_ERROR_CHECK(ret);
    if(ret != ESP_OK)   return ESP_FAIL;
    ESP_LOGI(TAG, "PDM Init Succes");
    return ESP_OK;
}
#endif
