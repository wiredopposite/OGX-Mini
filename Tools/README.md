# Tools

## Controller mapping capture (new gamepad support)

See **[controller_capture/README.md](controller_capture/README.md)** for legacy PC helpers. **None of those scripts’ output is accepted for mapping support** — use on-device Debug UART capture (see [Adding supported controllers](../Firmware/RP2040/docs/Adding_Supported_Controllers.md)).

## Dumping Xbox DVD dongle firmware

The firmware for the DVD Playback Kit is not included here, but you can dump your own or place a `.BIN` dump in this directory. Whichever you do, you'll have to run `dump-xremote-firmware.py` to have it included with the firmware when you compile it.