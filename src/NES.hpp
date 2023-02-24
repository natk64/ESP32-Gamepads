#pragma once

#include "stdint.h"
#include "driver/gpio.h"

class GamepadNES
{
private:
    uint8_t _report;
    
    gpio_num_t _data;
    gpio_num_t _latch;
    gpio_num_t _clock;

public:
    GamepadNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock);
    ~GamepadNES();

    void initialize();
    bool update();

    bool a();
    bool b();
    bool select();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();

    uint8_t raw();
};
