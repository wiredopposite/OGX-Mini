# OGX-Mini
![Adafruit Feather RP2040 USB Host](images/ada_feather_rp2040_usb.jpg "Adafruit Feather RP2040 USB Host")

Firmware for the RP2040, setup for the [Adafruit Feather USB Host board](https://www.adafruit.com/product/5723) (an be used with the Pi Pico), capable of emulating gamepads for
- Original Xbox
- XInput (not Xbox 360)
- Nintendo Switch (docked)
- Playstation Classic
- Playstation 3

Currently there's no way to switch what device is being emulated on the fly, so the firmware must be compiled specifically for whichever platform you'd like to play on. As long as that's the case, I'll provide compiled .uf2 files for each platform in [Releases](https://github.com/wiredopposite/OGX-Mini/releases).

# Supported devices
## Wired controllers
- Original Xbox Duke and S
- Xbox 360, One, Series, and Elite
- Dualshock 4 (PS4)
- Dualsense (PS5, Dualsense Edge should work but it's untested)
- Nintendo Switch Pro
- Nintnedo Switch wired (tested with PowerA brand, no rumble)
- Nintendo 64 USB (experimental, tested with RetroLink brand)
- Playstation Classic

## Wireless adapters
- Xbox 360 PC adapter (Microsoft or clones, syncs 1 controller)
- 8Bitdo v1 and v2 Bluetooth adapters
- Most wireless adapters that present themselves as Switch/XInput/Playstation controllers should work

Note: There are some third party controllers that use incorrect VID and PIDs, these will not work correctly.

# Adding supported controllers
If your third party controller isn't working, but the original version is listed above, send me the device's VID and PID and I'll add it so it's recognized properly.

# Compiling
You can compile this for the Pi Pico by commenting out this line in CMakeLists.txt
`add_compile_definitions(FEATHER_RP2040)`
That will set the D+ and D- host pins to GPIO 0 and 1. Below that you can uncomment whichever platform (OG Xbox, Xinput, etc.) you'd like to use. 

Here's a diagram of how you'd use the Pico:
![Pi Pico Wiring Diagram](images/pi_pico_diagram.png "Pi Pico Wiring Diagram]")

# Special thanks
Thank you to Ryzee119 and the OpenStickCommunity, without their work this project would not exist.