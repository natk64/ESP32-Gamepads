#pragma once

#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
    uint8_t msb_first;
} gc_n64_encoder_config_t;

esp_err_t rmt_new_gc_n64_encoder(const gc_n64_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif