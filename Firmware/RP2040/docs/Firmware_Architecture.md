# Firmware architecture & developer overview

This document explains **how OGX-Mini firmware is structured**, **how data flows from input controllers to output consoles**, which **external modules** the project depends on, and **every file you must touch** when adding a new USB host (input) or USB device (output) driver.

**Related**

| Document | Use when |
|----------|----------|
| [Building_From_Source.md](Building_From_Source.md) | Clone, tools, compile, flash |
| [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) | Capture HID reports, map buttons, Debug UART |
| [Controller_Mappings.md](Controller_Mappings.md) | PadIn ↔ output mode tables |
| [Wired_Controllers.md](Wired_Controllers.md) | Supported USB input VID/PID lists |

Paths below are relative to **`Firmware/RP2040/`** unless noted.

---

## Mental model (start here)

OGX-Mini is a **gamepad adapter**:

1. **Input** arrives from a real controller (USB host, Bluetooth, or GPIO).
2. Firmware normalizes that into one internal layout: **`Gamepad::PadIn`** (Xbox-style names).
3. The selected **output mode** reads `PadIn` and presents a console/PC device (USB gadget, GPIO protocol, or Wiimote BT).

```text
┌────────────── INPUT ──────────────┐      Gamepad::PadIn      ┌──────────── OUTPUT ────────────┐
│ USB host (TinyUSB + PIO-USB)      │ ─────────────────────► │ USB device (TinyUSB)           │
│   HostManager → HostDriver        │                        │   DeviceManager → DeviceDriver │
│ Bluetooth (Bluepad32 / BTstack)   │ ─────────────────────► │ GPIO (PS1/2, GC, N64, Maple)   │
│ GPIOHost (retro plugs)            │ ─────────────────────► │ Wii Wiimote BT / WebApp BLE    │
└───────────────────────────────────┘                        └────────────────────────────────┘
                    UserSettings (flash) selects DeviceDriverType / profiles
```

If you only remember one rule when extending the firmware: **always map into `PadIn` on input; never special-case every output mode inside a host driver.**

---

## Repository layout (what lives where)

### Repo root (high level)

| Path | Role |
|------|------|
| `README.md` | User overview, support policy, platforms, short build notes |
| `CHANGELOG.md` | Release notes |
| `Firmware/` | All firmware + external deps |
| `Hardware/` | PCB / wiring diagrams |
| `scripts/` | `build.sh` / `build.ps1` interactive builds |
| `Tools/controller_capture/` | Legacy PC capture scripts — **not accepted** for mapping submissions |
| `docs/` | Research / planning surveys |
| `WebApp/` | Web configuration UI (submodule) |

### `Firmware/`

| Path | Role |
|------|------|
| `FWDefines.cmake` | Firmware product name / version string |
| `cmake/` | Shared CMake: submodule init, lib patches, GATT header generation |
| `external/pico-sdk` | Raspberry Pi Pico SDK (**gitignored** — clone separately; see build guide) |
| `external/tinyusb` | USB device + host stack |
| `external/Pico-PIO-USB` | Software USB host over PIO (second USB port on many boards) |
| `external/bluepad32` | Bluetooth gamepad stack (+ nested **BTstack**) |
| `external/libxsm3` | Xbox 360 XSM3 authentication crypto (XInput **output**) |
| `external/libfixmath` | Fixed-point math for stick curves / deadzones |
| `external/patches/` | Diffs applied at CMake configure (Bluepad32 / BTstack / SDK) |
| `ESP32/` / `ESP32_Blueretro/` | Optional companion ESP32 firmwares (I2C hybrids) |
| `RP2040/` | **Main** RP2040/RP2350 firmware |

### `Firmware/RP2040/src/` (application code)

| Path | Role |
|------|------|
| `main.cpp` | Entry: UART/stdio, flash-safe init → `OGXMini::initialize()` / `run()` |
| `OGXMini/` | Board dispatch: picks Standard / PicoW / I2C board runners from `OGXM_BOARD` |
| `Board/` | Pin config (`Config.h`), LED/RGB, BT/USB board helpers, `ogxm_log.h` |
| `USBHost/` | USB **input**: TinyUSB callbacks, `HostManager`, `HardwareIDs`, per-protocol `HostDriver/` |
| `USBDevice/` | USB **output**: TinyUSB callbacks, `DeviceManager`, per-mode `DeviceDriver/` |
| `Descriptors/` | Packed report structs / bit masks / descriptor bytes shared by host or device |
| `Gamepad/` | `Gamepad`, **`PadIn` / `PadOut`**, stick/trigger scaling, profiles |
| `UserSettings/` | Flash NVS: output mode, profiles, input source, button combos |
| `Bluepad32/` | OGX platform glue: Bluepad32 events → `PadIn` |
| `BLEServer/` | Web-app BLE GATT |
| `TaskQueue/` | Deferred work between cores |
| `Wii/` | Wiimote Bluetooth emulator (Wii **output** mode) |
| `tusb_config.h` / `btstack_config.h` | TinyUSB / BTstack compile-time config |

