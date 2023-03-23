#pragma once

#include "stdint.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

enum class ControllerType
{
    Unknown,
    DigitalOnly,
    Dualshock1Digital,
    Dualshock1Analog,
    Dualshock2Digital,
    Dualshock2Analog,
};

struct PSXButtonReport
{
    bool select: 1;
    bool l3: 1;
    bool r3: 1;
    bool start: 1;
    bool up: 1;
    bool right: 1;
    bool down: 1; 
    bool left: 1; 
    bool l2: 1;
    bool r2: 1;
    bool l1: 1;
    bool r1: 1;
    bool triangle: 1;
    bool circle: 1;
    bool cross: 1;
    bool square: 1;
};

struct PSXStickReport
{
    uint8_t x;
    uint8_t y;
};

class GamepadPSX
{
private:
    gpio_num_t _pinSEL;
    gpio_num_t _pinSCK;
    gpio_num_t _pinACK;
    gpio_num_t _pinCMD;
    gpio_num_t _pinDATA;

    spi_device_handle_t _gamepadDeviceHandle;

    uint8_t _controllerId;
    bool _isInConfigMode;

    PSXButtonReport _buttonReport;
    PSXStickReport _rightStick;
    PSXStickReport _leftStick;

    // Will be set from an ISR when the ACK line goes low.
    // Used by send_receive to check if byte was acknowledged.
    volatile bool _acknowledged;

    static void ack_isr(void *arg);

    void update_id_config_mode(uint8_t id);

    /// @brief Sends a command and receives the response
    /// @param sendBuf Buffer with the data to send
    /// @param sendLen Length of the send buffer
    /// @param receiveBuf Buffer to receive response in, can be nullptr if receiveLen is 0
    /// @param receiveLen Length of the receive buffer
    /// @return Number of bytes acknowledged by the controller. Equal to the number of valid bytes written to the receive buffer.
    size_t send_receive(uint8_t *sendBuf, size_t sendLen, uint8_t *receiveBuf, size_t receiveLen);

public:
    GamepadPSX(uint8_t pinSEL, uint8_t pinSCK, uint8_t pinACK, uint8_t pinCMD, uint8_t pinDATA);
    ~GamepadPSX();

    esp_err_t initialize();
    bool update();

    bool set_analog_mode(bool analog);
    bool set_config_mode(bool configMode);
    bool config_mode_supported();

    /// @brief Gamepad buttons
    PSXButtonReport buttons();

    /// @brief Right analog stick position
    /// @note Always 0,0 if not in analog mode.
    PSXStickReport right_stick();
    /// @brief Left analog stick position
    /// @note Always 0,0 if not in analog mode.
    PSXStickReport left_stick();

    uint8_t controller_id();
    ControllerType controller_type();
    bool config_mode();
};
