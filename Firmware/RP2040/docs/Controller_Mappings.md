# Controller button and stick mappings

This document describes how OGX-Mini maps **input controllers** (USB host / Bluetooth) into the internal **`PadIn`** layout, and how each **output mode** maps **`PadIn`** to what the console or host sees.

**Related docs**

| Topic | Document |
|-------|----------|
| GPIO output (PS1/PS2, Dreamcast, GameCube, N64) pin-outs + mappings | [GPIO_Output_Pinout_and_Mappings.md](GPIO_Output_Pinout_and_Mappings.md) |
| Wii **output** mode (Wiimote over BT) | [Wii_Mode_Guide.md](Wii_Mode_Guide.md) |
| Supported wired USB pads (VID/PID lists) | [Wired_Controllers.md](Wired_Controllers.md) |
| Adding a new controller (capture, drivers, UART debug) | [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) |
| PS3/PS4 motion (accel/gyro) | [IMPROVEMENTS.md — motion passthrough](IMPROVEMENTS.md#ps3--ps4-output--motion-passthrough) |
| Bluetooth pad list (Bluepad32) | [Bluepad32 supported gamepads](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/) |

---

## How mappings work

```text
[Input controller]  →  PadIn  →  [Output driver]  →  Console / PC / GPIO
     USB host              ↑              USB gadget or GPIO protocol
     Bluetooth (Bluepad32)   │
                             └── User profiles may remap MAP_* fields (web app)
```

**`PadIn`** is the firmware’s canonical gamepad state (`Gamepad::PadIn` in `Gamepad.h`). Names follow **Xbox-style** layout:

| PadIn field | Meaning |
|-------------|---------|
| **A / B / X / Y** | Face buttons (A = south / Cross on PlayStation) |
| **LB / RB** | Bumpers |
| **trigger_l / trigger_r** | Analog triggers (0–255) |
| **Start / Back** | Menu buttons |
| **SYS** | Guide / Home / PS |
| **MISC** | Share / Capture / touchpad click (pad-dependent) |
| **L3 / R3** | Stick clicks |
| **dpad** | Up / Down / Left / Right (diagonals supported) |
| **joystick_lx/ly, joystick_rx/ry** | Sticks (−32768 … +32767, center 0) |
| **accel / gyro** | Motion (when input pad provides IMU) |

**User remapping:** For most **USB output** modes, button and stick assignments can be changed in the [web app](https://megacadedev.github.io/OGX-Mini-2026-WebApp/) (saved profiles alter `Gamepad::MAP_*` fields). **GPIO output** modes and some specialty mappings use fixed defaults only (see [GPIO doc](GPIO_Output_Pinout_and_Mappings.md#changing-mappings)).

**Source code index:** `Gamepad.h` (PadIn), `Bluepad32/Bluepad32.cpp` (BT → PadIn), `USBHost/HostDriver/*` (wired USB → PadIn), `USBDevice/DeviceDriver/*` and `Gamepad/*` GPIO drivers (PadIn → output).

---

## Input → PadIn

### Standard layout (most Bluetooth and USB pads)

Bluepad32 (Pico W / Pico 2 W) and most USB host drivers (XInput, DualShock 3/4/5, etc.) map the physical controller into **`PadIn` using the Xbox-style table above** — face buttons, bumpers, triggers, sticks, and menu buttons align with their Xbox equivalents. PlayStation and Switch pads are normalized to this layout in the host/BT layer.

| Physical (examples) | PadIn |
|---------------------|-------|
| Xbox A / DS Cross / Switch B | **A** |
| Xbox B / DS Circle / Switch A | **B** |
| Xbox X / DS Square / Switch Y | **X** |
| Xbox Y / DS Triangle / Switch X | **Y** |
| View / Select / Minus | **Back** |
| Menu / Start / Plus | **Start** |
| Guide / PS / Home | **SYS** |
| Share / Capture | **MISC** (when present) |

**Disconnect combo (Bluetooth):** **Start + Back** (~0.5 s). **OUYA:** **L3 + R3** (no Start/Select).

### Steam Controller 2026 / Triton (Bluetooth LE)

Wireless only on **Pico W / Pico 2 W / RP2354** (`28de:1303`). Pairing and protocol: [IMPROVEMENTS — Steam Controller 2026](IMPROVEMENTS.md#steam-controller-2026-triton--bluetooth).

Valve’s printed **View / Menu** labels do **not** match Xbox View/Menu. Firmware follows SDL:

| Triton control | PadIn |
|----------------|-------|
| **View** | **Start** |
| **Menu** | **Back** |
| **Steam** (or QAM) | **SYS** |
| A / B / X / Y, LB / RB, L3 / R3, d-pad, LT / RT, sticks | Standard layout |
| L4 / L5 / R4 / R5 grips | Unmapped |

---

### Wii Remote (Bluetooth input)

Handheld Wiimotes (not Wii U Pro, Classic Controller, or Balance Board) are forced to **vertical (TV-style)** orientation on connect: `uni_hid_parser_wii_set_mode(WII_MODE_VERTICAL)` in `Bluepad32.cpp`. Hold the remote with the **IR end toward the TV**. Plug the **Nunchuk in before pairing** (or reconnect after attaching it) so extension init completes.

| Wiimote (vertical) | PadIn |
|--------------------|-------|
| D-pad | D-pad |
| **B** (trigger under body) | **A** |
| **A** (large face button) | **B** |
| **1** | **X** (no Nunchuk) / **LB** (with Nunchuk) |
| **2** | **Y** (no Nunchuk) / **RB** (with Nunchuk) |
| **+** | **Start** |
| **−** | **Back** |
| **Home** | **SYS** |

| Nunchuk | PadIn |
|---------|-------|
| **Stick** | **Left stick** (`joystick_lx` / `joystick_ly`) |
| **C** | **X** |
| **Z** | **Y** |

**Motion:** In **PS3 / PS4 output mode**, continuous accel reports are requested: **`0x31`** (Wiimote only) or **`0x35`** (Wiimote + Nunchuk). **Switch** output does not request or pass through motion for now. See [motion passthrough](IMPROVEMENTS.md#ps3--ps4-output--motion-passthrough).

**Triggers:** Wiimote **B** maps to **A**, not analog triggers. Shoulder/Z does not fill `trigger_l` / `trigger_r` when mapped as LB/RB.

---

### Switch Pro Controller (wired USB host)

Switch 1 Pro (`0x2009`) uses `SwitchProHost` with deliberate swaps vs a 1:1 Pro layout (`SwitchPro.cpp`):

| Switch Pro physical | PadIn |
|---------------------|-------|
| **Y** | **X** |
| **B** | **A** |
| **A** | **B** |
| **X** | **Y** |
| **L / R** | **LB / RB** |
| **ZL / ZR** | **trigger_l / trigger_r** (digital full) |
| **− / +** | **Back / Start** |
| **Home** | **SYS** |
| **Capture** | **MISC** |
| **L3 / R3** | **R3 / L3** (swapped) |
| D-pad Up/Down/Left/Right | D-pad **Down/Up/Right/Left** (swapped) |

Sticks are scaled to `PadIn` int16 range. IMU is available for **PS3/PS4 output** motion passthrough.

---

### Switch 2 Pro (wired USB host)

Switch 2 Pro (`0x2069`) uses `Switch2ProHost` with **Switch-2-specific** bit positions (not the classic `Buttons0/1/2` layout). See `Switch2ProHost.cpp` for bit definitions. Summary:

| Switch 2 Pro | PadIn |
|--------------|-------|
| Face **B / A / Y / X** (bits 0–3) | **A / B / X / Y** |
| **RB** (bit 4) | **RB** |
| **LB** (Home bit on misc byte) | **LB** |
| **Minus** | **Back** |
| Classic **R** bit | **Start** |
| Stick **L3** (classic ZR bit) | **L3** |
| Stick **R3** (misc bit 7) | **R3** |
| **Home** (left byte) | **SYS** |
| **ZL / ZR** digital bits | **trigger_l / trigger_r** (digital) |
| D-pad (misc − / L3 / + / R3 bit positions) | D-pad |

Sticks use the same 12-bit packing as Switch 1 Pro, with **Y inverted** into PadIn. **Anti-deadzone** profiles apply a small noise floor (~8%) when inner deadzone is unset so imperfect stick centers do not drift.

**GL**, **GR**, and **Chat** are documented in source but intentionally unmapped.

---

### Generic / DInput USB host

Unknown HID pads use the generic host path; mapping depends on the report descriptor. Use the [controller capture tool](../../../Tools/controller_capture/README.md) when adding support. DInput **output** mode uses PlayStation names on the wire (see below).

---

## PadIn → USB output modes

Unless noted, **D-pad** and **sticks** map directly. **Triggers** map to analog axes where the protocol supports them.

### XInput (Xbox 360)

| PadIn | Xbox 360 |
|-------|----------|
| A / B / X / Y | A / B / X / Y |
| LB / RB | LB / RB |
| trigger_l / trigger_r | LT / RT |
| Back / Start | Back / Start |
| L3 / R3 | L3 / R3 |
| SYS | Guide |
| D-pad | D-pad |
| Sticks | Left / right stick |

**Source:** `USBDevice/DeviceDriver/XInput/XInput.cpp`

---

### PlayStation 3 (DualShock 3 / Sixaxis)

| PadIn | DS3 |
|-------|-----|
| A | **Cross** |
| B | **Circle** |
| X | **Square** |
| Y | **Triangle** |
| LB / RB | L1 / R1 |
| trigger_l / trigger_r | L2 / R2 (+ analog axes) |
| Back / Start | Select / Start |
| L3 / R3 | L3 / R3 |
| SYS | **PS** (8-frame latch for BT taps) |
| MISC | Touchpad (byte bit; rarely used on DS3) |
| D-pad | D-pad |
| Sticks | Left / right (0–255, center 0x80) |
| accel / gyro | **Sixaxis** (when input provides motion) |

**Source:** `USBDevice/DeviceDriver/PS3/PS3.cpp`

---

### PlayStation 4 (DualShock 4 USB)

| PadIn | DS4 |
|-------|-----|
| A / B / X / Y | Cross / Circle / Square / Triangle |
| LB / RB | L1 / R1 |
| trigger_l / trigger_r | L2 / R2 |
| Back / Start | Share / Options |
| L3 / R3 | L3 / R3 |
| SYS | PS |
| MISC | Touchpad click |
| D-pad / sticks | D-pad / sticks |
| accel / gyro | DS4 motion fields (when input provides motion) |

**Source:** `USBDevice/DeviceDriver/PS4/PS4.cpp`

---

### Nintendo Switch (Pro Controller emulation)

Face buttons use **Nintendo names on the wire**; mapping from PadIn is **crossed** so PlayStation/Xbox inputs feel correct on Switch:

| PadIn | Switch Pro output |
|-------|-------------------|
| **A** | **B** |
| **B** | **A** |
| **X** | **Y** |
| **Y** | **X** |
| LB / RB | **L / R** |
| trigger_l / trigger_r | **ZL / ZR** |
| Back / Start | **− / +** |
| L3 / R3 | L3 / R3 |
| SYS | **Home** |
| MISC | **Capture** |
| D-pad | D-pad |
| Sticks | Left / right (12-bit; Y inverted vs PadIn) |

**Rumble (v1.0.0.12a):** Console **HD rumble** in output reports **`0x01` / `0x10` / `0x11`** is decoded into **`PadOut`** and forwarded to the input pad (Bluetooth DualSense / Xbox, etc.). See [IMPROVEMENTS — Switch HD rumble](IMPROVEMENTS.md#switch-mode--hd-rumble-passthrough).

**Source:** `USBDevice/DeviceDriver/Switch/Switch.cpp`

---

### DInput (DirectInput)

| PadIn | DInput report |
|-------|----------------|
| A / B / X / Y | Cross / Circle / Square / Triangle |
| LB / RB | L1 / R1 |
| trigger_l / trigger_r | L2 / R2 axes |
| Back / Start | Select / Start |
| SYS / MISC | Mode / TP |
| L3 / R3 | L3 / R3 |
| D-pad / sticks | D-pad / sticks |

**Source:** `USBDevice/DeviceDriver/DInput/DInput.cpp`

---

### SteamOS / Bazzite (STEAM mode)

**Select mode:** **Start + Left Bumper + D-pad Up** (~3 s).

Enumerates as **DualSense** (`054c:0ce6`) plus a **HID mouse** interface. **DualSense input** (BT or wired USB): gamepad report is **passthrough**; touchpad → separate **mouse** interface. **Other input:** report is **synthesized** from PadIn using the mapping below; **no stick-mouse fallback**.

| PadIn | DualSense USB |
|-------|----------------|
| A / B / X / Y | Cross / Circle / Square / Triangle |
| LB / RB | L1 / R1 |
| trigger_l / trigger_r | L2 / R2 |
| Back / Start | Share / Options |
| L3 / R3 | L3 / R3 |
| SYS | PS |
| MISC | Touchpad click |
| **DualSense touchpad** (input) | USB mouse movement + click (IF1) |

**Source:** `USBDevice/DeviceDriver/Steam/*`, `SteamBtReport.h`, `SteamTouchpad.cpp`

---

### Original Xbox (Gamepad)

| PadIn | OG Xbox |
|-------|---------|
| A / B / X / Y | A / B / X / Y (digital or analog face per build) |
| LB / RB | White / Black |
| trigger_l / trigger_r | LT / RT |
| Back / Start | Back / Start |
| L3 / R3 | L3 / R3 |
| SYS | **Guide** (tap = Start; hold chords for IGR/shutdown — see README) |
| Sticks | Left / right (**Y inverted** on wire) |

**Source:** `USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp`

---

### Original Xbox — Steel Battalion

Uses a dedicated chatpad + gamepad map table (`XboxOG_SB.cpp`). See source `GP_MAP` / `CHATPAD_MAP` for the full SB control surface.

---

### Original Xbox — DVD Remote (XRemote)

| PadIn | Remote button |
|-------|----------------|
| SYS | Display |
| Start | Play |
| Back | Stop |
| A | Select |
| Y | Pause |
| X | Display |
| B | Back |
| LB / RB (alone or together) | Skip − / Skip + / Display |
| L3 / R3 (alone or together) | Title / Menu / Info |

**Source:** `USBDevice/DeviceDriver/XboxOG/XboxOG_XR.cpp`

---

### PlayStation Classic

| PadIn | PS Classic |
|-------|------------|
| A / B / X / Y | Cross / Circle / Square / Triangle |
| LB / RB | L1 / R1 |
| Back / Start | Select / Start |
| D-pad / sticks | D-pad / sticks |

**Source:** `USBDevice/DeviceDriver/PSClassic/PSClassic.cpp`

---

### Wii U (GameCube adapter emulation)

Emulates a **GameCube controller** on the Wii U adapter port. Face buttons are **swapped** to match GC layout:

| PadIn | GameCube (Wii U report) |
|-------|-------------------------|
| **B** | **A** |
| **A** | **B** |
| **Y** | **X** |
| **X** | **Y** |
| Start | Start |
| RB | **Z** |
| trigger_l / trigger_r (full) | **L / R** digital |
| D-pad | D-pad |
| Left stick | Main stick (Y axis may invert for Xbox vs Nintendo hosts) |
| Right stick | C-stick |

**LB** is not mapped to a GC button. **Source:** `USBDevice/DeviceDriver/WiiU/WiiU.cpp`

---

## PadIn → GPIO output modes

PS1/PS2, Dreamcast, GameCube, and N64 use fixed **PadIn → console** tables and pin-outs documented in:

**[GPIO_Output_Pinout_and_Mappings.md](GPIO_Output_Pinout_and_Mappings.md)**

---

## USB gamepad → Wii Remote (Wii output mode)

When the firmware is built with **`-DOGXM_FIXED_DRIVER=WII`**, a USB/BT gamepad is converted into Wiimote (+ extension) reports over Bluetooth. Mappings for **No Extension**, **Nunchuk**, and **Classic Controller** are in:

**[Wii_Mode_Guide.md — §5 Button and stick mapping](Wii_Mode_Guide.md#5-button-and-stick-mapping-by-mode)**

---

## Quick reference: PlayStation ↔ PadIn ↔ Xbox

| PlayStation | PadIn | Xbox |
|-------------|-------|------|
| Cross | **A** | A |
| Circle | **B** | B |
| Square | **X** | X |
| Triangle | **Y** | Y |
| L1 / R1 | LB / RB | LB / RB |
| L2 / R2 | triggers | LT / RT |
| Share / Options | Back / Start | View / Menu |
| PS | SYS | Guide |

Use this table when reading any **PadIn → output** section above.
