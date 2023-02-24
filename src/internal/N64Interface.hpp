#pragma once

#include "stdint.h"
#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class N64Interface
{
private:
    rmt_channel_handle_t _rx_channel;
    rmt_channel_handle_t _tx_channel;
    rmt_encoder_handle_t _encoder;
    rmt_symbol_word_t *_rx_symbols;
    QueueHandle_t _receive_queue;

    size_t _max_rx_bytes;
    gpio_num_t _data;
    bool _msbFirst;

    static bool n64_rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data);

public:
    N64Interface(uint8_t pinData, size_t max_rx_bytes, bool _msbFirst = true);
    ~N64Interface();

    esp_err_t initialize();
    size_t send_command(uint8_t *command, size_t command_len, uint8_t *receiveBuffer, size_t receiveBufferLen);
};
