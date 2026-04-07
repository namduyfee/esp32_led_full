
#include "i2s_driver.h"
#include "gpio_driver.h"

esp_err_t i2s_audio_init(i2s_audio_t* i2s_audio) 
{
    esp_err_t ret;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
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
    
    return ESP_OK;
}
