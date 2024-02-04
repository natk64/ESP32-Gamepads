#include "NES.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

const static char *TAG = "gamepad_nes";

GamepadNES::GamepadNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock)
{
    _report = 0;
    _data = static_cast<gpio_num_t>(pinData);
    _latch = static_cast<gpio_num_t>(pinLatch);
    _clock = static_cast<gpio_num_t>(pinClock);
}

GamepadNES::~GamepadNES() { }

void GamepadNES::initialize()
{
    gpio_reset_pin(_data);
    gpio_reset_pin(_latch);
    gpio_reset_pin(_clock);
    gpio_set_direction(_data, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_latch, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_clock, gpio_mode_t::GPIO_MODE_OUTPUT);
    ESP_LOGD(TAG, "Initialized: data: %d, latch: %d, clock: %d", _data, _latch, _clock);
}

bool GamepadNES::update()
{
    uint8_t currentReport = 0;

    gpio_set_level(_latch, 1);
    esp_rom_delay_us(12);
    gpio_set_level(_latch, 0);

    for(int i = 0; i < 8; i++)
    {
        bool dataLine = gpio_get_level(_data);

        if(!dataLine)
        {
            currentReport |= (1 << i);
        }

        esp_rom_delay_us(6);
        gpio_set_level(_clock, 1);
        esp_rom_delay_us(6);
        gpio_set_level(_clock, 0);
    }

    _report = currentReport;

    return true;
}

bool GamepadNES::a()      { return _report & (1 << 0); }
bool GamepadNES::b()      { return _report & (1 << 1); }
bool GamepadNES::select() { return _report & (1 << 2); }
bool GamepadNES::start()  { return _report & (1 << 3); }
bool GamepadNES::up()     { return _report & (1 << 4); }
bool GamepadNES::down()   { return _report & (1 << 5); }
bool GamepadNES::left()   { return _report & (1 << 6); }
bool GamepadNES::right()  { return _report & (1 << 7); }

uint8_t GamepadNES::raw() { return _report; }
