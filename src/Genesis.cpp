#include "Genesis.hpp"
#include "Genesis.hpp"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

const static char *TAG = "gamepad_genesis";

GamepadGenesis::GamepadGenesis(uint8_t pinSelect, uint8_t pinData0, uint8_t pinData1, uint8_t pinData2, uint8_t pinData3, uint8_t pinData4, uint8_t pinData5)
{
    _select = static_cast<gpio_num_t>(pinSelect);
    _data0 = static_cast<gpio_num_t>(pinData0);
    _data1 = static_cast<gpio_num_t>(pinData1);
    _data2 = static_cast<gpio_num_t>(pinData2);
    _data3 = static_cast<gpio_num_t>(pinData3);
    _data4 = static_cast<gpio_num_t>(pinData4);
    _data5 = static_cast<gpio_num_t>(pinData5);
}

GamepadGenesis::~GamepadGenesis() { }

void GamepadGenesis::initialize()
{
    gpio_reset_pin(_select);
    gpio_reset_pin(_data0);
    gpio_reset_pin(_data1);
    gpio_reset_pin(_data2);
    gpio_reset_pin(_data3);
    gpio_reset_pin(_data4);
    gpio_reset_pin(_data5);

    gpio_set_direction(_select, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_data0, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data1, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data2, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data3, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data4, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_data5, gpio_mode_t::GPIO_MODE_INPUT);

    gpio_set_pull_mode(_data0, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_data1, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_data2, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_data3, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_data4, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(_data5, gpio_pull_mode_t::GPIO_PULLUP_ONLY);

    gpio_set_level(_select, 1); // Idle HIGH

    ESP_LOGD(TAG, "Initialized: select: %d, 0: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d", _select, _data0, _data1, _data2, _data3, _data4, _data5);
}

bool GamepadGenesis::update()
{
    for(int i = 0; i < 8; i++)
    {
        // Alternate between HIGH and LOW on select.
        // We idle HIGH so the first (and every even) pulse must go LOW.
        gpio_set_level(_select, i % 2 == 1);
        esp_rom_delay_us(6);

        if(i == 1) {
            _report.up    = !gpio_get_level(_data0);
            _report.down  = !gpio_get_level(_data1);
            _report.left  = !gpio_get_level(_data2);
            _report.right = !gpio_get_level(_data3);
            _report.b     = !gpio_get_level(_data4);
            _report.c     = !gpio_get_level(_data5);
        } else if(i == 4) {
            _report.is_six_button = !(gpio_get_level(_data0) || 
                                      gpio_get_level(_data1) || 
                                      gpio_get_level(_data2) || 
                                      gpio_get_level(_data3));

            _report.a     = !gpio_get_level(_data4);
            _report.start = !gpio_get_level(_data5);
        } else if(i == 5) {
            if(_report.is_six_button) {
                _report.z    = !gpio_get_level(_data0);
                _report.y    = !gpio_get_level(_data1);
                _report.x    = !gpio_get_level(_data2);
                _report.mode = !gpio_get_level(_data3);
            } else {
                _report.z    = false;
                _report.y    = false;
                _report.x    = false;
                _report.mode = false;
            }
        }
    }

    return true;
}

bool GamepadGenesis::a()     { return _report.a; }
bool GamepadGenesis::b()     { return _report.b; }
bool GamepadGenesis::c()     { return _report.c; }
bool GamepadGenesis::x()     { return _report.x; }
bool GamepadGenesis::y()     { return _report.y; }
bool GamepadGenesis::z()     { return _report.z; }
bool GamepadGenesis::start() { return _report.start; }
bool GamepadGenesis::mode()  { return _report.mode; }
bool GamepadGenesis::up()    { return _report.up; }
bool GamepadGenesis::down()  { return _report.down; }
bool GamepadGenesis::left()  { return _report.left; }
bool GamepadGenesis::right() { return _report.right; }
bool GamepadGenesis::isSixButton() { return _report.is_six_button; }
