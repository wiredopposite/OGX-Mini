# OGX-Mini
![Adafruit Feather RP2040 USB Host](images/ada_feather_rp2040_usb.jpg "Adafruit Feather RP2040 USB Host")

Firmware for the RP2040, setup for the [Adafruit Feather USB Host board](https://www.adafruit.com/product/5723) (can be used with the Pi Pico as well), capable of emulating gamepads for
- Original Xbox
- Playstation 3
- Nintendo Switch (docked)
- XInput (not Xbox 360)
- Playstation Classic

## Supported devices
### Wired controllers
- Original Xbox Duke and S
- Xbox 360, One, Series, and Elite
- Dualshock 4 (PS4)
- Dualsense (PS5, Dualsense Edge should work but it's untested)
- Nintendo Switch Pro
- Nintendo Switch wired (tested with PowerA brand)
- Nintendo 64 USB (experimental, tested with RetroLink brand)
- Playstation Classic

### Wireless adapters
- Xbox 360 PC adapter (Microsoft or clones, syncs 1 controller)
- 8Bitdo v1 and v2 Bluetooth adapters (set to XInput mode)
- Most wireless adapters that present themselves as Switch/XInput/PlayStation controllers should work

Note: There are some third party controllers that can change their VID/PID, these might not work correctly.

## Changing input mode
By default the input mode is set to OG Xbox, you must hold a button combo for 3 seconds to change which platform you want to play on. Your chosen input mode will persist after powering off the device. 

Start = Plus (Switch) = Options (Dualsense/DS4)

### XInput
Start + Dpad Up 
### Original Xbox
Start + Dpad Right
### Switch
Start + Dpad Down
### PlayStation 3
Start + Dpad Left
### PlayStation Classic
Start + A (Cross for PlayStation and B for Switch gamepads)

After a new mode is stored, the RP2040 will reset itself so you don't need to unplug it. 

## Adding supported controllers
If your third party controller isn't working, but the original version is listed above, send me the device's VID and PID and I'll add it so it's recognized properly.

## Compiling
You can compile this for the Pi Pico by commenting out this line in CMakeLists.txt
`add_compile_definitions(FEATHER_RP2040)`
That will set the D+ and D- host pins to GPIO 0 and 1. 

Here's a diagram of how you'd use the Pico:
![Pi Pico Wiring Diagram](images/pi_pico_diagram.png "Pi Pico Wiring Diagram]")

## Special thanks
Thank you to Ryzee119 and the OpenStickCommunity, without their work this project would not exist.