---

## Boot and runtime flow

```text
main()
  stdio / UART init
  flash_safe_execute_core_init()
  OGXMini::initialize()   // board table keyed by OGXM_BOARD
  OGXMini::run()          // never returns
```

`OGXMini.cpp` selects a board implementation, for example:

| Board family | Implementation | Typical split |
|--------------|----------------|---------------|
| Pico, Zero, Feather, RP2350-USB-A, … | `OGXMini/Board/Standard.cpp` | **Core1:** USB host (`tuh_task` + PIO frame). **Core0:** USB device (`tud_task`) + `DeviceDriver::process` |
| Pico W / Pico 2 W / RP2354 | `OGXMini/Board/PicoW.cpp` | **Core1:** Bluetooth (Bluepad32) + BLE server. **Core0:** USB device + PIO USB host mux vs BT |
| ESP32 hybrids / 4CH I2C | Dedicated board files | RP2040 talks to ESP32 (or multi-channel) over I2C |

### Standard board (USB in → USB out)

1. Init board, flash settings, load profiles, create `DeviceManager` driver for saved mode.
2. **Core1** starts TinyUSB **host** on PIO USB; polls `tuh_task` / feedback.
3. When a pad mounts, `OGXMini::host_mounted(true)` lets **Core0** start TinyUSB **device**.
4. Core0 loop: `tud_task()` → `device_driver->process(i, gamepad)` (reads `PadIn`, sends reports).

### Pico W (BT and/or USB in → USB or GPIO out)

- Bluetooth pads update `PadIn` on Core1 via Bluepad32.
- Wired USB on the PIO host port can take over and disconnect BT (mux logic in PicoW board code).
- **Wii mode** is special: Core1 runs USB host; Core0 runs the Wiimote BT emulator.

### GPIO console modes (PS1/PS2, GameCube, Dreamcast, N64)

Still selected as a `DeviceDriverType`, but output is **bitbanged / PIO** to a controller cable — not a normal USB gadget. USB host or BT still fills `PadIn`; a Core1 (or Core0) protocol loop talks to the console.

---

## Canonical state: `Gamepad::PadIn`

Defined in `src/Gamepad/Gamepad.h`. Xbox-style fields:

| Field | Meaning |
|-------|---------|
| `dpad`, `buttons` | Digital controls (`BUTTON_A/B/X/Y`, bumpers, Start/Back, SYS, MISC, L3/R3) |
| `trigger_l` / `trigger_r` | Analog triggers 0–255 |
| `joystick_lx/ly`, `joystick_rx/ry` | Sticks (centered at 0) |
| `analog[]` | Optional pressure / face analog |
| `accel` / `gyro` | Motion when available |
| Touch fields | DualSense / STEAM touchpad path |

**Always** set buttons with `gamepad.MAP_*` (honors web-app profiles), and use `scale_joystick_*` / `scale_trigger_*` for axes.

`PadOut` carries rumble (and related feedback) back toward host drivers via `send_feedback`.

Full mapping tables: [Controller_Mappings.md](Controller_Mappings.md).

---

## USB host input (wired controllers)

### Flow

```text
TinyUSB host (often on Pico-PIO-USB HCD)
        │
        ▼
tuh_hid_mount_cb / tuh_xinput::mount_cb     [USBHost/tuh_callbacks.cpp]
        │
        ▼
HostManager::get_type(vid,pid)  or  get_type(XInput DevType)
        │
        ▼
HostManager::setup_driver(...)              [USBHost/HostManager.h]
        │  switch(HostDriverType) → make_unique<*Host>
        │  HostDriver::initialize(...)
        ▼
tuh_*_report_received_cb
        │
        ▼
HostManager::process_report → HostDriver::process_report
        │
        ▼
parse bytes → Gamepad::PadIn → gamepad.set_pad_in()
```

