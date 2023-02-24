#pragma once

#include "stdint.h"
#include "driver/gpio.h"

class GamepadSNES
{
private:
    uint16_t _report;
    
    gpio_num_t _data;
    gpio_num_t _latch;
    gpio_num_t _clock;

public:
    GamepadSNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock);
    ~GamepadSNES();

    void initialize();
    bool update();

    bool b();
    bool y();
    bool select();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();
    bool a();
    bool x();
    bool l();
    bool r();

    uint16_t raw();
};
