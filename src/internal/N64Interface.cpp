#include "N64Interface.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "rmt_gc_n64_encoder.h"

#define RMT_RESOLUTION_HZ 10000000 // 1 MHz (1µs) resolution

const static rmt_transmit_config_t n64_rmt_tx_config = {
    .loop_count = 0, // no transfer loop
    .flags = { 
        .eot_level = 1 // bus should be released in IDLE
    },
};

const static rmt_receive_config_t n64_rmt_rx_config = {
    .signal_range_min_ns = 1000000000 / RMT_RESOLUTION_HZ,
    .signal_range_max_ns = 12 * 1000,
};

bool N64Interface::n64_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t task_woken = pdFALSE;
    N64Interface *handle = (N64Interface *)user_data;

    xQueueSendFromISR(handle->_receive_queue, edata, &task_woken);

    return task_woken;
}

static size_t n64_rmt_decode_data(rmt_symbol_word_t *rmt_symbols, size_t symbol_num, uint8_t *decoded_bytes, size_t decoded_bytes_len)
{
    size_t byte_pos = 0, bit_pos = 0;
    for (size_t i = 0; i < symbol_num; i ++) {
        if(byte_pos > decoded_bytes_len - 1) {
            return byte_pos;
        }

        if (rmt_symbols[i].duration0 > 20) { // 0 bit
            decoded_bytes[byte_pos] &= ~(0x80 >> bit_pos); // MSB first
        } else { // 1 bit
            decoded_bytes[byte_pos] |= 0x80 >> bit_pos;
        }

        bit_pos ++;
        if (bit_pos >= 8) {
            bit_pos = 0;
            byte_pos ++;
        }
    }
    return byte_pos;
}

N64Interface::N64Interface(uint8_t pinData, size_t max_rx_bytes, bool msbFirst)
{
    _data = static_cast<gpio_num_t>(pinData);
    _max_rx_bytes = max_rx_bytes;
    _msbFirst = msbFirst;
}

N64Interface::~N64Interface() 
{
    if (_encoder) {
        rmt_del_encoder(_encoder);
    }
    if (_rx_channel) {
        rmt_disable(_rx_channel);
        rmt_del_channel(_rx_channel);
    }
    if (_tx_channel) {
        rmt_disable(_tx_channel);
        rmt_del_channel(_tx_channel);
    }
}

esp_err_t N64Interface::initialize()
{
    gc_n64_encoder_config_t encoder_config = {
        .resolution = RMT_RESOLUTION_HZ,
        .msb_first = _msbFirst,
    };
    esp_err_t err = rmt_new_gc_n64_encoder(&encoder_config, &_encoder);
    if (err != ESP_OK)
        return err;

    rmt_rx_channel_config_t rx_channel_config = {
        .gpio_num = _data,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ, 
        .mem_block_symbols = _max_rx_bytes * 8, // Maximum bits to receive. (Read command returns 32 bytes + 1 stop bit.)
        .flags = {
            .invert_in = false,
            .with_dma = false,
            .io_loop_back = false,
        },
    };
    
    err = rmt_new_rx_channel(&rx_channel_config, &_rx_channel);
    if (err != ESP_OK)
        return err;

    rmt_tx_channel_config_t tx_channel_config = {
        .gpio_num = _data,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags = {
            .invert_out = false,
            .with_dma = false,
            .io_loop_back = true,
            .io_od_mode = true,
        },
    };
    
    err = rmt_new_tx_channel(&tx_channel_config, &_tx_channel);
    if (err != ESP_OK)
        return err;

    _rx_symbols = new rmt_symbol_word_t[_max_rx_bytes * 8];

    _receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (!_receive_queue)
        return ESP_ERR_NO_MEM;

    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = n64_rmt_rx_done_callback,
    };
    err = rmt_rx_register_event_callbacks(_rx_channel, &cbs, this);
    if (err != ESP_OK)
        return err;

    rmt_enable(_tx_channel);
    rmt_enable(_rx_channel);

    return ESP_OK;
}

size_t N64Interface::send_command(uint8_t *command, size_t command_len, uint8_t *receiveBuffer, size_t receiveBufferLen)
{
    if(receiveBufferLen > _max_rx_bytes)
        return 0;

    size_t bitsToReceive = (receiveBufferLen + command_len) * 8 + 2; // Data we send + data we expect + stop bits for send and receive.

    esp_err_t err = rmt_receive(_rx_channel, _rx_symbols, bitsToReceive * sizeof(rmt_symbol_word_t), &n64_rmt_rx_config);
    if (err != ESP_OK) {
        ESP_LOGE("N64Interface", "Receive failed: %d", err);
        return 0;
    }
    err = rmt_transmit(_tx_channel, _encoder, command, command_len, &n64_rmt_tx_config);
    if (err != ESP_OK) {
        ESP_LOGE("N64Interface", "Transmit failed: %d", err);
        return 0;
    }

    rmt_rx_done_event_data_t rmt_rx_evt_data;
    if (xQueueReceive(_receive_queue, &rmt_rx_evt_data, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE("N64Interface", "Timeout");
        return 0;
    }

    if(rmt_rx_evt_data.num_symbols < command_len * 8 + 2)
    {
        ESP_LOGE("N64Interface", "Only received %d symbols. Min: %d", rmt_rx_evt_data.num_symbols, command_len * 8 + 2);
        return 0;
    }

    size_t receivedDataLenBits = rmt_rx_evt_data.num_symbols - command_len * 8 - 2;
    return n64_rmt_decode_data(rmt_rx_evt_data.received_symbols + command_len * 8 + 1, receivedDataLenBits, receiveBuffer, receiveBufferLen);
}
