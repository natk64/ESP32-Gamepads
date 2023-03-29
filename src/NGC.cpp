#include "NGC.hpp"
#include "esp_log.h"
#include "string.h"

#define RMT_MAX_RX_BYTES 34 // Maximum bytes received in a single read.

GamepadNGC::GamepadNGC(uint8_t pinData)
{
    _data = static_cast<gpio_num_t>(pinData);
    _interface = new N64CommandInterface(_data, RMT_MAX_RX_BYTES);
}

GamepadNGC::~GamepadNGC() 
{
    if(_interface != nullptr)
    {
        delete _interface;
    }
}

bool GamepadNGC::initialize()
{
    esp_err_t err = _interface->initialize();

    return err == ESP_OK;
}

bool GamepadNGC::update()
{
    uint8_t command[] = { 0x40, 0x03, 0x02 };
    size_t bytesReceived = _interface->send_command(command, sizeof(command), _report, sizeof(_report));

    if(bytesReceived != sizeof(_report))
    {
        uint8_t probe[] = { 0x00 };
        uint8_t recBuf[1];

        _interface->send_command(probe, sizeof(probe), recBuf, sizeof(recBuf));
        return false;
    }

    return true;
}

bool GamepadNGC::a()             { return _report[0] & (1 << 0); }
bool GamepadNGC::b()             { return _report[0] & (1 << 1); }
bool GamepadNGC::x()             { return _report[0] & (1 << 2); }
bool GamepadNGC::y()             { return _report[0] & (1 << 3); }
bool GamepadNGC::start()         { return _report[0] & (1 << 4); }

bool GamepadNGC::left()          { return _report[1] & (1 << 0); }
bool GamepadNGC::right()         { return _report[1] & (1 << 1); }
bool GamepadNGC::down()          { return _report[1] & (1 << 2); }
bool GamepadNGC::up()            { return _report[1] & (1 << 3); }
bool GamepadNGC::z()             { return _report[1] & (1 << 4); }
bool GamepadNGC::r_stop()        { return _report[1] & (1 << 5); }
bool GamepadNGC::l_stop()        { return _report[1] & (1 << 6); }

// TODO: The stick values need to be relative to the origin obtained by the 0x41 (Probe origin) command.
int8_t GamepadNGC::stick_x() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[2], sizeof(uint8_t));
    return signedValue - 127;
}

int8_t GamepadNGC::stick_y() 
{ 
    int8_t signedValue;
    memcpy(&signedValue, &_report[3], sizeof(uint8_t));
    return signedValue - 127; 
}

int8_t GamepadNGC::c_stick_x()
{
    int8_t signedValue;
    memcpy(&signedValue, &_report[4], sizeof(uint8_t));
    return signedValue - 127;
}

int8_t GamepadNGC::c_stick_y()
{
    int8_t signedValue;
    memcpy(&signedValue, &_report[5], sizeof(uint8_t));
    return signedValue;
}

uint8_t GamepadNGC::l() { return _report[6]; }
uint8_t GamepadNGC::r() { return _report[7]; }