### Key pieces

| Piece | Path | Job |
|-------|------|-----|
| TinyUSB callbacks | `USBHost/tuh_callbacks.cpp` | Mount / unmount / report → `HostManager` |
| VID/PID map | `USBHost/HardwareIDs.h` | Arrays + `HOST_TYPE_MAP` → `HostDriverType` |
| Factory / dispatch | `USBHost/HostManager.h` | Creates drivers, routes reports, rumble feedback |
| Drivers | `USBHost/HostDriver/*` | Protocol parsers |
| Generic HID | `HostDriver/HIDGeneric` + `HIDParser/` | Fallback when VID unknown but report desc looks like a joystick |
| XInput class | `HostDriver/XInput/tuh_xinput/` | Xbox 360/One/OG class driver (not VID lists) |

`HostDriverType` enum: `USBHost/HostDriver/HostDriverTypes.h`.

Base class (`HostDriver.h`): implement `initialize`, `process_report`, `send_feedback` (and optional wireless connect/disconnect callbacks).

---

## USB device output (what the console/PC sees)

### Flow

```text
UserSettings::get_current_driver() → DeviceDriverType
        │
        ▼
DeviceManager::initialize_driver(...)       [USBDevice/DeviceManager.cpp]
        │  switch → make_unique<*Device>
        │  DeviceDriver::initialize()
        ▼
tud_init → TinyUSB device
        │
tud_callbacks.cpp → DeviceDriver descriptors / HID / vendor reports
        │
Core0: device_driver->process(idx, gamepad)
        │
        ▼
get_pad_in() → fill USB (or GPIO) report → tud_* / PIO
```

`DeviceDriverType` enum: `USBDevice/DeviceDriver/DeviceDriverTypes.h`  
(`XINPUT`, `PS3`, `SWITCH`, `STEAM`, `PS4`, `XBOXOG`, GPIO modes, `WEBAPP`, …).

Custom TinyUSB classes (examples): `tud_xinput/` (Xbox 360), `tud_xid/` (OG Xbox).

---

## Bluetooth input (Pico W / Pico 2 W / RP2354)

```text
Core1: bluepad32::run_task(gamepads)     [Bluepad32/Bluepad32.cpp]
        │
        ▼
uni_platform callbacks (controller_data_cb)
        │
        ▼
uni_gamepad_t → map to PadIn
        │
        ▼
gamepad->set_pad_in_from_bluetooth(gp_in)
```

- Stack: **Bluepad32** + **BTstack** under `Firmware/external/bluepad32`.
- New BT brands usually need work **inside Bluepad32** (or a custom BLE path), then mapping tweaks in `Bluepad32.cpp`.
- `HardwareIDs.h` does **not** select Bluetooth parsers.

---

## Settings, modes, and flash

| Topic | Where |
|-------|--------|
| Flash NVS | `UserSettings/` (`UserSettings.cpp`, profiles, NVS helpers) |
| Current output mode | Stored `DeviceDriverType`; restored on boot |
| Button combos (~3 s) | `UserSettings.cpp` — `BUTTON_COMBO_MAP` / `VALID_DRIVER_TYPES` |
| Web app profiles | Remap `MAP_*` fields on `Gamepad` |
| Fixed builds | `-DOGXM_FIXED_DRIVER=…` (optional combos via CMake) |
| Flash writes | Prefer Core0; Pico W Core1 uses `flash_safe_execute` when needed |

Changing mode typically: detect combo → store type → disconnect → reboot.

---

## External modules (what makes the project work)

| Module | Path | Role in OGX |
|--------|------|-------------|
| **Pico SDK** | `Firmware/external/pico-sdk` | Multicore, GPIO, flash, clocks, CYW43 Wi‑Fi/BT radio API, PIO codegen |
| **TinyUSB** | `Firmware/external/tinyusb` | USB **device** gadget + USB **host** HID/class drivers |
| **Pico-PIO-USB** | `Firmware/external/Pico-PIO-USB` | Extra USB host port via PIO (Feather / Pico W host pins, etc.) |
| **Bluepad32** | `Firmware/external/bluepad32` | Wireless gamepad discovery/parsing |
| **BTstack** | `bluepad32/external/btstack` | Classic BT + BLE radio stack |
| **libxsm3** | `Firmware/external/libxsm3` | Retail Xbox 360 controller auth in **XInput output** |
| **libfixmath** | `Firmware/external/libfixmath` | Stick deadzone / response curves |

