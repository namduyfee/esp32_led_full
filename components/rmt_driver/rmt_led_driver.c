#include "rmt_led_driver.h"

static const char *TAG = "RMT_LED_DRIVER";

extern esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder, TypeLed led);

static esp_err_t rmt_led_init_channel0(rmt_led_t* rmt);
static esp_err_t rmt_led_init_channel1(rmt_led_t* rmt);

esp_err_t rmt_led_init(rmt_led_t* rmt, bool sync_support)
{

    if( rmt_led_init_channel0(rmt) != ESP_OK) return ESP_FAIL;
    if( rmt_led_init_channel1(rmt) != ESP_OK) return ESP_FAIL;

    if(sync_support == true) {
        // install sync manager
        rmt->sync.channel_list[0] = rmt->channel0.handl;
        rmt->sync.channel_list[1] = rmt->channel1.handl;
        
        rmt_sync_manager_config_t synchro_config = {
            .tx_channel_array = rmt->sync.channel_list,
            .array_size = sizeof(rmt->sync.channel_list) / sizeof(rmt->sync.channel_list[0]),
        };
        esp_err_t ret = rmt_new_sync_manager(&synchro_config, &rmt->sync.handl);
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;
    }
   
    return ESP_OK;
}

static esp_err_t rmt_led_init_channel0(rmt_led_t* rmt) 
{   
    /**< install encoder channel0 */
    {
        ESP_LOGI(TAG, "Install led encoder channel0");
        led_strip_encoder_config_t encoder_config = {
            .resolution = RMT_RESOLUTION_HZ,
        };

        esp_err_t ret = rmt_new_led_strip_encoder(&encoder_config, &rmt->channel0.encoder.handl, LED1903);
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;
    }
    
    /** config tx channel0 */
    {
        rmt->channel0.handl = NULL;

        rmt_tx_channel_config_t config_channel0 = {
            .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
            .gpio_num = RMT_CHANNEL0_GPIO_NUM,
            .mem_block_symbols = 64, // increase the block size can make the LED less flickering
            .resolution_hz = RMT_RESOLUTION_HZ,
            .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        };
        esp_err_t ret = rmt_new_tx_channel(&config_channel0, &rmt->channel0.handl);    
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;

        ESP_LOGI(TAG, "Enable RMT TX channel0");
        ret = rmt_enable(rmt->channel0.handl);
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;
    }

    /**< transmit config channel0 */
    {
        ESP_LOGI(TAG, "Start LED rainbow chase channel0");
        rmt_transmit_config_t trans_config = {
            .loop_count = 0, // no transfer loop
        };
        rmt->channel0.trans_conf = trans_config;
    }

    return ESP_OK;
}

static esp_err_t rmt_led_init_channel1(rmt_led_t* rmt) 
{
    /**< install encoder channel1 */
    {
        ESP_LOGI(TAG, "Install led encoder channel1");
        led_strip_encoder_config_t encoder_config = {
            .resolution = RMT_RESOLUTION_HZ,
        };

        esp_err_t ret = rmt_new_led_strip_encoder(&encoder_config, &rmt->channel1.encoder.handl, LED1903);
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;
    }
    
    /** config tx channel1 */
    {
        rmt->channel1.handl = NULL;

        rmt_tx_channel_config_t config_channel1 = {
            .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
            .gpio_num = RMT_CHANNEL1_GPIO_NUM,
            .mem_block_symbols = 64, // increase the block size can make the LED less flickering
            .resolution_hz = RMT_RESOLUTION_HZ,
            .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        };
        esp_err_t ret = rmt_new_tx_channel(&config_channel1, &rmt->channel1.handl);    
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;

        ESP_LOGI(TAG, "Enable RMT TX channel1");
        ret = rmt_enable(rmt->channel1.handl);
        ESP_ERROR_CHECK(ret);
        if(ret != ESP_OK) return ESP_FAIL;
    }

    /**< transmit config channel1 */
    {
        ESP_LOGI(TAG, "Start LED rainbow chase channel1");
        rmt_transmit_config_t trans_config = {
            .loop_count = 0, // no transfer loop
        };
        rmt->channel1.trans_conf = trans_config;
    }

    return ESP_OK;
}