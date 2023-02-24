#pragma once

#include "stdint.h"
#include "driver/gpio.h"

class GamepadGenesis
{
private:
    struct GenesisReport
    {
        bool up   : 1;
        bool down : 1;
        bool left : 1;
        bool right: 1;
        bool a    : 1;
        bool b    : 1;
        bool c    : 1;
        bool x    : 1;
        bool y    : 1;
        bool z    : 1;
        bool start: 1;
        bool mode : 1;

        bool is_six_button: 1;
    };

    GenesisReport _report;
    
    gpio_num_t _data0;
    gpio_num_t _data1;
    gpio_num_t _data2;
    gpio_num_t _data3;
    gpio_num_t _data4;
    gpio_num_t _data5;
    gpio_num_t _select;

public:
    GamepadGenesis(uint8_t pinSelect, uint8_t pinData0, uint8_t pinData1, uint8_t pinData2, uint8_t pinData3, uint8_t pinData4, uint8_t pinData5);
    ~GamepadGenesis();

    void initialize();
    bool update();

    bool a();
    bool b();
    bool c();
    bool x();
    bool y();
    bool z();
    bool up();
    bool down();
    bool left();
    bool right();
    bool start();
    bool mode();
    bool isSixButton();
};
