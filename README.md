# OGX-Mini
![Adafruit Feather RP2040 USB Host](images/ada_feather_rp2040_usb.jpg "Adafruit Feather RP2040 USB Host")

Firmware for the RP2040, setup for the [Adafruit Feather USB Host board](https://www.adafruit.com/product/5723), capable of emulating gamepads for
- Original Xbox
- XInput (not Xbox 360)
- Nintendo Switch (must be in dock mode, no rumble yet)

Currently there's no way to switch what device is being emulated on the fly, so the firmware must be compiled specifically for whichever platform you'd like to play on. As long as that's the case, I'll provide compiled .uf2 files for each platform in [Releases](https://github.com/wiredopposite/OGX-Mini/releases).

# Supported devices
## Wired controllers
- Original Xbox Duke and S controllers
- Xbox 360, One, Series, and Elite controllers
- Nintendo Switch Pro controller
- Nintnedo Switch wired controllers (tested with PowerA brand, no rumble)
- Nintendo 64 USB controller (experimental)
- Playstation Classic controller
- Dualshock 3 / Batoh (PS3) controllers
- Dualshock 4 (PS4) controllers
- Dualsense (PS5) controllers (Dualsense Edge should work but it's untested)

## Wireless adapters
- Xbox 360 PC adapter (Microsoft or clones, syncs 1 controller)
- 8Bitdo v1 and v2 Bluetooth adapters
- Generic Nintendo Switch wireless adapters
- Most/all wireless adapters that present as Switch/XInput/Playstation controllers

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