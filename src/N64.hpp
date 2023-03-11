#pragma once

#include "stdint.h"
#include "driver/gpio.h"
#include "N64CommandInterface.hpp"

class GamepadN64
{
private:
    uint8_t _report[4];
    gpio_num_t _data;

    N64CommandInterface *_interface;   

public:
    GamepadN64(uint8_t pinData);
    ~GamepadN64();

    bool initialize();
    bool update();

    bool a();
    bool b();
    bool z();
    bool start();
    bool up();
    bool down();
    bool left();
    bool right();
    bool l();
    bool r();
    bool c_up();
    bool c_down();
    bool c_left();
    bool c_right();

    int8_t stick_x();
    int8_t stick_y();
};
