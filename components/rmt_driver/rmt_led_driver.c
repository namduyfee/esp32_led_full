
#include "rmt_led_driver.h"

#define RMT_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us
#define RMT_CHANNEL0_GPIO_NUM       2
#define RMT_CHANNEL1_GPIO_NUM       4
static const char *TAG = "RMT_LED_DRIVER";

extern esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder, TypeLed led);

static void rmt_led_init_channel0(rmt_led_t* rmt);
static void rmt_led_init_channel1(rmt_led_t* rmt);

void rmt_led_init(rmt_led_t* rmt)
{
    rmt_led_init_channel0(rmt);
    rmt_led_init_channel1(rmt);
}

static void rmt_led_init_channel0(rmt_led_t* rmt) 
{   
    /**< install encoder channel0 */
    ESP_LOGI(TAG, "Install led encoder channel0");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &rmt->channel0.encoder.handl, LED1903));

    /** config tx channel0 */
    rmt->channel0.handl = NULL;

    rmt_tx_channel_config_t config_channel0 = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_CHANNEL0_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&config_channel0, &rmt->channel0.handl));
    ESP_LOGI(TAG, "Enable RMT TX channel0");
    ESP_ERROR_CHECK(rmt_enable(rmt->channel0.handl));

    /**< transmit config channel0 */
    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t trans_config = {
        .loop_count = 0, // no transfer loop
    };
    rmt->channel0.trans_conf = trans_config;

} 

static void rmt_led_init_channel1(rmt_led_t* rmt) 
{

    /**< install encoder channel1 */
    ESP_LOGI(TAG, "Install led encoder channel1");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &rmt->channel1.encoder.handl, LED1903));

    /** config tx channel1 */
    rmt->channel1.handl = NULL;

    rmt_tx_channel_config_t config_channel1 = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_CHANNEL0_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&config_channel1, &rmt->channel1.handl));
    ESP_LOGI(TAG, "Enable RMT TX channel1");
    ESP_ERROR_CHECK(rmt_enable(rmt->channel1.handl));

    /**< transmit config channel1 */
    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t trans_config = {
        .loop_count = 0, // no transfer loop
    };
    rmt->channel1.trans_conf = trans_config;
}
