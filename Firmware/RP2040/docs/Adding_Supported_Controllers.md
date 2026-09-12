# Adding supported controllers

This guide is for **community contributors** who want to add a new USB (or Bluetooth) gamepad themselves: identify the device, capture its report layout, map buttons into firmware, optionally add debug logging, and open a pull request.

**Maintainer policy:** New first-party maintainer support normally requires hardware on hand, a **donation to purchase**, or a shipping agreement — see the main [README — Support policy](../../../README.md#support-policy). Opening an issue? Include everything in [Support_Issue_Requirements.md](Support_Issue_Requirements.md) or the issue may be closed or delayed. Everyone else is welcome to **clone this repo**, implement support, and **open a PR**. Board-specific fixes are especially welcome for targets the maintainer does not own.

> **Do not submit PC capture script output for mapping or new-controller support.**  
> The tools under `Tools/controller_capture/` — including **`controller_capture.py`** (interactive input grabber) and **`hidraw_full_report_dump.py`** (Linux hidraw stream) — **do not produce information that can be used** to map a pad correctly **at this time**. **Issues and PRs that attach logs from those scripts will not be used** and will be closed or delayed. For acceptable evidence, use **on-device Debug UART** full report hex from the adapter (Step 3–4) and/or **USB analyzer** captures on the adapter host port when init/handshake matters.

**Related docs**

| Topic | Document |
|-------|----------|
| How the firmware is structured / which files enable a driver | [Firmware_Architecture.md](Firmware_Architecture.md) |
| Supported wired pads (by driver) | [Wired_Controllers.md](Wired_Controllers.md) |
| PadIn ↔ output mode mappings | [Controller_Mappings.md](Controller_Mappings.md) |
| Capture on the adapter (required for mapping issues/PRs) | [Step 2](#step-2--capture-reports-on-the-adapter-required-for-driver-mapping) + [Step 3–4](#step-3--build-debug-firmware-and-read-uart-logs) (Debug UART) |
| PC scripts in `Tools/controller_capture/` — **not accepted** | `controller_capture.py`, `hidraw_full_report_dump.py`, etc. |
| Bluetooth gamepad list (upstream) | [Bluepad32 supported gamepads](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/) |

---

## How input support works

```text
USB plug / BT connect
        │
        ▼
 VID/PID (or XInput class) → HostManager picks a HostDriver
        │
        ▼
 process_report() parses HID / XInput bytes → Gamepad::PadIn
        │
        ▼
 Output mode (XInput, PS3, Switch, STEAM, GPIO, …) reads PadIn
```

- **Wired USB host:** TinyUSB calls `tuh_hid_mount_cb` / XInput mount → `HostManager::setup_driver()` → a class under `Firmware/RP2040/src/USBHost/HostDriver/`.
- **Unknown HID** with a parseable joystick report descriptor can fall back to **`HID_GENERIC`** (`HIDGeneric.cpp`).
- **Bluetooth (Pico W / Pico 2 W / RP2354):** Bluepad32 parsers → `Bluepad32.cpp` → `PadIn`. New BT brands usually mean work in Bluepad32 (or a custom BLE path), not only `HardwareIDs.h`.

Canonical state is **`Gamepad::PadIn`** (Xbox-style names: A/B/X/Y, LB/RB, triggers, Start/Back, SYS, MISC, sticks, d-pad). Always map **into PadIn**; do not special-case output modes inside the host driver. See [Controller_Mappings.md](Controller_Mappings.md).

---

## Step 0 — Decide what kind of work you need

| Situation | Typical fix |
|-----------|-------------|
| Pad already speaks **Xbox 360 / One / Series / OG** over USB (XInput / GIP) | Often **no VID list** — the XInput class driver mounts it. If it fails, capture logs and compare to `Xbox360` / `XboxOne` hosts. |
| Same protocol as an existing list (DInput, PS3/4/5, Switch wired, N64, …) but **new VID/PID** | Add `{ vid, pid }` to the matching array in [`HardwareIDs.h`](../src/USBHost/HardwareIDs.h). |
| Standard HID joystick; unknown VID/PID | May already work as **HID Generic**. If buttons are wrong, fix mapping in `HIDGeneric.cpp` **or** add a dedicated driver. |
| Custom report / init handshake / multi-interface vendor protocol | **New `HostDriver`** (+ optional descriptor struct under `Descriptors/`). |
| Bluetooth-only pad | Bluepad32 support (or a dedicated BLE parser). On-device UART capture on the adapter still helps document the layout. |

**VID/PID alone is not always enough.** Mode switches (XInput vs DInput vs Switch), report IDs, init packets, and stick axis polarity all matter. Prefer the mode that matches an existing driver when the pad offers one.

---

## Step 1 — Get VID and PID

Use **uppercase hex** in code (e.g. `0x054C`, `0x09CC`). Note the **controller mode** when you read IDs (XInput / DInput / Switch / Android, etc.).

### Windows

- **Device Manager** → Sound, video and game controllers or HID → Properties → Details → **Hardware Ids** → `HID\VID_xxxx&PID_yyyy`.
- Or **USBDeview** (NirSoft).

### Linux

```bash
lsusb
# Bus ... ID vid:pid Manufacturer Product
```

Or `lsusb -v` → `idVendor` / `idProduct`.

### macOS

**System Information** → USB (or Bluetooth) → Vendor ID / Product ID (often decimal — convert to hex).

### Browser

[gamepad-tester.com](https://gamepad-tester.com/) and similar tools sometimes show VID/PID via the Gamepad API.

---

## Step 2 — Capture reports on the adapter (required for driver mapping)

Host drivers decode **raw USB HID (or XInput) report bytes** as **TinyUSB delivers them on the OGX-Mini USB host port** — bit masks, byte offsets, axis ranges, report IDs. You must map from those buffers, not from OS/SDL abstractions or PC-side hidraw.

### Not accepted — PC scripts in `Tools/controller_capture/`

**At this time, do not submit output from any of the PC capture scripts for mapping support, issues, or PRs.** That includes:

| Script | Why it is not accepted |
|--------|-------------------------|
| **`controller_capture.py`** | SDL/pygame indices and optional hidraw sidecar do not reliably match adapter-side report layout. |
| **`hidraw_full_report_dump.py`** | Linux hidraw on a PC is **not** the same path as the Pico USB host stack (init, interfaces, report IDs, and timing differ). Logs from this script **cannot be used** for mapping at this time. |
| **`analyze_hidraw_xor.py`** on the above | Same source data — not acceptable as submission evidence. |

**Issues and PRs that attach only these dumps will be closed or delayed.** VID/PID from `lsusb` / Device Manager is still fine.

### Accepted capture (what maintainers can use)

Use evidence from **the adapter itself** (or USB traffic **on the adapter’s host port**):

#### A) On-device Debug UART — **preferred** (Step 3–4)

Plug the pad into the **OGX-Mini USB host port**, flash a **Debug** build, and log **`report` / `len` + full hex** from `process_report` (or temporary logging in the receive path). That is the ground truth for OGX host drivers — especially when the pad needs adapter-side init or does not enumerate cleanly on a PC.

Method:

1. Leave the pad at **rest**; note bytes that **always** change (report timer, IMU) — ignore those as button signatures.
2. Press **one** digital control at a time; record which bytes/bits flip vs idle.
3. Move each stick **slowly through full range** (and each trigger); record which bytes are the axes and their min/center/max.
4. Build a packed `InReport` / bit masks from those offsets (see `Descriptors/*.h` for examples).
5. Re-check: hold a face button while moving sticks so axis bytes do not collide with your button masks.

See [Step 3](#step-3--build-debug-firmware-and-read-uart-logs) and [Step 4](#step-4--add-temporary-full-report-logging-on-the-adapter).

#### B) USB analyzer on the adapter host port (init / multi-interface)

When bring-up matters (Switch 2 bulk, feature reports, multiple interfaces):

- **Wireshark + USBPcap** (Windows) or **usbmon** (Linux) on the **adapter ↔ controller** link (not “PC hidraw only”).
- Capture setup packets, feature reports, and input endpoints **as the adapter sees them**.

`Tools/controller_capture/` scripts may exist for historical or local experimentation; they are **not** supported submission formats.

### How to turn full reports into a mapping table

| Goal | What to collect |
|------|-----------------|
| Digital buttons | Idle hex vs held hex for **each** button; note byte index + bit mask |
| D-pad | Hat nibble/enum values for N/S/E/W/diagonals/neutral |
| Triggers | Bytes (or bits) from released → fully pressed |
| Sticks | Bytes for X/Y at center, +X, −X, +Y, −Y; signed vs unsigned; invert flags |
| Report framing | Report ID byte(s), length variants, which interface carries gamepad data |

Save the dump (and note controller **mode**, board, firmware) with your issue or PR.

---

## Step 3 — Build Debug firmware and read UART logs

Debug builds enable `CONFIG_OGXM_DEBUG` / `OGXM_DEBUG`. Logging goes to **UART**, not USB CDC (TinyUSB host disables USB stdio).

### Build

Interactive (from repo root):

```bash
./scripts/build.sh          # Linux / macOS
# or
.\scripts\build.ps1         # Windows
```

Choose your **board**, then **Debug** (UART logging).

Manual example:

```bash
cd Firmware/RP2040
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOGXM_BOARD=PI_PICOW
cmake --build build
```

Flash the `.uf2` from the build output folder.

### UART pins and baud

| Board family | TX pin | RX pin | Baud |
|--------------|--------|--------|------|
| **Pico W / Pico 2 W / RP2354** | **GP4** | **GP5** | **115200** 8N1 |
| **Most other boards** (Pico, Zero, Feather, RP2350-USB-A, …) | **GP0** | **GP1** | **115200** 8N1 |

Pico W / 2 W use GP4/GP5 so UART does not collide with PIO USB on GP0/GP1.

Connect a USB–UART adapter: adapter **RX** ← Pico **TX**, common **GND**. Open a serial terminal at **115200** (e.g. `minicom`, `screen`, PuTTY, Arduino Serial Monitor).

```bash
# Linux example
screen /dev/ttyUSB0 115200
# or
picocom -b 115200 /dev/ttyUSB0
```

You should see mount messages (`… Loaded`), Bluepad32 lines on wireless boards, and any `OGXM_LOG` / `printf` you add.

### Logging APIs

Include `Board/ogxm_log.h`:

```cpp
OGXM_LOG("MyPad mount vid=0x%04x pid=0x%04x len=%u\n", vid, pid, len);
OGXM_LOG_HEX(report, len);   // dump bytes
```

In **Release**, these macros compile out.

**Bluepad32 (Debug):** `BLUEPAD32_UART_LOG_INPUT` defaults **on** in Debug and prints gamepad state when it changes (`[BP32 idx=…] …`).

**Do not flood UART** on every HID packet (many pads report at 250–1000 Hz). Log only when button bytes change, or throttle. See `Switch2ProHost.cpp` + `OGXM_SWITCH2_HID_RAW_LOG` for a change-gated hex dump pattern:

```bash
cmake … -DCMAKE_BUILD_TYPE=Debug -DOGXM_SWITCH2_HID_RAW_LOG=ON …
```

---

## Step 4 — Add temporary **full-report** logging on the adapter

Use this when the PC dump is incomplete, the pad only works after adapter init, or you need the exact buffer TinyUSB delivers.

1. Build **Debug**.
2. In the driver’s `process_report` (or a temporary dump in `HostManager::process_report` / HID receive path), log **`len` and the full report hex**.
3. Gate on change so UART is readable: compare the **whole** report, or a masked copy with timer/IMU bytes zeroed — but still **print the full hex** when something meaningful changes so you can see every bit/axis field.
4. Press **one** control at a time; sweep sticks/triggers; write a byte/bit map from the log.
5. Gate or remove the logging behind `#ifdef` / a CMake option before merging (keep Release clean).

Minimal pattern (change-gated **full** dump):

```cpp
#include "Board/ogxm_log.h"
#include <cstring>

#if defined(CONFIG_OGXM_DEBUG)
static uint8_t prev[64]{};
static uint16_t prev_len = 0;
const uint16_t n = len < 64 ? len : 64;
if (n != prev_len || std::memcmp(prev, report, n) != 0) {
    prev_len = n;
    std::memcpy(prev, report, n);
    OGXM_LOG("HID len=%u\n", (unsigned)len);
    OGXM_LOG_HEX(report, n);   // full buffer the driver must parse
}
#endif
```

If the pad includes a free-running timer or IMU, mask those bytes **only in the `memcmp`**, still dump the **unmasked** report with `OGXM_LOG_HEX` so axis/button bytes remain visible. See `Switch2ProHost.cpp` + `OGXM_SWITCH2_HID_RAW_LOG` for a production-style gate.

Always call `tuh_hid_receive_report(address, instance)` again after handling a HID report (existing drivers already do this).

---

## Step 5 — Easiest path: add VID/PID to an existing driver list

1. Confirm the pad’s **full report** matches an existing driver (DInput Sony-style, PS4, Switch wired, N64, etc.) — compare hex dumps to `Descriptors/*.h`, not SDL indices alone.
2. Edit `Firmware/RP2040/src/USBHost/HardwareIDs.h` — add `{0xVID, 0xPID}, // Name / mode` to the correct array (`DINPUT_IDS`, `PS4_IDS`, `SWITCH_WIRED_IDS`, …).
3. Arrays are wired through `HOST_TYPE_MAP` at the bottom of the same file — new arrays need a new `HostTypeMap` entry **and** a `case` in `HostManager::setup_driver`.
4. Rebuild, flash, test every face button, bumpers, triggers, sticks, d-pad, Start/Back/Guide.
5. Document the pad in [Wired_Controllers.md](Wired_Controllers.md) and mention it in the PR.

**Do not** put Microsoft XInput pads (`045E` gamepads that use the XInput class) into `DINPUT_IDS` — that can double-bind HID + XInput and break input (see comments in `HardwareIDs.h`).

---

## Step 6 — Create a new USB host driver (custom brand / layout)

Use a small existing driver as a template: **`N64`** or **`PSClassic`** (simple HID → PadIn). More complex examples: **`DInput`**, **`PS4`**, **`SwitchPro`**.

### Checklist

1. **Report struct / constants** — e.g. `Firmware/RP2040/src/Descriptors/MyPad.h`  
   Packed `InReport`, button bit masks, stick midpoints, report IDs.

2. **Host class** — `Firmware/RP2040/src/USBHost/HostDriver/MyPad/MyPad.h` + `MyPad.cpp`  
   Inherit `HostDriver` and implement:
   - `initialize` — start HID receive (`tuh_hid_receive_report`); run any init/feature reports if needed.
   - `process_report` — parse bytes → fill `Gamepad::PadIn` → `gamepad.set_pad_in(gp_in)`; skip unchanged reports; re-arm receive.
   - `send_feedback` — rumble/LEDs if supported; otherwise `return true`.

3. **`HostDriverType`** — add an enum value in `HostDriverTypes.h`.

4. **`HardwareIDs.h`** — `MY_PAD_IDS[]` + entry in `HOST_TYPE_MAP`.

5. **`HostManager.h`** — `#include` the header; in `setup_driver` `switch`, `std::make_unique<MyPadHost>(gp_idx)`.

6. **`CMakeLists.txt`** — add `MyPad.cpp` next to the other `USBHost/HostDriver/...` sources.

7. **Stick Y polarity** — Xbox-class pads use “positive Y = up”; Nintendo-style often inverted. `HostManager` sets this for XInput types; for Nintendo-like reports follow Switch/N64 conventions or call `set_stick_y_positive_is_up` appropriately.

8. **Build Debug**, verify UART mount line and mappings, then Release.

### Mapping buttons into PadIn

Always OR into `gp_in` using **`gamepad.MAP_*`** (honors web-app profiles), not hardcoded bit constants alone:

```cpp
Gamepad::PadIn gp_in{};

if (in_report->buttons & MyPad::Buttons::A)
    gp_in.buttons |= gamepad.MAP_BUTTON_A;
if (in_report->buttons & MyPad::Buttons::B)
    gp_in.buttons |= gamepad.MAP_BUTTON_B;
// X, Y, LB, RB, L3, R3, BACK, START, SYS, MISC …

gp_in.trigger_l = gamepad.scale_trigger_l(raw_lt);  // or digital 0 / 255
gp_in.trigger_r = gamepad.scale_trigger_r(raw_rt);

std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
    gamepad.scale_joystick_l(raw_lx, raw_ly);
std::tie(gp_in.joystick_rx, gp_in.joystick_ry) =
    gamepad.scale_joystick_r(raw_rx, raw_ry);

gamepad.set_pad_in(gp_in);
```

Face-button convention into PadIn (Xbox names):

| Physical (examples) | PadIn |
|---------------------|-------|
| South / Cross / Switch B | **A** |
| East / Circle / Switch A | **B** |
| West / Square / Switch Y | **X** |
| North / Triangle / Switch X | **Y** |
| Select / View / Minus | **Back** |
| Start / Menu / Plus | **Start** |
| Guide / PS / Home | **SYS** |
| Share / Capture / touchpad click | **MISC** (when present) |

D-pad: set `gp_in.dpad` with `MAP_DPAD_*` (including diagonals if the pad reports them).

**User remapping** for USB output modes is done in the [web app](https://megacadedev.github.io/OGX-Mini-2026-WebApp/) via profiles — host drivers should keep using `MAP_*`.

---

## Step 7 — Bluetooth controllers

1. Check whether Bluepad32 already supports the pad: [supported gamepads](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/).
2. If yes but OGX mis-maps: adjust conversion in `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp` (PadIn fill from `uni_gamepad_t`).
3. If no: extend Bluepad32 (under `Firmware/external/bluepad32`) or add a custom BLE path (see Switch 2 / Steam Triton work in [IMPROVEMENTS.md](IMPROVEMENTS.md)).
4. Use a **Debug** Pico W / Pico 2 W build and watch `[BP32 …]` UART lines while pressing controls.
5. Pairing quirks (Classic BT vs BLE, DS3 `0xF5` USB pair, etc.) are documented in IMPROVEMENTS / the main README — follow those before assuming a parser bug.

---

## Step 8 — Test matrix and docs

Before opening a PR, exercise:

- [ ] All face buttons, bumpers, triggers (analog if present), sticks, stick clicks, d-pad, Start/Back/Guide
- [ ] Idle stick centers (no drift / no stuck bits)
- [ ] Hot-plug unplug/replug on your board’s USB host port
- [ ] At least one **USB output** mode you care about (and GPIO mode if relevant)
- [ ] Rumble / player LEDs if you implemented `send_feedback`
- [ ] **Release** build (no debug spam) still works

Update:

- [Wired_Controllers.md](Wired_Controllers.md) — name, VID/PID, mode notes  
- [Controller_Mappings.md](Controller_Mappings.md) — if the layout is non-obvious  
- Optional short note in CHANGELOG for the PR description  

### What to include in an issue or PR

- Controller **retail name** and **mode** (XInput / DInput / Switch / …)
- **VID** / **PID** (hex)
- Board + firmware version / commit
- **Debug UART full report hex** from the adapter (`OGXM_LOG_HEX` in `process_report`, Step 4) — **required** acceptable format for mapping issues/PRs. **Do not attach** `Tools/controller_capture/` script output (`controller_capture.py`, `hidraw_full_report_dump.py`, etc.) — those **cannot be used** at this time.
- UART log snippets if mount/init fails
- Summary of code changes (files touched)

---

## Quick reference — important paths

| Path | Role |
|------|------|
| `src/USBHost/HardwareIDs.h` | VID/PID → `HostDriverType` |
| `src/USBHost/HostManager.h` | Instantiates host drivers |
| `src/USBHost/tuh_callbacks.cpp` | TinyUSB HID / XInput mount hooks |
| `src/USBHost/HostDriver/*` | Per-protocol parsers → PadIn |
| `src/Descriptors/*` | Packed report layouts / bit masks |
| `src/Gamepad/Gamepad.h` | `PadIn`, `MAP_*`, scaling helpers |
| `src/Board/ogxm_log.h` | `OGXM_LOG` / `OGXM_LOG_HEX` |
| `src/Bluepad32/Bluepad32.cpp` | BT → PadIn |
| `Tools/controller_capture/` | **Not accepted** for mapping submissions — PC scripts do not match adapter-side reports |
| Debug UART + `OGXM_LOG_HEX` in host driver | **Accepted** — full report as TinyUSB delivers on the adapter |
| `Firmware/RP2040/CMakeLists.txt` | Sources + Debug UART pins |

---

## Common pitfalls

- **Submitting `Tools/controller_capture/` dumps** — **`controller_capture.py`**, **`hidraw_full_report_dump.py`**, and related PC script output are **not accepted** for mapping or new-pad support at this time. Use **Debug UART** full hex from the adapter instead.
- **Mapping from PC hidraw / SDL indices** — drivers need the **adapter-side** report layout; capture on the Pico host (Step 4).
- **Wrong mode** on multi-mode pads — capture and test the mode you listed in `HardwareIDs.h`.
- **Logging every report without a gate** — UART cannot keep up; gate the *trigger*, still print full hex when logging.
- **Timer / IMU bytes** mistaken for buttons — causes flicker; XOR against idle and ignore always-changing fields.
- **Forgetting `tuh_hid_receive_report`** — input stops after one packet.
- **Double drivers** (HID + XInput) — avoid duplicate list entries for devices that already mount as XInput.
- **Assuming SDL button indices equal HID bit positions** — they usually do not.
