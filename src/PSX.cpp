#include "PSX.hpp"
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CONFIG_MODE_ID 0xf3

static const char *TAG = "PSX";

GamepadPSX::GamepadPSX(uint8_t pinSEL, uint8_t pinSCK, uint8_t pinACK, uint8_t pinCMD, uint8_t pinDATA)
{
    _pinSEL = static_cast<gpio_num_t>(pinSEL);
    _pinSCK = static_cast<gpio_num_t>(pinSCK);
    _pinACK = static_cast<gpio_num_t>(pinACK);
    _pinCMD = static_cast<gpio_num_t>(pinCMD);
    _pinDATA = static_cast<gpio_num_t>( pinDATA);

    _controllerId = 0;
    _isInConfigMode = false;
    _gamepadDeviceHandle = nullptr;
}

GamepadPSX::~GamepadPSX(){ }

esp_err_t GamepadPSX::initialize()
{
    ESP_LOGI(TAG, "Init start");

    spi_bus_config_t busConfig = {
        .mosi_io_num = _pinCMD,
        .miso_io_num = _pinDATA,
        .sclk_io_num = _pinSCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 128,
        .flags = 0,
        .intr_flags = 0,
    };
    spi_device_interface_config_t devConfig = {
        .mode = 3,
        .clock_speed_hz = 250000,
        .spics_io_num = -1,
        .flags = SPI_DEVICE_BIT_LSBFIRST,
        .queue_size = 7,
    };

    // Initialize SPI bus
    esp_err_t err = spi_bus_initialize(HSPI_HOST, &busConfig, SPI_DMA_CH_AUTO);
    if(err != ESP_OK)
        return err;

    err = spi_bus_add_device(HSPI_HOST, &devConfig, &_gamepadDeviceHandle);
    if (err != ESP_OK)
        return err;
    
    // Attach ACK Interrupt
    err = gpio_install_isr_service(0);
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) 
    {
        ESP_LOGE(TAG, "gpio install isr service failed: %s", esp_err_to_name(err));
        return err;
    }

    gpio_set_direction(_pinACK, gpio_mode_t::GPIO_MODE_INPUT);

    err = gpio_isr_handler_add(_pinACK, ack_isr, this);
    if(err != ESP_OK)
        return err;
    err = gpio_intr_enable(_pinACK);
    if(err != ESP_OK)
        return err;

    gpio_set_intr_type(_pinACK, gpio_int_type_t::GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(_pinACK, gpio_pull_mode_t::GPIO_PULLUP_ONLY);

    gpio_set_direction(_pinSEL, gpio_mode_t::GPIO_MODE_OUTPUT);
    gpio_set_level(_pinSEL, 1);

    return ESP_OK;
}

bool GamepadPSX::update()
{
    uint8_t recBuf[8];
    uint8_t command[] = {0x1, 0x42, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    size_t receivedLen = send_receive(command, sizeof(command), recBuf, sizeof(recBuf));

    if(receivedLen < 4)
        return false;

    update_id_config_mode(recBuf[0]);

    uint8_t *buttonReport = (uint8_t *)&_buttonReport;
    buttonReport[0] = ~recBuf[2];
    buttonReport[1] = ~recBuf[3];
    _buttonReport = *(PSXButtonReport *)buttonReport;

    if(receivedLen < 8) 
    {
        // Digital Mode
        _rightStick.x = 0;
        _rightStick.y = 0;
        _leftStick.x = 0;
        _leftStick.y = 0;
    }
    else
    {
        // Analog Mode
        _rightStick.x = recBuf[4];
        _rightStick.y = recBuf[5];
        _leftStick.x = recBuf[6];
        _leftStick.y = recBuf[7];
    }
    
    return true;
}

bool GamepadPSX::set_analog_mode(bool analog)
{
    uint8_t setAnalogMode[] = {0x01, 0x44, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
    uint8_t recBuf[1];
    size_t receivedLen = send_receive(setAnalogMode, sizeof(setAnalogMode), recBuf, sizeof(recBuf));

    if(receivedLen < 4)
        return false;

    update_id_config_mode(recBuf[0]);
    return true;
}

bool GamepadPSX::set_config_mode(bool configMode)
{
    uint8_t exitConfigMode[] = {0x01, 0x43, 0x00, configMode, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t recBuf[1];
    size_t receivedLen = send_receive(exitConfigMode, sizeof(exitConfigMode), recBuf, sizeof(recBuf));

    if(receivedLen < 4)
        return false;

    update_id_config_mode(recBuf[0]);
    return true;
}

bool GamepadPSX::config_mode_supported()
{
    return false; // TODO:
}

PSXButtonReport GamepadPSX::buttons()
{
    return _buttonReport;
}

PSXStickReport GamepadPSX::right_stick()
{
    return _rightStick;
}

PSXStickReport GamepadPSX::left_stick()
{
    return _leftStick;
}

uint8_t GamepadPSX::controller_id()
{
    return _controllerId;
}

ControllerType GamepadPSX::controller_type()
{
    switch (_controllerId)
    {
    case 0x41:
        return ControllerType::DigitalOnly;
    case 0x73:
        return ControllerType::Dualshock1Analog;
    default:
        return ControllerType::Unknown;
    };
}

bool GamepadPSX::config_mode()
{
    return _isInConfigMode;
}

void GamepadPSX::update_id_config_mode(uint8_t id)
{
    if(id == CONFIG_MODE_ID) {
        _isInConfigMode = true;
    } else {
        _isInConfigMode = false;
        _controllerId = id;
    }
}

void GamepadPSX::ack_isr(void *arg)
{
    GamepadPSX *gamepad = static_cast<GamepadPSX *>(arg);

    gamepad->_acknowledged = true;
}

size_t GamepadPSX::send_receive(uint8_t *sendBuf, size_t sendLen, uint8_t *receiveBuf, size_t receiveLen)
{
    gpio_set_level(_pinSEL, 0);
    esp_rom_delay_us(20);
    size_t receivedBytes = 0;

    for (size_t i = 0; i < sendLen; i++)
    {
        bool isLastByte = i == sendLen  - 1;
        spi_transaction_t transDesc = {
            .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
            .length = 8,
            .rxlength = 8,
            .tx_data = {sendBuf[i], 0x0, 0x0, 0x0},
        };

        _acknowledged = false;
        spi_device_polling_transmit(_gamepadDeviceHandle, &transDesc);

        // We don't receive any data on the first command byte.
        bool inReceiveRange = i > 0 && i-1 < receiveLen;

        if(inReceiveRange)
        {
            receiveBuf[i - 1] = transDesc.rx_data[0];
            receivedBytes++;
        }

        if (isLastByte)
        {
            // Last byte transmitted.
            gpio_set_level(_pinSEL, 1);
        }
        else
        {
            // Wait for ACK, timeout 100µs.
            for(int i = 0; i < 100; i++)
            {
                if(_acknowledged)
                    break;

                esp_rom_delay_us(1);
            }
        }

        if (!isLastByte && !_acknowledged)
        {
            break;
        }
    }
    gpio_set_level(_pinSEL, 1);

    return receivedBytes;
}
