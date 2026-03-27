
#ifndef RMT_LED_DRIVER
#define RMT_LED_DRIVER


#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/rmt_common.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "driver/rmt_encoder.h"

typedef enum {
    LED1903 = 0,
} TypeLed;

typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;

typedef struct {

    rmt_encoder_handle_t handl;

} rmt_encoder_mana_t;

typedef struct {
    
    rmt_channel_handle_t handl;
    rmt_encoder_mana_t encoder;
    TypeLed cur_type;
    rmt_transmit_config_t trans_conf;
    
} rmt_channel_mana_t;

typedef struct {

    rmt_channel_mana_t channel0;
    rmt_channel_mana_t channel1;

} rmt_led_t;

void rmt_led_init(rmt_led_t* rmt);


#endif