CMake (`Firmware/cmake/patch_libs.cmake`) applies project patches to Bluepad32/BTstack/SDK at configure time.

---

## Adding a new USB **host** (input) driver

Use this when the pad’s report layout is **not** covered by an existing driver (DInput, PS4, Switch, …). For VID/PID-only additions to an existing list, see the shorter path below.

### Checklist — files you must update

| # | File | What to do |
|---|------|------------|
| 1 | **`src/Descriptors/MyPad.h`** (new) | Packed `InReport`, button bit masks, stick midpoints, report IDs |
| 2 | **`src/USBHost/HostDriver/MyPad/MyPad.h`** (new) | Class inheriting `HostDriver` |
| 3 | **`src/USBHost/HostDriver/MyPad/MyPad.cpp`** (new) | `initialize` → start HID receive; `process_report` → fill `PadIn`; `send_feedback` → rumble/LEDs or no-op |
| 4 | **`src/USBHost/HostDriver/HostDriverTypes.h`** | Add `HostDriverType::MY_PAD` |
| 5 | **`src/USBHost/HardwareIDs.h`** | Add `MY_PAD_IDS[]` with `{vid,pid}` entries **and** a row in `HOST_TYPE_MAP` |
| 6 | **`src/USBHost/HostManager.h`** | `#include "…/MyPad.h"`; add `case HostDriverType::MY_PAD:` → `std::make_unique<MyPadHost>(gp_idx)`; set stick Y polarity if needed |
| 7 | **`CMakeLists.txt`** | Add `MyPad.cpp` under the `EN_USB_HOST` host-driver source list (near other `USBHost/HostDriver/…` entries) |
| 8 | **`docs/Wired_Controllers.md`** | Document controller name, VID/PID, mode notes |
| 9 | **`docs/Controller_Mappings.md`** | If the layout is non-standard |
| 10 | **`CHANGELOG.md`** (repo root, optional) | Note the addition for releases |

### Usually **do not** need changes

| File | Why |
|------|-----|
| `USBHost/tuh_callbacks.cpp` | Already calls `get_type` + `setup_driver` for all HID mounts |
| `DeviceManager` / output drivers | Host maps to `PadIn`; outputs already consume `PadIn` |
| `UserSettings` | Output mode combos are unrelated to input VID lists |

### Exceptions

- **New XInput-class device type:** also extend `tuh_xinput` (`DevType` detect + `HostManager::get_type(DevType)`), not only `HardwareIDs.h`.
- **Init / feature reports required before input:** put that in `initialize` / early `process_report` (see Switch Pro / Switch 2 hosts).

### VID/PID only (existing driver)

If the report already matches DInput / PS4 / N64 / etc.:

1. Add `{0xVID, 0xPID}, // Name` to the correct array in `HardwareIDs.h`.
2. Update `Wired_Controllers.md`.
3. Rebuild and test — **no** new enum, CMake, or `HostManager` case.

### Mapping rules (host)

```cpp
Gamepad::PadIn gp_in{};
if (report_bits & MyPad::A) gp_in.buttons |= gamepad.MAP_BUTTON_A;
// …
std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
    gamepad.scale_joystick_l(raw_lx, raw_ly);
gamepad.set_pad_in(gp_in);
tuh_hid_receive_report(address, instance);  // required every time
```

Templates: `HostDriver/N64`, `HostDriver/PSClassic`. Complex examples: `DInput`, `PS4`, `SwitchPro`.

Detailed capture / UART workflow: [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md).

---

## Adding a new USB **device** (output) driver

Different checklist — this is a new **console/PC identity**, not a new input pad.

| # | File | What to do |
|---|------|------------|
| 1 | **`src/Descriptors/MyOut.h`** (new) | Device / config / HID (or vendor) descriptor bytes + report structs |
| 2 | **`src/USBDevice/DeviceDriver/MyOut/MyOut.h/.cpp`** (new) | Subclass `DeviceDriver`: `initialize`, `process`, descriptor callbacks |
| 3 | **`src/USBDevice/DeviceDriver/DeviceDriverTypes.h`** | Add `DeviceDriverType::MY_OUT` |
| 4 | **`src/USBDevice/DeviceManager.cpp`** | `#include` + `case` → `std::make_unique<MyOutDevice>()` |
| 5 | **`CMakeLists.txt`** | Add sources to the always-built `SOURCES_BOARD` device-driver list |
| 6 | **`src/UserSettings/UserSettings.cpp`** | Add to `VALID_DRIVER_TYPES[]`; add combo in `BUTTON_COMBO_MAP` (unless fixed-build-only) |
| 7 | **Custom TinyUSB class** (if needed) | New `tud_myout/` similar to `tud_xinput` / `tud_xid`, wired from your driver |
| 8 | **`tud_callbacks.cpp`** | Only if the mode must opt out of class drivers (GPIO modes do this) |
| 9 | **Board runners** | GPIO outputs may need Core1 entry / `gpio_device_mode` branches in `Standard.cpp` / `PicoW.cpp` |
| 10 | **Docs** | Feature guide, pinouts, `Controller_Mappings.md` PadIn → output table |

