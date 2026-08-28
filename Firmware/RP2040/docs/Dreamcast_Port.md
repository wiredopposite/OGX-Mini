# Dreamcast (Maple Bus) Port from DreamPicoPort

OGX-Mini has **stub** support for Dreamcast: you can select **Start + Y** for Dreamcast mode and **Start + Select** to cycle input source to **Dreamcast GPIO**, but the actual Maple Bus protocol is not yet implemented. This document describes how to complete the port from [DreamPicoPort](https://github.com/OrangeFox86/DreamPicoPort) (Dreamcast to USB gamepad converter for Raspberry Pi Pico).

## What DreamPicoPort Provides

- **Maple Bus**: Sega’s serial protocol for Dreamcast controllers and peripherals. Two wires (A and B), bidirectional, frame-based with CRC.
- **Host mode**: Read a real Dreamcast controller → USB (or in our case, fill `Gamepad::PadIn`).
- **Client mode**: USB gamepad → emulate a Dreamcast controller (we feed from `Gamepad::PadIn` and respond to the console).

DreamPicoPort uses:

- Two PIO programs: `maple_in.pio` (read) and `maple_out.pio` (write), on **different PIO blocks** (each block has a 32-instruction limit).
- DMA for TX/RX, IRQ on completion.
- **Pin A** and **Pin B** = two consecutive GPIOs (e.g. 10 and 11). Optional **direction pin** for a transceiver.
- Packet format: `MaplePacket` (frame word, payload, 8-bit CRC). Commands: device info, GET_CONDITION (controller state), etc.
- Controller condition: 8-byte `controller_condition_t` (buttons, analog L/R, sticks). See `inc/dreamcast_structures.h`.

## Files to Port from DreamPicoPort

1. **Maple Bus HAL**
   - `inc/hal/MapleBus/MapleBusInterface.hpp`
   - `inc/hal/MapleBus/MaplePacket.hpp`
   - `src/hal/MapleBus/MapleBus.hpp`
   - `src/hal/MapleBus/MapleBus.cpp`
   - `src/hal/MapleBus/PioProgram.hpp`
   - `src/hal/MapleBus/maple_in.pio`
   - `src/hal/MapleBus/maple_out.pio`

2. **Dreamcast types**
   - `inc/dreamcast_constants.h` (commands, device functions)
   - `inc/dreamcast_structures.h` (`controller_condition_t`)

3. **Configuration**
   - Provide a small `configuration.h` (or equivalent) with:
     - `MAPLE_OPEN_LINE_CHECK_TIME_US`, `MAPLE_NS_PER_BIT`, `MAPLE_RESPONSE_TIMEOUT_US`, etc.
     - Pins: e.g. `P1_BUS_START_PIN` (pin A), `P1_DIR_PIN` (-1 if no direction pin).
   - DreamPicoPort uses a **forked pico-sdk**; we use the stock SDK, so any SDK-specific code may need minor adjustments.

4. **Client (output to Dreamcast)**
   - Either port `DreamcastController` + `DreamcastMainPeripheral` / `DreamcastPeripheral` (full state machine), or implement a **minimal controller** that:
     - Responds to **device info** and **GET_CONDITION**.
     - Builds `controller_condition_t` from `Gamepad::PadIn` (see `Gamepad::PadIn` ↔ `controller_condition_t` mapping in DreamPicoPort’s `DreamcastController` / host code).
   - Run this loop on **core1** (like PS1/PS2 and GameCube), replacing the stub in `Dreamcast.cpp` / `dreamcast_core1_entry()`.

5. **Host (read Dreamcast controller)**
   - Port the host path that sends GET_CONDITION and reads the response (e.g. hostLib code that uses `MapleBus::write(..., autostartRead=true)` and parses the reply).
   - In `GPIOHost::dreamcast_host_init()` create and configure the Maple Bus (pin A, optional dir pin; use one PIO for in and one for out).
   - In `GPIOHost::dreamcast_host_poll()` send GET_CONDITION, read response, fill `controller_condition_t`, convert to `Gamepad::PadIn`, call `gamepad.set_pad_in()`.

## OGX-Mini Integration Points

- **Device (output)**  
  - `USBDevice/DeviceDriver/Dreamcast/Dreamcast.cpp`  
  - `DreamcastDevice::initialize()`: one-time Maple Bus + minimal controller setup.  
  - `DreamcastDevice::process(idx, gamepad)`: not used for USB; core1 loop reads a shared report (e.g. a `controller_condition_t` or `PadIn` updated from the main loop).  
  - `dreamcast_core1_entry()`: run the Maple Bus **device** loop (wait for console commands, respond with device info / condition from the shared report).

- **Host (input)**  
  - `USBHost/GPIOHost/GPIOHost.cpp`  
  - `dreamcast_host_init(pio_in_index, pio_out_index, pin_a, dir_pin)`: create MapleBus instance(s), configure pins.  
  - `dreamcast_host_poll(gamepad)`: send GET_CONDITION, parse response into `controller_condition_t`, map to `PadIn`, `gamepad.set_pad_in()`.

- **Board**
  - Standard and PicoW already treat `DeviceDriverType::DREAMCAST` as GPIO device mode (no USB device, core1 runs `dreamcast_core1_entry`).
  - When input source is `DREAMCAST_GPIO`, they call `dreamcast_host_init()` and `dreamcast_host_poll()`.

- **Pins**
  - DreamPicoPort defaults: e.g. P1 bus = GPIO 10 (A), 11 (B); direction pin 6. We use `dreamcast_host_init(1, 0, 10, -1)` in the stub (PIO1 for in, PIO0 for out, pin A 10, no dir pin). Adjust in your config to match hardware.

## References

- [DreamPicoPort](https://github.com/OrangeFox86/DreamPicoPort) – source for Maple Bus and Dreamcast controller logic.
- [Maple Bus (Dreamcast wiki)](https://dreamcast.wiki/Maple_bus) – protocol overview.
- DreamPicoPort README: Maple Bus implementation, PIO data handoff, timeouts.
