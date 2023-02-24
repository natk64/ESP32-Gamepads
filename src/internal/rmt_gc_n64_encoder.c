#include "rmt_gc_n64_encoder.h"
#include "esp_check.h"

static const char *TAG = "gc_n64_encoder";

typedef struct {
    rmt_encoder_t base;           // the base "class", declares the standard encoder interface
    rmt_encoder_t *copy_encoder;  // use the copy_encoder to encode the stop bit.
    rmt_encoder_t *bytes_encoder; // use the bytes_encoder to encode the address and command data
    rmt_symbol_word_t stop_bit_symbol;  // Stop bit.
    int state;
} rmt_gc_n64_encoder_t;

static size_t rmt_encode_gc_n64(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_gc_n64_encoder_t *gc_n64_encoder = __containerof(encoder, rmt_gc_n64_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;

    uint8_t *data = (uint8_t *)primary_data;

    rmt_encoder_handle_t copy_encoder = gc_n64_encoder->copy_encoder;
    rmt_encoder_handle_t bytes_encoder = gc_n64_encoder->bytes_encoder;
    
    switch (gc_n64_encoder->state) {

    case 0: // send data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            gc_n64_encoder->state = 1; // we can only switch to next state when current encoder finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 1: // send stop bit
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &gc_n64_encoder->stop_bit_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            gc_n64_encoder->state = 0; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_gc_n64_encoder(rmt_encoder_t *encoder)
{
    rmt_gc_n64_encoder_t *gc_n64_encoder = __containerof(encoder, rmt_gc_n64_encoder_t, base);
    rmt_del_encoder(gc_n64_encoder->copy_encoder);
    rmt_del_encoder(gc_n64_encoder->bytes_encoder);
    free(gc_n64_encoder);
    return ESP_OK;
}

static esp_err_t rmt_gc_n64_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_gc_n64_encoder_t *gc_n64_encoder = __containerof(encoder, rmt_gc_n64_encoder_t, base);
    rmt_encoder_reset(gc_n64_encoder->copy_encoder);
    rmt_encoder_reset(gc_n64_encoder->bytes_encoder);
    gc_n64_encoder->state = 0;
    return ESP_OK;
}

esp_err_t rmt_new_gc_n64_encoder(const gc_n64_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_gc_n64_encoder_t *gc_n64_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gc_n64_encoder = calloc(1, sizeof(rmt_gc_n64_encoder_t));
    ESP_GOTO_ON_FALSE(gc_n64_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for ir nec encoder");
    gc_n64_encoder->base.encode = rmt_encode_gc_n64;
    gc_n64_encoder->base.del = rmt_del_gc_n64_encoder;
    gc_n64_encoder->base.reset = rmt_gc_n64_encoder_reset;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &gc_n64_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    // construct the leading code and ending code with RMT symbol format
    gc_n64_encoder->stop_bit_symbol = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = 1 * config->resolution / 1000000, // 1µs
        .level1 = 1,
        .duration1 = 3 * config->resolution / 1000000, // 3µs
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 0,
            .duration0 = 3 * config->resolution / 1000000, // 3µs LOW
            .level1 = 1,
            .duration1 = 1 * config->resolution / 1000000, // 1µs HIGH
        },
        .bit1 = {
            .level0 = 0,
            .duration0 = 1 * config->resolution / 1000000, // 1µs LOW
            .level1 = 1,
            .duration1 = 3 * config->resolution / 1000000, // 3µs HIGH
        },
        .flags = {
            .msb_first = config->msb_first,
        }
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &gc_n64_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");

    *ret_encoder = &gc_n64_encoder->base;
    return ESP_OK;

err:
    if (gc_n64_encoder) {
        if (gc_n64_encoder->bytes_encoder) {
            rmt_del_encoder(gc_n64_encoder->bytes_encoder);
        }
        if (gc_n64_encoder->copy_encoder) {
            rmt_del_encoder(gc_n64_encoder->copy_encoder);
        }
        free(gc_n64_encoder);
    }
    return ret;
}