`process()` should read `gamepad.get_pad_in()` and produce the host-visible report.

---

## Adding Bluetooth support for a new pad

1. Prefer upstream / forked **Bluepad32** parser support under `Firmware/external/bluepad32`.
2. Adjust OGX conversion in `src/Bluepad32/Bluepad32.cpp` if buttons/axes need remapping into `PadIn`.
3. Test with a **Debug** Pico W / 2 W build and UART (`[BP32 …]` lines).
4. Document in `Wired_Controllers.md` / Bluetooth sections of the README or IMPROVEMENTS as appropriate.

Custom BLE protocols (Switch 2, Steam Triton) are larger projects — see [IMPROVEMENTS.md](IMPROVEMENTS.md) for existing patterns.

---

## How the pieces connect (end-to-end example)

**DualSense on Pico W → Xbox 360 (XInput) output**

1. Bluepad32 connects DualSense → `Bluepad32.cpp` fills `PadIn`.
2. `UserSettings` says current driver is `DeviceDriverType::XINPUT`.
3. `DeviceManager` owns `XInputDevice`.
4. Core0 `process()` maps `PadIn` → Xbox 360 report; `tud_xinput` + **libxsm3** handle USB + console auth.
5. Rumble from the 360 returns via `PadOut` → (for BT) Bluepad32 feedback path.

**Wired Switch Pro on Feather → PS3 output**

1. HID mount → `HardwareIDs` → `SWITCH_PRO` → `SwitchProHost::process_report` → `PadIn`.
2. `DeviceDriverType::PS3` → PS3 device driver builds PS3 reports (and motion if enabled).

---

## CMake / build awareness for contributors

| Item | Notes |
|------|--------|
| Host drivers | Listed under `if(EN_USB_HOST)` in `CMakeLists.txt` — **must** add your `.cpp` |
| Device drivers | In the main `SOURCES_BOARD` list |
| Board | `-DOGXM_BOARD=…` selects pins and BT/USB features |
| Debug | `-DCMAKE_BUILD_TYPE=Debug` enables `CONFIG_OGXM_DEBUG` / `OGXM_LOG` |
| Patches | Applied every configure; “already applied” is normal |

Build walkthrough: [Building_From_Source.md](Building_From_Source.md).

---

## Suggested learning path for newcomers

1. Read this document’s **mental model** and **PadIn** section.  
2. Build and flash once: [Building_From_Source.md](Building_From_Source.md).  
3. Trace one host driver (`N64` or `DInput`) from `HardwareIDs.h` → `HostManager` → `process_report`.  
4. Trace one device driver (`DInput` or `Switch`) from `DeviceManager` → `process`.  
5. To add a pad: capture **full HID reports**, then follow the host checklist above + [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md).  
6. For issues: [Support_Issue_Requirements.md](Support_Issue_Requirements.md).

---

## Quick reference — critical paths

| Concern | Path |
|---------|------|
| Entry | `src/main.cpp` |
| Board run loops | `src/OGXMini/Board/Standard.cpp`, `PicoW.cpp` |
| Host factory | `src/USBHost/HostManager.h` |
| Host VID/PID | `src/USBHost/HardwareIDs.h` |
| Host types | `src/USBHost/HostDriver/HostDriverTypes.h` |
| Device factory | `src/USBDevice/DeviceManager.cpp` |
| Device types | `src/USBDevice/DeviceDriver/DeviceDriverTypes.h` |
| PadIn | `src/Gamepad/Gamepad.h` |
| Mode combos / flash | `src/UserSettings/UserSettings.cpp` |
| BT → PadIn | `src/Bluepad32/Bluepad32.cpp` |
| Build sources | `CMakeLists.txt` |
