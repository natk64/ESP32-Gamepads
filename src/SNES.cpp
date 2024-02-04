#include "SNES.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

const static char *TAG = "gamepad_snes";

GamepadSNES::GamepadSNES(uint8_t pinData, uint8_t pinLatch, uint8_t pinClock)
{
    _report = 0;
    _data = static_cast<gpio_num_t>(pinData);
    _latch = static_cast<gpio_num_t>(pinLatch);
    _clock = static_cast<gpio_num_t>(pinClock);
}

GamepadSNES::~GamepadSNES() { }

void GamepadSNES::initialize()
{
    gpio_reset_pin(_data);
    gpio_reset_pin(_latch);
    gpio_reset_pin(_clock);
    gpio_set_direction(_data, gpio_mode_t::GPIO_MODE_INPUT);
    gpio_set_direction(_latch, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_direction(_clock, gpio_mode_t::GPIO_MODE_OUTPUT);
    ESP_LOGD(TAG, "Initialized: data: %d, latch: %d, clock: %d", _data, _latch, _clock);
}

bool GamepadSNES::update()
{
    uint16_t currentReport = 0;

    gpio_set_level(_latch, 1);
    esp_rom_delay_us(12);
    gpio_set_level(_latch, 0);

    for(int i = 0; i < 16; i++)
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

bool GamepadSNES::b()      { return _report & (1 << 0); }
bool GamepadSNES::y()      { return _report & (1 << 1); }
bool GamepadSNES::select() { return _report & (1 << 2); }
bool GamepadSNES::start()  { return _report & (1 << 3); }
bool GamepadSNES::up()     { return _report & (1 << 4); }
bool GamepadSNES::down()   { return _report & (1 << 5); }
bool GamepadSNES::left()   { return _report & (1 << 6); }
bool GamepadSNES::right()  { return _report & (1 << 7); }
bool GamepadSNES::a()      { return _report & (1 << 8); }
bool GamepadSNES::x()      { return _report & (1 << 9); }
bool GamepadSNES::l()      { return _report & (1 << 10); }
bool GamepadSNES::r()      { return _report & (1 << 11); }

uint16_t GamepadSNES::raw() { return _report; }
