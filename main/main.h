
#ifndef MAIN
#define MAIN

#include "soc/soc_caps.h"

/** define mode I2S for audio */
//#define I2S_PCM
#ifndef I2S_PCM
#define I2S_PDM
#endif

typedef struct {

    void *data;
    uint32_t tot_byte;

} audio_buf_t;

#endif
