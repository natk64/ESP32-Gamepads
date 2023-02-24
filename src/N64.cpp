#include "N64.hpp"
#include "esp_log.h"
#include "string.h"

#define RMT_MAX_RX_BYTES 34 // Maximum bytes received in a single read.

GamepadN64::GamepadN64(uint8_t pinData)
{
    _data = static_cast<gpio_num_t>(pinData);
    _interface = new N64Interface(_data, RMT_MAX_RX_BYTES);
}

GamepadN64::~GamepadN64() 
{
    if(_interface != nullptr)
    {
        delete _interface;
    }
}

bool GamepadN64::initialize()
{
    esp_err_t err = _interface->initialize();

    return err == ESP_OK;
}

bool GamepadN64::update()
{
    uint8_t command[] = { 0x01 };
    size_t bytesReceived = _interface->send_command(command, sizeof(command), _report, sizeof(_report));

    return bytesReceived == sizeof(_report);
}

bool GamepadN64::a()      { return _report[0] & (1 << 7); }
bool GamepadN64::b()      { return _report[0] & (1 << 6); }
bool GamepadN64::z()      { return _report[0] & (1 << 5); }
bool GamepadN64::start()  { return _report[0] & (1 << 4); }
bool GamepadN64::up()     { return _report[0] & (1 << 3); }
bool GamepadN64::down()   { return _report[0] & (1 << 2); }
bool GamepadN64::left()   { return _report[0] & (1 << 1); }
bool GamepadN64::right()  { return _report[0] & (1 << 0); }

bool GamepadN64::l()        { return _report[1] & (1 << 5); }
bool GamepadN64::r()        { return _report[1] & (1 << 4); }
bool GamepadN64::c_up()     { return _report[1] & (1 << 3); }
bool GamepadN64::c_down()   { return _report[1] & (1 << 2); }
bool GamepadN64::c_left()   { return _report[1] & (1 << 1); }
bool GamepadN64::c_right()  { return _report[1] & (1 << 0); }

int8_t GamepadN64::stick_x() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[2], sizeof(uint8_t));
    return signedValue; 
}

int8_t GamepadN64::stick_y() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[3], sizeof(uint8_t));
    return signedValue; 
}
