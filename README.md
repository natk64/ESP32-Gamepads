# ESP32-Gamepads

This library allows the ESP32 to interface with many retro gamepads

## Supported Gamepads

- NES
- SNES
- N64
- Gamecube
- Genesis/Mega drive

## Installation

Clone [ESP32-N64-RMT](https://github.com/NicoKleinschmidt/ESP32-N64-RMT) and this Repo into the components directory of your ESP-IDF project

## Basic Usage

```cpp
// Create a new Gamepad.
// Set the pin numbers the gamepad is connected to.
GamepadNES gamepad(17, 19, 21);

// Initialize the Gamepad.
gamepad.initialize();

for(;;)
{
    // Call update to get current values.
    gamepad.update();

    // Print button states.
    ESP_LOGI("NES", 
             "A: %d B: %d, SELECT: %d, START: %d, UP: %d, DOWN: %d, LEFT: %d, RIGHT: %d", 
             gamepad.a(), 
             gamepad.b(),
             gamepad.select(),
             gamepad.start(),
             gamepad.up(),
             gamepad.down(),
             gamepad.left(),
             gamepad.right());

    // Wait 20ms before next update.
    vTaskDelay(20 / portTICK_PERIOD_MS);;
}
```