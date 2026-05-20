
#ifndef MAIN
#define MAIN

#include "soc/soc_caps.h"

typedef struct {

    void *data;
    uint32_t tot_byte;

} audio_buf_t;

typedef struct {
    struct {
        void *data;
        uint32_t tot_byte;
    } channel0;

    struct {
        void *data;
        uint32_t tot_byte;
    } channel1;

} request_strip_led_t;

#endif
