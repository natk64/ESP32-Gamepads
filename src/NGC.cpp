#include "NGC.hpp"
#include "esp_log.h"
#include "string.h"

#define RMT_MAX_RX_BYTES 34 // Maximum bytes received in a single read.

GamepadNGC::GamepadNGC(uint8_t pinData)
{
    _data = static_cast<gpio_num_t>(pinData);
    _interface = new N64CommandInterface(_data, RMT_MAX_RX_BYTES);
    _stick_origin_x = 0;
    _stick_origin_y = 0;
    _c_stick_origin_x = 0;
    _c_stick_origin_y = 0;
    _rumble = 0;
    _connected = false;
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
    if(!_connected)
    {
        uint8_t probe[] = { 0x00 };
        uint8_t recBuf[10];
        size_t bytesReceived = _interface->send_command(probe, sizeof(probe), recBuf, sizeof(recBuf));

        if(bytesReceived != 3) {
            return false;
        }

        uint8_t origin[] = { 0x41 };
        bytesReceived = _interface->send_command(origin, sizeof(origin), recBuf, sizeof(recBuf));
        if(bytesReceived != sizeof(recBuf)) {
            return false;
        }

        _connected = true;
        _stick_origin_x = recBuf[2];
        _stick_origin_y = recBuf[3];
        _c_stick_origin_x = recBuf[4];
        _c_stick_origin_y = recBuf[5];
    }


    uint8_t poll[] = { 0x40, 0x03, _rumble != 0 };
    size_t bytesReceived = _interface->send_command(poll, sizeof(poll), _report, sizeof(_report));

    if(bytesReceived != sizeof(_report)) {
        _connected = false;
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

uint8_t GamepadNGC::rumble() { return _rumble; }
void GamepadNGC::setRumble(uint8_t value) { _rumble = value; }

bool GamepadNGC::reset()
{
    uint8_t reset[] = { 0xFF };
    uint8_t recBuf[3];
    size_t bytesReceived = _interface->send_command(reset, sizeof(reset), recBuf, sizeof(recBuf));
    return bytesReceived == sizeof(recBuf);
}

bool GamepadNGC::calibrate()
{
    uint8_t calibrate[] = { 0x42 };
    uint8_t recBuf[10];
    size_t bytesReceived = _interface->send_command(calibrate, sizeof(calibrate), recBuf, sizeof(recBuf));
    return bytesReceived == sizeof(recBuf);
}

int8_t GamepadNGC::stick_x() 
{
    uint8_t pos = _report[2];
    int16_t diff = pos - _stick_origin_x;
    if(diff < -128)
        return -128;
    if(diff > 127)
        return 127;
    return diff;
}

int8_t GamepadNGC::stick_y() 
{
    uint8_t pos = _report[3];
    int16_t diff = pos - _stick_origin_y;
    if(diff < -128)
        return -128;
    if(diff > 127)
        return 127;
    return diff;
}

int8_t GamepadNGC::c_stick_x()
{
    uint8_t pos = _report[4];
    int16_t diff = pos - _c_stick_origin_x;
    if(diff < -128)
        return -128;
    if(diff > 127)
        return 127;
    return diff;
}

int8_t GamepadNGC::c_stick_y()
{
    uint8_t pos = _report[5];
    int16_t diff = pos - _c_stick_origin_y;
    if(diff < -128)
        return -128;
    if(diff > 127)
        return 127;
    return diff;
}

uint8_t GamepadNGC::l() { return _report[6]; }
uint8_t GamepadNGC::r() { return _report[7]; }
