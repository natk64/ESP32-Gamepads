#pragma once

#include "stdint.h"
#include "driver/gpio.h"
#include "N64CommandInterface.hpp"

class GamepadNGC
{
private:
    uint8_t _report[8];
    gpio_num_t _data;

    N64CommandInterface *_interface;   

public:
    GamepadNGC(uint8_t pinData);
    ~GamepadNGC();

    bool initialize();
    bool update();

    bool a();
    bool b();
    bool x();
    bool y();
    bool z();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();
    bool l_stop();
    bool r_stop();
    
    int8_t stick_x();
    int8_t stick_y();
    int8_t c_stick_x();
    int8_t c_stick_y();
    uint8_t l();
    uint8_t r();
};
