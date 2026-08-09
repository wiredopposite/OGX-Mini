# Firmware Improvements

Improvements and fixes applied to the OGX-Mini RP2040 firmware in this project.

**Version:** From **v1.0.0a3** the version was bumped to **v1.0.0a4** to reflect Wii U controller fixes, Gamecube USB mode, PS3 driver fixes, latency improvements, and Xbox 360 (XInput) support (see below). **v1.0.0.8a+** documents **Pico W / Pico 2 W** work on **DualShock 4 (Classic Bluetooth)** vs **BLE advertising**, **BR inquiry**, and related BT stability (see *Pico W / Pico 2 W — DualShock 4 and Classic Bluetooth* below). **v1.0.0.10a** adds **Nintendo Switch 2** wireless support (**Pro 2** and **Joy-Con 2**) over **BLE**, **Switch 1 Joy-Con L+R merge**, and **Joy-Con dual-half latency fixes** (see *Nintendo Switch 2 — Bluetooth* and *Joy-Con pair merge — latency* below). **v1.0.0.11a** adds **PIO USB host wired connection fixes** — **Switch 1/2 Pro**, **DualShock 3**, **Xbox 360 wireless receiver**, and **Razer Atrox Xbox One** (`1532:0a00`) — validated on **Waveshare RP2350-USB-A**, plus **STEAM output mode** for **SteamOS / Bazzite** (see *STEAM mode* below). **v1.0.0.12a** adds **PS3 / PS4 motion passthrough**, **Bluetooth disconnect reboot**, **USB resume → restore BT pairing**, **XInput stock stick feel (#38)**, **Switch HD rumble passthrough**, **Switch 2 anti-deadzone / L3–R3 (#64)**, **RP2354 Bluetooth**, **STEAM touchpad-mouse-only**, **Steam Controller 2026 (Triton) BLE**, and **DualShock 3 USB → Bluetooth auto-pair restore** (sync feature **0xF5** after PIO USB wired init; see sections below).

**Version 1.0.0.12a — documented here for release notes:**

- **DualShock 3 — USB → Bluetooth auto-pair restored** — Sync **feature `0xF5`** after **0xF4** + LED (PIO USB sync init had skipped the old async F2→F5 path). See [§ DualShock 3 — automatic USB programming for Bluetooth pairing](#dualshock-3--automatic-usb-programming-for-bluetooth-pairing) and [§ DualShock 3 — wired USB host](#dualshock-3--wired-usb-host).
- **Steam Controller 2026 (Triton) — Bluetooth** — See [§ Steam Controller 2026 (Triton) — Bluetooth](#steam-controller-2026-triton--bluetooth) below.
- **PS3 / PS4 output — motion passthrough** — See [§ PS3 / PS4 output — motion passthrough](#ps3--ps4-output--motion-passthrough) below.
- **Pico W / Pico 2 W — Bluetooth disconnect reboot for clean reconnect** — See [§ Pico W / Pico 2 W — Bluetooth disconnect reboot for clean reconnect](#pico-w--pico-2-w--bluetooth-disconnect-reboot-for-clean-reconnect) below.
- **USB resume restores pairing scans** — See [§ Pico W / Pico 2 W / RP2354 BT — USB resume restores pairing scans](#pico-w--pico-2-w--rp2354-bt--usb-resume-restores-pairing-scans) below.
- **XInput / Xbox 360 — stock stick feel (#38)** — See [§ XInput / Xbox 360 — stock stick feel](#xinput--xbox-360--stock-stick-feel) below.
- **Switch mode — HD rumble passthrough** — See [§ Switch mode — HD rumble passthrough](#switch-mode--hd-rumble-passthrough) below.
- **Switch 2 Pro — anti-deadzone drift and L3/R3 (#64)** — See [§ Switch 2 Pro — anti-deadzone and L3/R3](#switch-2-pro--anti-deadzone-and-l3r3) below.
- **RP2354 Bluetooth** — Board profile uses Pico 2 W BT path; see [§ Additional board support](#additional-board-support-rp2350_zero-rp2040_xiao-rp2354).
- **STEAM mode** — Touchpad → mouse only (no stick-mouse); see [§ STEAM mode](#steam-mode--steamos--bazzite-linux-desktop).

**Version 1.0.0.9a — documented here for release notes:**

- **Bluetooth (Pico W / Pico 2 W):** ~**1 second** haptic **connection rumble** when a wireless controller reaches **device ready** (so you can feel that pairing completed). **DualShock 4** uses a **longer start delay** before rumble (same idea as the existing PS4 FF grace window) so early force-feedback does not destabilize the link. **File:** `src/Bluepad32/Bluepad32.cpp` — `ogxm_play_connection_rumble()` from `device_ready_cb`.
- **PS3 mode with the adapter plugged into a Windows PC:** **Host output rumble** forwarded to the Bluetooth pad no longer stays on at idle. Windows DInput often sends a **small non-zero** large-motor byte or a **small-motor byte other than 0/1**; the driver now applies a **deadzone** on the large motor and treats the small motor as on **only when the byte is `1`** (DS3 output semantics). **File:** `src/USBDevice/DeviceDriver/PS3/PS3.cpp` — `new_report_out_` path.
- **Pico W / Pico 2 W — PIO USB wired unplug:** **Reliable disconnect** when the gamepad cable is removed from the adapter’s PIO USB host port (see [§ Pico W / Pico 2 W — PIO USB wired controller unplug detection](#pico-w--pico-2-w--pio-usb-wired-controller-unplug-detection)). **Files:** `src/OGXMini/Board/PicoW.cpp`, `src/USBHost/HostManager.h`.
- **DualShock 3 — automatic Bluetooth pairing over USB (boards with Bluetooth):** When a **PS3 / DualShock 3** controller is used **wired** on the USB host, right after **0xF4** + LED init the firmware sync-sends **HID feature report `0xF5`** with the adapter’s **local BD_ADDR** (same as [Bluepad32’s sixaxispairer](https://bluepad32.readthedocs.io/en/latest/pair_ds3/)), so the user can **unplug USB** and connect with **PS**. If `uni_local_bd_addr` is not ready yet, pairing is **deferred** until a later report. **(Behavior as of v1.0.0.12a** — sync SET after PIO USB wired init; originally added in 1.0.0.9a.) **File:** `src/USBHost/HostDriver/PS3/PS3.cpp` (guarded by `CONFIG_EN_BLUETOOTH`).

---

## Pico W / Pico 2 W — Bluetooth disconnect reboot for clean reconnect

**Goal:** After a wireless controller fully connects and then disconnects, the next pair must work like a **fresh plug-in** — rumble, inputs, and pairing LED — without unplugging the Pico.

**Problem:** In-place BLE reconnect for **Xbox Series / One over BLE (HOGP)** does not reliably re-run the same path as first connect. Bonded **re-encryption**, leftover **HIDS** client / descriptor state, and scan flags left the adapter in a bad state: LED solid or off, no connection rumble, no gamepad input, until a power cycle. Rebooting **every** pad on disconnect (including Classic BT **8BitDo** / DualShock / Joy-Con) made **OG Xbox** look frozen on reconnect ([#86](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/86)).

**Approach:** Reboot only when the last ready pad was **Xbox BLE**. Classic BT and other BLE pads restore pairing mode and reconnect in place.

| Item | Detail |
|------|--------|
| **When (reboot)** | Last **Xbox BLE** pad that reached **device ready** disconnects |
| **When (no reboot)** | Classic BT / non-Xbox BLE ready disconnect (**8BitDo** Android/Switch, DS4, Joy-Con, Switch 2, Triton, etc.) |
| **Immediate** | Restore **pairing mode**: LED blink + BT scan / new connections enabled |
| **After ~500 ms (Xbox BLE only)** | **Watchdog reboot** (`watchdog_reboot`) from the BT core (Core1) |
| **USB resume** | Drop incomplete BLE slots stuck mid-DIS/HIDS after suspend, then restore scans |
| **Not when** | Failed pair attempts while already scanning (avoids reboot loops) |
| **Multi-pad** | No reboot while another BT pad is still connected |

**User flow (Xbox BLE):** Disconnect pad → LED flashes (pairing) → brief reboot (USB may re-enumerate) → LED flashes again → press connect → normal first-connect rumble and inputs.

**User flow (Classic / 8BitDo):** Disconnect pad → LED flashes → press connect → reconnects without adapter reboot.

**Files:** `src/Bluepad32/Bluepad32.cpp` (`device_ready_cb` sets `s_bt_slot_was_ready`; `device_disconnected_cb` restores pairing mode and schedules Xbox-BLE-only reboot; `drop_incomplete_ble_slots`), `src/Board/board_api.cpp` (`board_api::reboot()` via `watchdog_reboot`).

---

## 8BitDo Pro 2 / SN30 Pro — Bluetooth (Pico W / Pico 2 W)

**Goal:** Stable wireless input on **OG Xbox** (and other modes) with **8BitDo Pro 2** and **SN30 Pro** ([#86](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/86)).

**Problem:** Wrong controller mode (Windows/X-input) plus aggressive GAP inquiry (`inquiry=2`) caused failed discovery, laggy sticks/buttons, frequent drops, and a frozen adapter on reconnect when every disconnect triggered a full Pico reboot.

**Approach:**

| Item | Detail |
|------|--------|
| **Controller mode** | Put the pad in **Switch** or **Android (D-input)** before pairing — not Windows/X-input |
| **GAP inquiry** | Restore Bluepad32 defaults (`3` / `5` / `4` × 1.28 s) so Android/D-input 8BitDo pads are discoverable |
| **Reconnect** | Classic BT reconnect without reboot (see disconnect reboot section above) |
| **PIDs** | Register Android-mode IDs `0x2dc8:0x6003` and `0x6103` as `8BitdoController` |
| **Rumble** | Android/D-input 8BitDo path has no rumble parser yet; Switch mode uses Switch rumble |

**Files:** `src/Bluepad32/Bluepad32.cpp`, `Firmware/external/patches/bluepad32_8bitdo_pids.diff`, `Firmware/cmake/patch_libs.cmake`.

---

## Pico W / Pico 2 W / RP2354 BT — USB resume restores pairing scans

**Goal:** When the adapter stays plugged into a console that **USB-suspends** in standby (e.g. **Xbox 360** soft shutdown via Guide → Turn off console), Bluetooth **BR/EDR inquiry** and **BLE scan** must resume after the host wakes so the user can pair without unplugging the dongle.

**Problem:** With no wireless pad connected, long USB suspend could leave Bluepad32’s “new connections enabled” flag **true** while the radio had **stopped scanning** — LED solid or off, no pairing blink.

**Approach:**

| Item | Detail |
|------|--------|
| **USB resume** | `tud_resume_cb` → `bluepad32::on_usb_device_resume()` (marshaled to BT core) → `restore_bt_pairing_mode(-1)` when no BT pad and no PIO wired host pad is active |
| **Idle watchdog** | Every **45 s** on the BT run loop, restart BR/LE scans if idle and pairing should be active |
| **No reboot on resume** | Unlike pad disconnect, console wake does **not** trigger watchdog reboot (avoids XSM3 re-auth every standby cycle) |
| **Wired host release** | `wired_usb_release_enable_bt_pairing()` uses the same scan-restore path (not only `enable_new_connections`, which is a no-op when the flag is already true) |

**Files:** `src/Bluepad32/Bluepad32.cpp`, `src/USBDevice/tud_callbacks.cpp`.

---

## XInput / Xbox 360 — stock stick feel

**Goal:** Controllers used in **Xbox 360 (XInput) mode** — especially modern pads like **Xbox Series / One over Bluetooth** on Pico 2 W — should feel closer to a **stock 360** stick than a near-linear high-precision Series stick ([#38](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/38)).

**Problem:** Default profiles left sticks **uncurved** (`curve=1`, `dz_inner=0`, `uncap_radius=true`). Series pads report tiny physical deflections as strong USB values, so 360 games feel twitchy. A real 360 stick is quieter near center and firmer toward the rim.

**Approach (XInput mode only):**

| Item | Detail |
|------|--------|
| **When** | Current output driver is **XInput** and WebApp stick settings are still at defaults (not customized) |
| **Inner deadzone** | Remapped **~8%** left / **~9%** right (mild — not Microsoft’s ~24% software constant, so games that also deadzone are not double-dulled) |
| **Curve** | **0.35** (`mag^(1/curve)`): quieter center, steeper outer third |
| **Uncap** | **Off** — clamp to circular radius like a stock pad |
| **Override** | Any WebApp stick customization replaces these defaults |

**Files:** `src/UserSettings/JoystickSettings.*`, `src/Gamepad/Gamepad.h`, board `initialize()` (`PicoW.cpp`, `Standard.cpp`, `Four_Channel_I2C.cpp`).

---

## Switch mode — HD rumble passthrough

**Goal:** When OGX-Mini enumerates as a **Switch Pro** controller, console **HD rumble** must reach the **input** pad (especially Bluetooth DualSense / Xbox).

**Problem:** Output reports **`0x01`** / **`0x10`** / **`0x11`** (and related rumble commands) were acknowledged for Switch protocol bookkeeping but never decoded into **`PadOut`**, so wireless pads never vibrated in Switch mode.

**Approach:** Parse left/right HD rumble motor blocks (dekuNukem-style amplitude decode), write **`rumble_l` / `rumble_r`** into `PadOut`, and let Bluepad32 / USB host feedback forward motors to the input controller. Applies to all boards that support Switch output (including Pico 2 W / RP2354 with BT pads).

**Files:** `src/USBDevice/DeviceDriver/Switch/Switch.cpp`, `Switch.h`.

---

## Switch 2 Pro — anti-deadzone and L3/R3

**Goal:** Fix WebApp **anti-deadzone** drift and **L3/R3** mapping on wired **Switch 2 Pro** ([#64](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/64)).

**Problem:** Anti-Deadzone Circular/Square amplified imperfect Switch 2 Pro stick centers into constant bottom-left drift. Circular resize used the wrong WebApp field (`anti_dz_circle` vs `anti_dz_square`). Wired **L3/R3** were mapped to the wrong physical sticks.

**Approach:**

| Item | Detail |
|------|--------|
| **Idle gate** | Small noise floor when anti-dz is active (raw magnitude; adaptive with anti-dz strength) |
| **Circular formula** | Match WebApp (`anti_dz_square` where intended) |
| **L3/R3** | Correct physical stick clicks on wired Switch 2 Pro |
| **Profiles** | Joystick settings stay enabled when the saved profile customizes sticks/triggers |
| **Wired disconnect** | Bulk **keepalive** every **2 s** + USB disable-timeout after Switch 2 bulk bring-up; PIO unmount reboot **debounced 1.5 s** |

**Files:** `Gamepad.h`, `JoystickSettings.*`, `SwitchPro.cpp`, `Standard.cpp`, `Switch2ProHost.cpp`.

---

## STEAM mode — SteamOS / Bazzite (Linux desktop)

**Goal:** On **SteamOS**, **Bazzite**, or other **Linux desktops** (including **Steam Deck** in desktop mode), present a **PS5 DualSense** USB gamepad **and** route the **touchpad** to a separate **HID mouse** so the user can move the desktop cursor without Steam Input remapping.

### Select mode (button combos and build options)

| Method | Detail |
|--------|--------|
| **Button combo** | **Start + Left Bumper + D-pad Up** — hold ~3 s; saved to flash; RP2040 resets |
| **Web app** | Output mode **SteamOS / Bazzite** (value `16`) |
| **Fixed build** | **`-DOGXM_FIXED_DRIVER=STEAM`** — also option **13** in `scripts/build.sh` / `build.ps1` fixed-mode menu |

**User docs:** [README — SteamOS / Bazzite output mode](../../../README.md#steamos--bazzite-output-mode). **Mappings:** [Controller_Mappings.md — STEAM mode](Controller_Mappings.md#steamos--bazzite-steam-mode).

### DualSense USB emulation

| Item | Detail |
|------|--------|
| **USB VID:PID** | `054c:0ce6` (Sony DualSense) |
| **Interfaces** | IF0: DualSense gamepad (64-byte HID report, ID `0x01`); IF1: relative HID mouse |
| **DualSense input (BT or wired USB host)** | **`SteamPassthrough`** — full **64-byte** report copied each frame (ControllerMedic-style PS5 passthrough); wired host sticks/triggers taken from raw PS5 IN when available |
| **Other input (Xbox, DS4, Switch Pro, etc.)** | **`SteamBtReport`** — synthesizes a DualSense **`PS5::InReport`** from PadIn / `uni_gamepad_t` (face → Cross/Circle/Square/Triangle, LB/RB → L1/R1, triggers, sticks, D-pad, Share/Options, PS, touchpad-click bit) |
| **Descriptors** | `Descriptors/PS5Usb.h` (gamepad), `Descriptors/Steam.h` (composite + mouse interface) |

### Touchpad → mouse

| Item | Detail |
|------|--------|
| **Input** | **DualSense (PS5)** — Bluetooth on Pico W / Pico 2 W (`uni_hid_parser_ds5_get_touchpad()`), or wired USB host (`PS5Host::process_report()` → `SteamPassthrough::store()`) |
| **Output** | **`SteamTouchpad`** — touch finger deltas → **relative mouse** on IF1; touchpad click (`TP` / `MISC`) → **left button** |
| **Report merge (BT)** | Raw touch bytes merged into USB report offset **33** (`SteamTouchpad::apply_to_passthrough()`) for Linux `hid-playstation` compatibility |
| **No touchpad** | Mouse interface stays idle — no right-stick mouse fallback |

### How it works (code path)

1. **`SteamDevice::process()`** — Builds the outgoing DualSense USB report (passthrough or synthesized), then sends mouse HID **only** when the input pad has a touchpad.
2. **Bluetooth** — `SteamBtReport::update_from_uni_gamepad()` + touchpad parser; `Bluepad32.cpp` sets `SteamPassthrough::input_has_touchpad` for DS5.
3. **Wired USB host** — `PS5Host::process_report()` stores the raw report when output driver is **STEAM**.
4. **Non-PS5 input** — `SteamBtReport::update_from_gamepad()` / `fill_report_from_pad()` maps any supported PadIn into DualSense layout.

After changing to STEAM mode, **replug** the adapter USB cable on the PC so the composite device (gamepad + mouse) enumerates cleanly.

**Verify on Linux:**

```bash
lsusb -v -d 054c:0ce6 2>/dev/null | grep -E "iProduct|bNumInterfaces|bEndpointAddress"
sudo evtest   # select the mouse interface; drag touchpad for REL_X / REL_Y
```

**Files:** `src/USBDevice/DeviceDriver/Steam/` (`Steam.cpp`, `SteamPassthrough.h`, `SteamBtReport.h`, `SteamTouchpad.cpp`), `src/Descriptors/PS5Usb.h`, `src/Descriptors/Steam.h`, `src/Bluepad32/Bluepad32.cpp`, `src/USBHost/HostDriver/PS5/PS5.cpp`, `WebApp/modules/userSettings.js` (mode label **SteamOS / Bazzite**).

---

## PS3 / PS4 output — motion passthrough

**Goal:** In **PS3** and **PS4 (DualShock 4 USB)** output modes, pass **accelerometer** (and **gyro** when available) from modern input controllers into the emulated report so titles that use tilt / gyro respond correctly. **Switch** output mode does **not** pass through motion for now (Pro report IMU bytes stay zero).

**Important — console wiring (PS3/PS4):** OGX-Mini is a **USB gadget** (it emulates a controller to a **host**). It does **not** replace a Brook-style dongle on its own. To play **motion games on a real PlayStation 3 or PlayStation 4**, chain:

```text
[Your input pad] → (BT or USB host on OGX-Mini) → OGX-Mini → USB → [USB adapter] → PS3 or PS4
```

**Tested adapter:** **[Brook Wingman XE 2 Converter](https://www.brookaccessories.com/products/wingman-xe2)** — validated for **PS3** Sixaxis titles with OGX-Mini in **PS3 output mode**. The same Brook adapter also works on **PS4**: set OGX-Mini to **PS4 output mode** (**Start + Left Bumper + D-pad Left**), plug OGX-Mini into the Brook, and connect the Brook to the **PS4** USB port.

| Item | Detail |
|------|--------|
| **Output modes** | **PS3** (Start + D-pad Left) — Sixaxis in the DS3 report; **PS4** (Start + LB + D-pad Left) — Brook-style accel/gyro in the DS4 report |
| **Input — Bluetooth (Pico W / Pico 2 W)** | **DualShock 4**, **DualSense**, **Switch Pro**, **Wii Remote** (accel only; **0x31** report requested when motion output is active) |
| **Input — wired USB host** | **DualShock 4**, **DualSense**, **Switch 1 Pro**, **Switch 2 Pro** (IMU parsed from full HID payload) |
| **Wii Remote hold** | Point **IR end toward the TV**; roll/pitch tuned for that orientation |
| **Wii Motion Plus gyro** | Not yet parsed from extension reports — accel only from Wii Remote |
| **PC / emulator testing** | Motion is also present when OGX-Mini is in PS3/PS4 mode on a **PC** (no Brook required) |

### How it works

1. **`Gamepad::PadIn`** — `accel[3]`, `gyro[3]`, and `motion_source` tag the active IMU (`MOTION_SRC_DS4`, `DS5`, `SWITCH_PRO`, `SWITCH_USB`, `DS4_USB`, `DS5_USB`, `WII_BT`, etc.).
2. **`MotionImu.h`** — Unit conversion for USB Switch/DS4/DS5 payloads; **`remap_to_ds4_playing_frame()`** rotates Switch Pro and Wii BT samples into the DS4/Sixaxis playing frame (+Z ≈ 1 G when level).
3. **`PS3.cpp`** — `apply_pad_imu_to_ps3_sixaxis()` encodes accel into Linux **hid-sony** Sixaxis layout (X @ 41–42, Y/Z swapped per kernel); **Wii** uses a dedicated pitch/gravity wire assignment.
4. **`PS4.cpp`** — Brook-style accel/gyro scaling into the 64-byte DS4 gadget report when `has_motion()`.
5. **`MotionOutputActive`** — PS3/PS4 drivers set a flag so Bluepad32 can request Wii accel reports without `WII_MODE_ACCEL` (that mode breaks button layouts).
6. **`Bluepad32.cpp`** — Maps DS4/DS5/Switch/Wii motion from Bluepad reports; on connect requests **continuous** Wii **0x31** (DRM_KA) or **0x35** (DRM_KAE with Nunchuk) when motion output is active. Nunchuk stick → **left stick**. `uni_hid_parser_wii.c` implements **DRM_KAE** parsing.

**Files:** `src/Gamepad/MotionImu.h`, `src/Gamepad/Gamepad.h`, `src/USBDevice/DeviceDriver/PS3/PS3.cpp`, `src/USBDevice/DeviceDriver/PS4/PS4.cpp`, `src/USBDevice/DeviceDriver/MotionOutputActive.h`, `src/USBDevice/DeviceManager.cpp`, `src/Bluepad32/Bluepad32.cpp`, `src/USBHost/HostDriver/SwitchPro/SwitchPro.cpp`, `src/USBHost/HostDriver/SwitchPro/Switch2ProHost.cpp`, `src/USBHost/HostDriver/PS4/PS4.cpp`, `src/USBHost/HostDriver/PS5/PS5.cpp`.

**User docs:** [README — Motion controls](../../../README.md#ps3--ps4-motion-controls).

**Button mappings (all modes):** [Controller_Mappings.md](Controller_Mappings.md)

---

## Steam Controller 2026 (Triton) — Bluetooth

**Goal:** Pair the **Steam Controller 2026** (Valve “Triton”) over **Bluetooth LE** to **Pico W**, **Pico 2 W**, and **RP2354** (Pi Radio Module 2) adapters and use it as a normal gamepad input in any USB or GPIO output mode.

| Variant | VID:PID | Role |
|---------|---------|------|
| Body USB | `28de:1302` | Wired USB (host stack — not this feature) |
| Body BLE | `28de:1303` | **Wireless input target** |
| Puck dongle | `28de:1304` | Valve RF puck (PID registered) |
| Nereid | `28de:1305` | Related PID (registered) |

**Advertisement / services (validated on Linux):** Appearance **`0x03c4`** (gamepad), HID UUID **`00001812`**, Valve vendor service **`100f6c32-…`**, Modalias **`usb:v28DEp1303`**. Address type is typically **random**.

Unlike the original Steam Controller (custom GATT only), Triton also exposes **standard HID-over-GATT**. Main input report ID **`0x45`** (`TritonMTUNoQuat_t`: buttons, hall triggers, dual sticks, trackpads). Feature report **ID 1** + message **`0x87`** disables **lizard mode** (desktop mouse/keyboard), refreshed about every **3 s**. Rumble uses output report **`0x80`** (`MsgHapticRumble`) with a **~40 ms** resend while active.

### Pairing (user)

1. **Remove** the controller from any PC/phone Bluetooth list and make sure it is **disconnected** (BLE is one host at a time).
2. Put OGX-Mini in pairing mode (LED blinking).
3. On the controller, hold **RB + B + Steam** until it advertises.
4. Wait for connection rumble / solid LED. **Do not** pair the pad in Windows/macOS/phone Bluetooth settings for OGX use.

### Changes

1. **Bluepad32 parser** — `uni_hid_parser_steam_triton.c` / `.h`: decode **`0x45`** / **`0x42`** / **`0x47`** state reports, battery **`0x43`**, lizard-off + rumble, teardown of timers on disconnect.
2. **Controller type** — `k_eControllerType_SteamControllerTriton` (**61**); PIDs **`0x1302`–`0x1305`** in `uni_controller_list.h`.
3. **LE Secure Connections** — Triton rejects legacy SMP with **`AUTH_REQUIREMENTS_MISMATCH`**. Enabled **`ENABLE_LE_SECURE_CONNECTIONS`** in `Firmware/RP2040/src/btstack_config.h`. Non–Xbox / non–Switch-2 BLE pads pair with **`SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING`** (`uni_bt_le.c`); one Just-Works retry if AuthReq still mismatches.
4. **Connection interval** — On setup, request **6–9 × 1.25 ms (~7.5–11 ms)** like Xbox BLE (default intervals feel very laggy).
5. **Button map** — Face / LB/RB / L3/R3 / d-pad / triggers as normal. Triton **View → Start**, **Menu → Back/Select** (matches SDL; printed names are not Xbox View/Menu). **Steam** (and QAM) → **Guide / SYS**. Grip paddles L4/L5/R4/R5 left unmapped.
6. **OGX path** — Rumble routed in `Bluepad32.cpp`; stall-watchdog exemption so idle HID does not force-disconnect.

**References:** [SDL3 `SDL_hidapi_steam_triton.c`](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_steam_triton.c), [`controller_structs.h`](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/steam/controller_structs.h).

**Files:** `uni_hid_parser_steam_triton.c` / `.h`, `btstack_config.h`, `uni_bt_le.c`, `uni_controller_type.h`, `uni_controller_list.h`, `uni_hid_device.c`, `uni_gamepad.c`, `Bluepad32.cpp`.

---

## Nintendo Switch 2 — Bluetooth (Pico W / Pico 2 W)

**Goal:** Use **Nintendo Switch 2** controllers as **wireless input** on **Pico W** and **Pico 2 W**, in any USB or GPIO output mode (e.g. PS3, XInput, OG Xbox, Switch Pro emulation).

**Confirmed wireless (BLE):**

| Controller | Product ID | Notes |
|------------|------------|--------|
| **Switch 2 Pro** | **0x2069** | Full gamepad; same 63-byte composed report path as Joy-Con 2. |
| **Joy-Con 2 (L)** | **0x2067** | Solo or **merged pair** with Joy-Con R → one player. |
| **Joy-Con 2 (R)** | **0x2066** | Solo or merged with Joy-Con L (pair L first, then SYNC R). |

**Not yet confirmed:** **NSO GameCube** (**0x2073**) — PID registered; GC-style report path exists in the parser.

Switch 2 controllers use a **proprietary BLE GATT protocol**, not standard HID-over-GATT. They advertise as **“Nintendo Switch”** with manufacturer data and require a **custom pairing / SYNC** command sequence before encrypted input notifications arrive.

**References (protocol reverse engineering — thank you to these projects):**

| Project | Contribution |
|---------|----------------|
| **[Nadeflore/switch2-controllers](https://github.com/Nadeflore/switch2-controllers)** | Foundational Switch 2 **BLE discovery, pairing, and GATT** flow for Joy-Con 2 / Pro 2 on PC. |
| **[TommyWabg/switch2-controllers-windows10-gyro](https://github.com/TommyWabg/switch2-controllers-windows10-gyro)** | Extended fork with **63-byte composed input report** layout (button dword @ offset 4, sticks @ 10/13, IMU @ 48), **init / feature / rumble** command formats, and **Pro Controller** rumble packet structure. |
| **[BlueRetro #1249](https://github.com/darthcloud/BlueRetro/issues/1249)** (darthcloud) | Early community notes on **63-byte reports**, **notify handle 0x000A**, and Switch 2 **button bitfield** layout. |
| **`Switch2ProHost.cpp`** (this repo) | **Wired USB** bit layout for PID **0x2069** — cross-check for face / d-pad / **Home → SYS** mapping on PS3-oriented builds. |

### Changes

1. **New Bluepad32 parser** — `Firmware/external/bluepad32/.../parser/uni_hid_parser_switch2.c` (+ `.h`): GATT service discovery, **SYNC / pair** subcommands, calibration read, **input notify** on handle **0x0A**, **command** write/notify, **Pro** and **Joy-Con** vibration characteristics (keepalive rumble), connection parameter update (~7.5 ms), and teardown on disconnect. Supports PIDs **0x2069** (Pro), **0x2066** (Joy-Con R), **0x2067** (Joy-Con L).

2. **LE advertisement hook** — `uni_bt_le.c` detects Switch 2 manufacturer data and routes connect to the Switch 2 parser instead of generic BLE HID.

3. **Input decode** — **63-byte** notify: little-endian **button dword** at offset **4** (masks verified on hardware). Face, d-pad, ±, L3/R3, shoulders, ZL/ZR digital, **Home** (`0x100000` / `0x1000`), and **Capture** mapped into Bluepad32 `uni_gamepad_t`. Sticks calibrated from factory data; gyro/accel forwarded when present.

4. **Home → PS / Guide** — Home sets `MISC_BUTTON_SYSTEM` → `MAP_BUTTON_SYS` in `Bluepad32.cpp` → PS3 / XInput / etc. **Home latch** (~24 frames) in the parser so Nintendo’s often **single-frame** Home pulse is not dropped by the lock-free Core1→Core0 pad staging buffer before the PS3 **8-frame SYS latch** runs.

5. **Link stability** — Periodic **rumble keepalive** (zero-amplitude vibration write every ~5–8 ms) prevents **HCI disconnect (0x08)** when the pad would otherwise go idle without output reports.

6. **OGX integration** — `Bluepad32.cpp`: rumble routing, motion source `MOTION_SRC_SWITCH_PRO`, stall-watchdog exemption, keepalive timer for Switch 2 BLE devices.

7. **Joy-Con 2 pair merge** — When **Left** and **Right** Joy-Con 2 are both connected, input is **merged into one gamepad** (player slot = lower BT index, typically player 1). Left half: left stick, L/ZL, d-pad; right half: face buttons, right stick, R/ZR. Solo Joy-Con keeps BLE scan on for the partner; disconnecting one half unpairs and the other continues solo.

8. **Switch 1 Joy-Con pair merge** — Original **Joy-Con (L/R)** over **Classic Bluetooth** (PIDs **0x2006** / **0x2007**) use the same **L first, then R** flow in `uni_hid_parser_switch.c`. When paired, each half uses **Pro-style** stick/button layout before merge; solo Joy-Con keeps **BR/EDR inquiry** running for the partner. Requires **two** Bluepad32 device slots (`CONFIG_BLUEPAD32_MAX_DEVICES=2`) even on single-player USB builds.

9. **Joy-Con pair latency (Switch 1 + Switch 2)** — When **both** halves send input at once, merged pairs no longer feel laggy or drop button edges. See [§ Joy-Con pair merge — latency when both halves are active](#joy-con-pair-merge--latency-when-both-halves-are-active) below.

**Pairing (user):** Put the **Pro 2** or **Joy-Con 2** in pairing mode (SYNC / hold the pairing button per Nintendo instructions). The adapter scans and connects; **do not** pair the pad in Windows/macOS Bluetooth settings first.

**Joy-Con 2 as one controller (like Switch):** Pair the **Left** Joy-Con first, then put the **Right** Joy-Con in SYNC mode while the left is still connected. The firmware **merges** both halves into **one player** (gamepad slot **1** / the lower BT slot index). Left stick, d-pad, L/ZL map from the left Joy-Con; face buttons, right stick, R/ZR from the right. If only one Joy-Con is connected, it works solo and BLE scan stays on so you can add the partner.

**Switch 1 Joy-Con (same user flow):** Pair **Left** first (hold **SYNC** on the rail), then **Right** while left stays connected. Uses Classic BT inquiry instead of BLE scan. Solo horizontal Joy-Con layout applies until the pair is formed; merged input uses a full gamepad layout. **Wired USB** for Switch 2–family pads remains in **Switch2ProHost** / **SwitchProHost** — see [Wired_Controllers.md](Wired_Controllers.md).

**Files:** `uni_hid_parser_switch2.c`, `uni_hid_parser_switch2.h`, `uni_hid_parser_switch.c`, `uni_hid_parser_switch.h`, `uni_bt_le.c`, `uni_bt_bredr.c`, `uni_hid_device.c`, `uni_controller_list.h`, `src/Bluepad32/Bluepad32.cpp`.

---

## Joy-Con pair merge — latency when both halves are active

**Problem:** With **Left + Right** Joy-Cons merged into one player (Switch 1 Classic BT or Switch 2 BLE), pressing buttons on **both** halves at the same time could feel **laggy** or miss rapid edges. Switch 1 Pro / solo Joy-Con and Switch 2 Pro were unaffected.

**Root causes:**

1. **Stale merge** — Each half’s report was merged using the partner’s last parsed `controller.gamepad`, which could be **one frame behind** when L and R updated in the same interval.
2. **Dropped Core1→Core0 reports** — Bluetooth HID runs on **Core1**; a **single** lock-free staging slot could be **overwritten** before Core0 drained it when L and R fired back-to-back.
3. **Switch 2 radio / BT-thread load** — Dual BLE links plus **keepalive vibration writes** and **verbose raw-input logging** on button changes competed with inbound notifications on the shared CYW43439 radio.

### Changes

#### Shared (Switch 1 + Switch 2 parsers + Gamepad)

1. **Per-half cached state** — Each Joy-Con slot stores its latest parsed `uni_gamepad_t` in a half cache (`sw1_joycon_half_gp` / `sw2_joycon_half_gp`). On any report from either half, merge reads **both caches** and emits **once** through the **primary** (left) device slot only.
2. **2-slot Bluetooth pad staging** — `Gamepad::set_pad_in_from_bluetooth()` uses a **two-entry ring** instead of one overwrite buffer, so rapid L-then-R reports are less likely to lose a frame before Core0 merges them into the main pad-in queue. **File:** `src/Gamepad/Gamepad.h`.

#### Switch 1 (Classic Bluetooth)

3. **IMU off when paired** — After L+R pair is established, both halves receive **SUBCMD_ENABLE_IMU (0)** so 0x30 reports carry less motion traffic and parsing skips IMU while paired. **File:** `uni_hid_parser_switch.c` — `switch_set_imu_enabled()`, `switch_establish_joycon_pair()`.

#### Switch 2 (BLE)

4. **Deferred merge emit** — When paired, L/R input notifications **update the half cache immediately** but schedule a **0 ms** run-loop timer to merge and call `uni_hid_device_process_controller()` **once** per BT tick if both halves updated in the same loop iteration.
5. **Keepalive traffic reduced** — Removed **keepalive on every input notification** (timer already maintains the link). **Secondary** Joy-Con **stops** its 5 ms keepalive timer when paired; **primary** alternates idle rumble between L and R every **10 ms** (one GATT write per tick). Keepalive is **deferred briefly** while either half is actively reporting. **Bluepad32** skips redundant feedback-path keepalive when the parser timer is already running.
6. **Joy-Con–specific parse** — Joy-Con 2 L/R parse **only their physical stick** (not both sticks + IMU on every 63-byte packet). Raw hex input logging on button change is **disabled** by default (`SW2_DEBUG_RAW_INPUT=0`) so UART work does not stall the BT thread during gameplay.
7. **Connection interval** — **7.5 ms** connection parameter update is requested on **both** halves when the pair is formed.

**Files:** `uni_hid_parser_switch.c`, `uni_hid_parser_switch2.c`, `uni_hid_parser_switch2.h`, `src/Gamepad/Gamepad.h`, `src/Bluepad32/Bluepad32.cpp`.

**User impact:** Paired Joy-Cons (Switch 1 or Switch 2) should feel as responsive when using **both** halves simultaneously as when using one half alone. Pairing flow unchanged: **Left first**, then **Right** (SYNC on the partner while left stays connected).

---

## PIO USB host — wired connection fixes (Waveshare RP2350-USB-A)

**Primary test platform:** **Waveshare RP2350-USB-A** (`OGXM_BOARD=RP2350_USB_A`, build option **7** in `scripts/build.sh`). Firmware uses the **Standard** path: **Core1** runs **`tuh_task()`** plus a **1 ms SOF timer** for PIO USB host.

**Scope:** Changes live in **shared USB host drivers** and **`HostManager`**, not board-specific `#ifdef`s. Any build with **`CONFIG_EN_USB_HOST`** (Pico, Feather, RP2350-Zero, RP2350-USB-A, etc.) gets the same wired-host behavior. **Pico W / Pico 2 W Bluetooth** (Bluepad32) is **unchanged**.

**Goal:** Reliable **wired USB host** input on PIO USB adapters for **Switch Pro 1/2**, **DualShock 3**, the **Xbox 360 wireless PC receiver** + pad, and **Xbox One GIP arcade sticks** (e.g. **Razer Atrox XBO**) — without starving the PIO stack during init or OUT transfers.

### Shared — HostManager and PIO USB servicing

1. **`pio_usb_host_frame()` on all PIO USB host boards** — Previously gated to **Pico W only**; now runs after **`send_feedback()`** / nested **`tuh_task()`** on **every** `CONFIG_EN_USB_HOST` build so wired IN reports do not die after ~1–2 s when the host sends OUT.
2. **Periodic feedback for init-heavy HID drivers** — **`HostManager::send_feedback()`** now always invokes **`send_feedback()`** for **PS3** and **Switch Pro / Switch 2 Pro** (not only on rumble / `new_pad_out()`), so wired init steps and keepalives run on a timer.
3. **`disconnect_cb()` on unmount** — Drivers get a clean teardown hook when a device is removed (bulk cancel for Switch 2, state reset for 360 wireless, etc.).
4. **XInput instance storage** — **`MAX_INTERFACES = MAX_GAMEPADS + OGXM_TUH_XINPUT_INSTANCES`**; **`OGXM_TUH_XINPUT_INSTANCES = 4`** when `MAX_GAMEPADS < 4` so the **360 wireless receiver’s four ports** each have a TinyUSB slot even on single-player builds.

**Files:** `HostManager.h`, `Board/Config.h`.

### Nintendo Switch Pro — wired USB

**References:**

| Project | Contribution |
|---------|----------------|
| **[Chromium `device/gamepad`](https://chromium.googlesource.com/chromium/src/+/main/device/gamepad/)** | **Switch 1 Pro wired USB** init: report **0x80** subcommands (MAC, handshake, baud, disable timeout), **0x81** ack handling, **0x12** output + subcommands for LED / mode / IMU. |
| **`Tools/controller_capture/switch2_usb_init.py`** (this repo) | Captured **Switch 2–family bulk OUT** packet sequence used on PC before HID reports work. |
| **[HandHeldLegend/procon2tool](https://github.com/HandHeldLegend/procon2tool)** | **ProCon 2 enabler** — same bulk bring-up pattern as the capture tool; cross-check for **17-packet** interface **1** sequence. |
| **Bluepad32 `uni_hid_parser_switch.c`** | **Switch 1** button bit layout (`Buttons0/1/2`) for wired report decode validation. |
| **`Switch2ProHost.cpp`** (v1.0.0.9a) | **Switch 2 Pro wired** digitals from report **0x09** — extended here with bulk-only bring-up (no post-bulk Switch 1 init). |

### Switch 1 Pro (PID 0x2009) — wired USB init

1. **Wired USB state machine** — `InitState` phases: **USB_MAC** → **USB_HANDSHAKE1** → **USB_BAUD** → **USB_HANDSHAKE2** → **USB_NO_TIMEOUT** → **LED** → **LED_HOME** → **FULL_REPORT** → **IMU** → **DONE**.
2. **0x80 path** — OUT on report ID **`SwitchPro::REPORT_ID_USB_OUT` (0x80)** with subcommands **`USB_SUB_MAC`**, **`USB_SUB_HANDSHAKE`**, **`USB_SUB_BAUD`**, **`USB_SUB_DISABLE_TIMEOUT` (0x04)**; advance on matching **0x81** IN ack bytes.
3. **0x12 + 0x33 probe** — After disable-timeout, send **`CMD::AND_RUMBLE` (0x12)** with subcommand **`USB_PROBE` (0x33)**; on **0x21** subcommand ack, continue to player LED and **full report mode** subcommands.
4. **PIO-friendly OUT** — **`try_hid_out_id()`** checks **`tuh_hid_send_ready()`**; **`send_feedback()`** calls **`pio_usb_host_frame()`** + **`tuh_task()`** during init so Core1 keeps the PIO host alive.
5. **Per-IN advancement** — Init steps also run from **`process_report()`** on each IN packet, not only from the feedback timer.
6. **Timeout fallback** — If **USB_NO_TIMEOUT** ack is slow, **8 retries** in **`send_feedback()`** force advance to **LED** so init does not hang indefinitely.
7. **Input mapping fixes** — D-pad **UP↔DOWN** and **LEFT↔RIGHT** swapped; **L3↔R3** swapped in **`SwitchProHost::process_report()`**.

### Switch 2 Pro (PID 0x2069) and Switch 2–family USB — bulk bring-up

Applies to Nintendo VID **0x057E** PIDs **0x2066**, **0x2067**, **0x2069**, **0x2073** (`is_switch2_usb_family()`).

1. **Bulk sequence** — Read configuration descriptor, open **interface 1** bulk IN/OUT, send **17 packets** from **`Switch2UsbInitPackets.h`** (~**12 ms** spacing between rounds), optional bulk IN read per round when an IN endpoint exists.
2. **No post-bulk Switch 1 init** — On successful bulk completion, **`init_state_ = DONE`** immediately and **`tuh_hid_receive_report()`** starts — **Switch 1 `0x80` HID init is skipped** for Switch 2–family devices.
3. **Switch2ProHost** — Parses **64-byte** HID report ID **0x09** (1-byte counter + **10-byte** payload) with **Switch 2–specific** button bits (see v1.0.0.9a / [Wired_Controllers.md](Wired_Controllers.md)).
4. **Async safety** — Bulk callbacks use **`switch2_make_cb_token(daddr, instance)`** and **`HostManager::get_switch_pro_host()`** to look up the live driver; safe after hot-swap or unplug.
5. **Teardown** — **`disconnect_cb()`** / destructor call **`switch2_cancel_bringup()`**: abort in-flight bulk transfers, clear pending retry state.

### DualShock 3 — wired USB host

**Problem:** **DualShock 3** over **USB cable** failed to connect or dropped shortly after plug-in on **PIO USB host** boards. Init used the **USB gadget** default OUT template (leading **`0x01`**) and **async-only** control transfers without servicing **`pio_usb_host_frame()`**, so enable/LED commands never completed reliably.

**Reference:** [USB Host Shield PS3](https://github.com/felis/USB_Host_Shield_2.0) host report buffer / feature **0xF4** enable sequence.

#### Changes

1. **Host OUT report layout** — **`init_host_out_report()`** builds the **USB Host Shield `PS3_REPORT_BUFFER`** layout (player LED bitmap **`0x02 << player`**, per-LED timing fields) instead of copying gadget **`DEFAULT_OUT_REPORT`**.
2. **Synchronous wired init** — On plug-in: **SET feature 0xF4** `{0x42, 0x0c, 0x00, 0x00}` then **synchronous OUT** (rumble/LED report) via **`send_control_xfer_wait()`**, polling **`pio_usb_host_frame()` + `tuh_task()`** up to **150 ms** per step.
3. **Immediate input** — Wired path sets **`reports_enabled`** after enable + LED OUT (and BT **0xF5** when applicable); **`tuh_hid_receive_report()`** starts without the old multi-stage **GET feature 0xF2** chain.
4. **Bluetooth auto-pair (BT boards)** — After **0xF4** + LED, **`program_ds3_bt_host()`** sync-sends **feature 0xF5** with the adapter BD_ADDR (see section below).
5. **PIO keepalive** — **`KEEPALIVE_MS = 1000`** on USB host sends periodic neutral OUT so the DS3 stays connected when idle; **`HostManager`** includes PS3 in periodic **`send_feedback()`**.
6. **Async OUT for rumble** — Runtime rumble/keepalive uses non-blocking **`send_control_output_async()`** so feedback does not stall IN.

**File:** `src/USBHost/HostDriver/PS3/PS3.cpp`, `PS3.h`.

**User testing (Waveshare RP2350-USB-A):** **DualShock 3 wired** connects and holds input after flash.

### Xbox 360 wireless receiver — wired USB host

**Problem:** **Microsoft Xbox 360 wireless PC receiver** (`045e:0719`) plugged into the adapter’s **PIO USB host** port often failed to **sync** a wireless pad or delivered **no input** when **`MAX_GAMEPADS=1`**. Only one of four receiver interfaces was stored; **controller-present** was mis-detected (**headset `0x40`** treated as paired); connect path **slept 1 s** and blocked PIO servicing; **`Xbox360WHost`** assumed a fixed struct layout for variable-length wireless reports.

#### Changes

1. **Four XInput instances on single-player builds** — **`OGXM_TUH_XINPUT_INSTANCES = 4`** when `MAX_GAMEPADS < 4`; **`MAX_INTERFACES = MAX_GAMEPADS + OGXM_TUH_XINPUT_INSTANCES`**; **`host_storage_index()`** maps XInput instances to **`MAX_GAMEPADS + instance`**.
2. **One driver, four ports** — **`HostManager`** treats sibling **`XBOX360W`** interfaces as sharing one gamepad slot when `MAX_GAMEPADS=1`; **`process_report` / `connect_cb` / `disconnect_cb`** fall back to the mounted **`Xbox360WHost`** when the per-instance slot has no driver pointer.
3. **`tuh_xinput` pairing** — **`prime_port_for_pairing()`** on each idle port: **RUMBLE_ENABLE + player LED** (quadrant from interface number **0/2/4/6**); called at **set_config** and periodically via **`service_wireless_ports()`** from **`HostManager::send_feedback()`**.
4. **Connect detection** — Controller present when **IN byte1 & 0x80** (not merely non-zero / headset **0x40**); on connect: **RUMBLE_ENABLE**, **`wait_for_tx_complete()`** (200 ms cap), **LED** — **no 1 s sleep**.
5. **Input reports** — Accept **report size 0x13 or 0x14**; **`Xbox360WHost::process_report()`** decodes **button word, triggers, sticks** from raw byte offsets; tracks **`active_instance_`** for rumble on the port that last connected.
6. **Rumble** — **`set_rumble()`** skips **RUMBLE_ENABLE** when already connected; returns false if port not connected (avoids spurious OUT on idle RF slots).

**Files:** `Board/Config.h`, `HostManager.h`, `HostDriver/XInput/tuh_xinput/tuh_xinput.cpp`, `HostDriver/XInput/Xbox360W.cpp`.

**User testing (Waveshare RP2350-USB-A):** **360 wireless receiver + pad** sync and input confirmed over PIO USB host.

### Razer Atrox Xbox One — GIP arcade stick (wired USB host)

**Problem:** **Razer Atrox Arcade Stick** for **Xbox One** (`1532:0a00`) enumerates as a **vendor-class GIP** device (not standard Microsoft `0x47/0xD0` XInput). On PIO USB host it would **mount** (adapter LED on) but the stick **lit briefly then powered off** with **no inputs** — or **flashed on/off** when the host retried **POWER_ON** too often.

**References:**

| Project | Contribution |
|---------|----------------|
| **[OOPMan/XBOFS.win](https://github.com/OOPMan/XBOFS.win)** | Atrox-specific **GIP init** (`05 20 00 01 00`), **30-byte** IN reads, input layout (face/LT/RT on **byte 22**). |
| **Linux `xpad`** | Generic Xbox One **POWER_ON** packet and **64-byte** IN for standard gamepads; **MAP_TRIGGERS_TO_BUTTONS** quirk for `1532:0a00` in the device table. |

#### Changes

1. **Vendor GIP detection** — **`tuh_xinput::open()`** claims interface **0** for known XBO arcade sticks (`1532:0a00`, Mad Catz TE2, PDP, Hori, etc.) via **`XboxArcadeStick.h`** VID/PID table.
2. **Arcade init path (XBOFS)** — **`start_xboxone()`** (deferred **50 ms** after mount from **`XboxOneHost::initialize()`**): send **POWER_ON** once, **`wait_for_tx_complete()`** on OUT with **`pio_usb_host_frame()`**, then arm **30-byte** IN polling. Standard Xbox One pads keep **IN-first** + full **`xboxone_init()`** with **64-byte** reads.
3. **Input decode** — **`XboxOneHost`** uses **XBOFS byte layout** for arcade VID/PIDs; standard pads use **`GipWireButtons`** / **`InReport`** layout.
4. **PIO keepalive** — **`service_gip()`** from **`HostManager::send_feedback()`** re-arms stalled IN every **250 ms** for arcade sticks only. **Does not** repeat **POWER_ON** (repeated power commands caused visible **on/off flashing**).
5. **Xbox 360 Atrox** (`24c6:5000`) — unchanged: standard **XInput** with **digital trigger** bytes in **`Xbox360Host`**.

**Files:** `src/USBHost/HostDriver/XInput/XboxArcadeStick.h`, `tuh_xinput/tuh_xinput.cpp`, `tuh_xinput/tuh_xinput.h`, `XboxOne.cpp`, `XboxOne.h`, `HostManager.h`.

**User testing (Waveshare RP2350-USB-A):** **Razer Atrox Xbox One** (`1532:0a00`) — connect, stay powered, and full button input confirmed.

### Hot-swap (Switch 1 ↔ Switch 2 Pro)

Unplug during bulk or HID init runs **`HostManager::disconnect_cb()`** → driver **`disconnect_cb()`** → bulk cancel. Plugging the other model starts a fresh init path (Switch 1 **0x80** vs Switch 2 bulk) without requiring an adapter power cycle.

### Files (all wired-host fixes)

| File | Role |
|------|------|
| `src/USBHost/HostManager.h` | PIO **`pio_usb_host_frame()`** on all host boards; PS3/Switch periodic feedback; 360W driver routing; **`disconnect_cb`** on unmount; **`get_switch_pro_host()`** |
| `src/Board/Config.h` | **`OGXM_TUH_XINPUT_INSTANCES`** (4 when `MAX_GAMEPADS < 4`) |
| `src/USBHost/HostDriver/SwitchPro/SwitchPro.cpp` | Switch 1 wired init, Switch 2 bulk bring-up, Switch 1 input mapping, PIO USB service loop |
| `src/USBHost/HostDriver/SwitchPro/SwitchPro.h` | Init state machine, bulk bring-up members, `disconnect_cb` |
| `src/USBHost/HostDriver/SwitchPro/Switch2ProHost.cpp` | Switch 2 Pro wired button decode |
| `src/USBHost/HostDriver/SwitchPro/Switch2UsbInitPackets.h` | 17 bulk OUT packets (capture tool / HHL) |
| `src/Descriptors/SwitchPro.h` | Wired USB constants (`REPORT_ID_USB_OUT`, `USB_SUB_*`, `USB_PROBE`) |
| `src/USBHost/HostDriver/PS3/PS3.cpp` | DualShock 3 wired init, sync control xfer, 1 Hz keepalive |
| `src/USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.cpp` | 360 wireless receiver port priming, connect detect, **`service_wireless_ports()`**, **`start_xboxone()`**, **`service_gip()`** |
| `src/USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h` | GIP arcade interface state (`gip_power_sent`, IN stall tracking) |
| `src/USBHost/HostDriver/XInput/XboxArcadeStick.h` | XBO arcade VID/PIDs, **30-byte** xfer size, XBOFS report parser |
| `src/USBHost/HostDriver/XInput/XboxOne.cpp` | Arcade vs standard GIP decode; deferred **`start_xboxone()`** |
| `src/USBHost/HostDriver/XInput/Xbox360W.cpp` | Wireless input decode from byte offsets, active instance rumble |

**Build:** `./scripts/build.sh` → board **RP2350_USB_A**; typical **`-DMAX_GAMEPADS=1`**. Optional **`-DOGXM_SWITCH2_HID_RAW_LOG=ON`** for UART hex when Switch 2 Pro digital button bytes change.

**User testing (Waveshare RP2350-USB-A):** **Switch 2 Pro wired**, **DualShock 3 wired**, **Xbox 360 wireless receiver + pad**, and **Razer Atrox Xbox One** (`1532:0a00`) confirmed on PIO USB host. **Switch 1 Pro wired** — init and mapping updated; retest after flash if a prior S2 regression was seen.

---

## Xbox 360 (XInput) support

**Goal:** Use the adapter in XInput mode on Xbox 360 with Bluetooth controllers (e.g. PS5, Xbox One). The 360 requires XSM3 authentication and specific USB descriptors.

**References:** [joypad-os](https://github.com/joypad-ai/joypad-os) XInput implementation; [libxsm3](https://github.com/InvoxiPlayGames/libxsm3).

### Changes

1. **Descriptors**
   - Device and configuration descriptors aligned with joypad-os (153-byte config, 4 interfaces, bConfigurationValue 1, bMaxPower 0xFA).
   - Configuration callback returns `nullptr` for `index != 0` (single config).
   - String descriptor index 4 (XSM3 security) uses a 96-character buffer and the full string (no 31-char truncation).

2. **XSM3 authentication**
   - XSM3 state is initialized at driver init (not in the 0x81 callback).
   - Challenge init (0x82) and verify (0x87) data are stored when received; crypto (`xsm3_do_challenge_init` / `xsm3_do_challenge_verify`) runs in the main loop (`process()`), not in the USB callback.
   - 0x83 responses: 46 bytes for init, 22 bytes for verify.
   - 0x86 state: 1 = processing, 2 = response ready.

3. **USB on Pico 2 W**
   - USB is initialized before Core1 (Bluetooth) so the 360 can enumerate and run XSM3 even if BT firmware is still loading.

4. **Control handling**
   - Vendor and class control requests are forwarded to the active driver so XSM3 traffic reaches the XInput handler.

5. **Wake console from standby (remote wakeup)**
   - When the Xbox 360 has been turned off via **Guide → Turn off console** (not the front power button), the console may keep USB power and put the bus in suspend. The adapter can then wake the console by signaling USB remote wakeup when you:
     - **Press Guide (Home)** on the controller, or
     - **Hold Start for 3 seconds** (avoids holding Guide on Xbox One/PS5 pads, which can turn the controller off).
   - Remote wakeup is advertised in the configuration descriptor; the stack defaults it enabled when supported so wake works even if the host did not send SET_FEATURE before standby.
   - **Disclaimer:** Wake only works when the console was previously powered on with the adapter connected and then turned off via the controller (soft shutdown). It **cannot** power on the console from a cold start—e.g. if the console was just plugged in, lost power completely, or was turned off with the front power button. In those cases use the console’s power button to turn it on first.

Descriptors and XSM3 flow are aligned with [joypad-os](https://github.com/joypad-ai/joypad-os); a local comparison doc may be kept out of the repo for reference.

---

## PS3 mode — input delays, stuck inputs, Home button, and analog stick emulation

**Source:** Fixes from [OGX-Mini-Plus](https://github.com/guimaraf/OGX-Mini-Plus) (v1.1.1) — *“PS3 Driver Fixes - Fixed input delays and stuck inputs.”* Plus subsequent improvements for Home (PS) button and DS3-accurate analog sticks.

**Files:** `src/USBDevice/DeviceDriver/PS3/PS3.cpp`, `src/Descriptors/PS3.h`

### Changes

1. **L2/R2 axis values**
   - `report_in_.l2_axis` and `report_in_.r2_axis` are now set from `gp_in.trigger_l` and `gp_in.trigger_r`.
   - Previously left at zero, which could cause stuck or incorrect trigger behaviour on PS3. Filling these is required for many PS3 games.

2. **DualShock 3–accurate analog sticks**
   - Sticks now match the real DS3/Sixaxis HID spec:
     - **Range:** 0–255 (full 8-bit; was 0–254).
     - **Center:** **0x80 (128)** at rest and in deadzone (was 0x7F). Matches Linux gamepad spec and HID logical max 255.
     - **Deadzone:** ~1.5% (512 on ±32768) so small movements register without drift; in deadzone the report sends 0x80.
     - **Scaling:** Linear map from signed 16-bit input to 0–255 with correct rounding so center (0) → 128.
   - Reduces stick drift and matches console expectations for DS3-compatible games.

3. **D-pad in analog mode**
   - When `gamepad.analog_enabled()` is true, D-pad axes (`up_axis`, `down_axis`, `left_axis`, `right_axis`) are now derived from the **digital** D-pad bits instead of `gp_in.analog[ANALOG_OFF_*]`.
   - Prevents noisy analog D-pad values from causing stuck or wrong D-pad input on PS3.

4. **Face button axes (digital / non-analog branch)**
   - Corrected mapping so that:
     - `circle_axis` = BUTTON_B (was BUTTON_X)
     - `cross_axis` = BUTTON_A (was BUTTON_B)
     - `square_axis` = BUTTON_X (was BUTTON_A)
   - Circle / Cross / Square now match the intended face buttons.

5. **Home (PS) button**
   - Report is built only when `tud_hid_ready()` (right before send), using `gamepad.get_pad_in()` so the console gets the latest state (helps with DS4/DS5 over Bluetooth).
   - **PS button latch:** When `BUTTON_SYS` is set, the driver latches the PS bit for 8 consecutive report frames so short taps are not missed by timing. `buttons[2]` bit 0 (PS) and bit 1 (Touchpad) are set from `BUTTON_SYS` and `BUTTON_MISC`.
   - If Home does not work over Bluetooth, try a wired controller (some consoles only react to the first controller's Home).

6. **Wake console from standby (remote wakeup)**
   - The configuration descriptor now advertises **remote wakeup** (`bmAttributes` 0xA0) so the PS3 can suspend the USB bus in standby and the adapter can signal wake. When the console is in standby (turned off via **PS button → Turn off system** or similar, not full power loss), you can wake it by **pressing PS (Home)** or **holding Start for 3 seconds**—**only if the console keeps USB power when off**.
   - **Many PS3s cut power to the USB ports** when shut down, so the adapter and controller disconnect and wake is not possible. If your controller stays powered (e.g. charging LED) when the PS3 is off, that model may keep USB in standby and wake may work. Same disclaimer as 360: no wake from cold start; use the console power button first if needed.
   - **Recovery Mode:** Even when wake from standby is not possible (e.g. console cuts USB when off), the adapter is **confirmed working in PS3 Recovery Mode** — you can use it to navigate and select options in Recovery.

7. **Host rumble → Bluetooth pad (PS3 mode on PC)** *(v1.0.0.9a)*  
   When the adapter is in **PS3 output mode** and connected to a **Windows** host, the OS still sends **HID output** (rumble) to the device. That output is forwarded to the **Bluetooth** gamepad as `PadOut` rumble. Some Windows stacks leave **noise** in the report (small large-motor values, or non‑`0`/`1` small-motor bytes). Forwarding that blindly caused **constant vibration** on the wireless controller at idle.  
   **Fix:** Ignore large-motor values **below a small threshold** (deadzone), and treat the **small motor as on only when the host byte is `1`** (not “any non-zero”).  
   **File:** `PS3.cpp` — block that runs when `new_report_out_` is set after `set_report_cb` parses the DS3 output report.

8. **Sixaxis / motion passthrough** *(v1.0.0.12a)*  
   When the input gamepad provides IMU data (`has_motion()`), accel/gyro are encoded into the DS3 Sixaxis fields using Linux **hid-sony** wire layout. See [§ PS3 / PS4 output — motion passthrough](#ps3--ps4-output--motion-passthrough) for supported input pads, Wii Remote notes, and **Brook Wingman XE 2** console wiring.

---

## Latency reduction

**Goal:** Reduce input-to-output latency in the device (Core0) main loop, especially for XInput with Bluetooth controllers (PS5, Xbox One).

### Main loop delay

- **Before:** The device loop used `sleep_ms(1)` every iteration, then a configurable `MAIN_LOOP_DELAY_US` (default 250 µs).
- **After:**
  - **Default: 0 µs** — no added delay; loop runs as fast as possible for minimum latency (low-latency default).
  - **250+ µs** — set via CMake (e.g. `-DMAIN_LOOP_DELAY_US=250`) to reduce CPU use if desired.

**Files changed:**

- `src/Board/Config.h` — `MAIN_LOOP_DELAY_US` default **0**; override via CMake.
- `src/OGXMini/Board/Standard.cpp`, `PicoW.cpp`, `Four_Channel_I2C.cpp` — use `sleep_us(MAIN_LOOP_DELAY_US)` when `> 0`.
- `CMakeLists.txt` — `MAIN_LOOP_DELAY_US` cache variable (default 0).

### XInput (360): report always fresh; send when ready (minimal latency)

Same goal as Switch Pro and PS3: the only added latency when using a wireless controller is the Bluetooth radio.

- **Every** `process()` call: read `get_pad_in()` and build `in_report_` (buttons, triggers, sticks). Then, if suspended, wake; call `tud_xinput::send_report(&in_report_)`. `send_report()` only actually transmits when the IN endpoint is free (`send_report_ready()`); otherwise we keep the latest `in_report_` so that (1) the host’s `get_report_cb` returns current state if it polls, and (2) the next time the endpoint is free we send that report. No `new_pad_in()` gate.
- **File:** `src/USBDevice/DeviceDriver/XInput/XInput.cpp` — `process()` always builds `in_report_` every loop; `tud_xinput::send_report()` sends only when `send_report_ready()` (see `tud_xinput.cpp`).

### Switch Pro and PS3: report always fresh; send when ready (minimal latency)

**Goal:** For Switch Pro and PS3 output modes, the only added latency should be wireless Bluetooth (radio) when using a BT controller. The adapter does not batch, throttle, or delay reports.

- **Switch Pro:** Every `process()` call reads `get_pad_in()`, builds `switch_report_`, and builds the standard or subcommand report into `report_` **before** any USB decisions. So (1) the host’s `get_report` (poll) always gets the latest `report_`, and (2) when `tud_hid_n_ready(0)` we push that same report. Init reply (0x81) is sent first when pending, then the standard report. No “only build when ready” — report is always current.
- **PS3:** Every `process()` call reads `get_pad_in()` and builds `report_in_` (full DS3 report). Then, when `tud_hid_ready()`, we send it. So (1) the host’s `get_report_cb` always returns the latest `report_in_`, and (2) we push that report whenever the IN endpoint is free. No “only build when ready” — report is always current.

Both modes: no `new_pad_in()` gate; main loop runs with `MAIN_LOOP_DELAY_US=0` by default; `tud_task()` runs before `process()` so the endpoint is ready when we try to send.

### Xbox OG (Duke) gamepad: send only on new input (match Team-Resurgent)

Report build/send timing matches [Team-Resurgent/OGX-Mini](https://github.com/Team-Resurgent/OGX-Mini): the HID report is built and sent **only** when `gamepad.new_pad_in()` is true. Sending every poll caused random disconnects on some OG Xbox setups; the “send when new input” rule avoids that. Guide (SYS) combos are kept: **Guide only** = IGR (LT+RT+Start+Back), **Guide+Start** = shutdown (LT+RT+Back+White). Rumble handling is unchanged.

- **File:** `src/USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp` — `process()` builds `in_report_` and calls `tud_xid::send_report()` only when `new_pad_in()` and `send_report_ready(0)`.

**DualSense (PS5) input when outputting to OG Xbox:** The PS5 USB host no longer skips reports that are byte-identical to the previous one. Previously, “unchanged” reports did not call `set_pad_in()`, so with a polled output (OG Xbox) some transitions or sustained input could be dropped when the host loop was slower than the DualSense report rate. Every DualSense report is now pushed into the gamepad queue so the OG Xbox device always has the latest state. **File:** `src/USBHost/HostDriver/PS5/PS5.cpp` — removed the unchanged-report early return; every report is parsed and passed to `gamepad.set_pad_in()`.

### Main loop order: tud_task() before process()

The main loop now calls **`tud_task()` before** `device_driver->process()`. That way the USB stack updates completion status of the previous IN transfer first; then `process()` sees the endpoint as ready and can send the next report immediately with the latest gamepad state. Reduces latency by up to one main-loop iteration (avoids sending only every other loop when the host polls frequently).

**Files:** `src/OGXMini/Board/PicoW.cpp`, `Standard.cpp`, `Four_Channel_I2C.cpp` — order is `process_tasks()` → `tud_task()` → `process()` for each gamepad.

---

## PS2 (GPIO) / Open PS2 Loader stability

**Issue:** With a “primed” first response byte (0xFF) before the mode byte, Open PS2 Loader could hang at startup (black screen) when the adapter was connected.

**Fix:** The PS2 controller (device) response was reverted so the **first response byte is the mode byte** (no leading 0xFF). Protocol and escape-mode response lengths otherwise follow PicoGamepadConverter and DS4toPS2. The main loop drains all pending PS2 transactions each tick so rapid pad init (e.g. OPL at boot) does not desync. Core1 runs Bluetooth when used; Core0 runs the main loop and `psx_device_poll()` so the console sees input correctly.

**File:** `src/USBDevice/DeviceDriver/PS1PS2/controller_simulator.c` — first response byte is the mode byte; no `prime_first_byte()` / leading 0xFF.

---

## PS2 (GPIO) and OG Xbox — Home/Guide IGR and shutdown

**Goal:** Use the Home (PS2) or Guide (OG Xbox) button for in-game reset (IGR) and console shutdown with the same interaction pattern on both platforms: **Home only** = restart (IGR), **Home+Start** = shutdown.

### PS2 (GPIO) mode

**Files:** `src/USBDevice/DeviceDriver/PS1PS2/PS1PS2.cpp`, `PS1PS2.h`

- **Home only** — Sends the OPL in-game reset combo: **L1+L2+R1+R2+Start+Select** (triggers full). The console restarts the game / returns to OPL.
- **Home+Start** — Sends shutdown combo: **L1+L2+R1+R2+L3+R3** (triggers full). The console shuts down.

### OG Xbox mode

**Files:** `src/USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp`, `XboxOG_GP.h`

- **Guide only** — Sends IGR (restart) combo: **LT+RT+Start+Back** (triggers full). The console performs a soft reset / returns to dashboard (or IGR handler).
- **Guide+Start** — Sends shutdown combo: **LT+RT+Back+White** (triggers full). The console shuts down.

### How to use

| Goal | Setting |
|------|--------|
| Low latency (default) | Use default `MAIN_LOOP_DELAY_US=0`. |
| Lower CPU use | Configure with e.g. `-DMAIN_LOOP_DELAY_US=250`. |

### Notes

- Core1 (Bluetooth / gamepad) runs with no sleep in its loop.
- USB full-speed poll interval (e.g. 4 ms for XInput) still applies; the improvement is that each report carries the **latest** input and the main loop adds no extra delay by default.
- Bluetooth adds latency; the changes above minimize the adapter’s contribution.

---

## Additional board support (RP2350_ZERO, RP2040_XIAO, RP2354)

**Source:** [Sakura-Research-Lab/OGX-Mini-2026-Testing](https://github.com/Sakura-Research-Lab/OGX-Mini-2026-Testing).

Three additional boards use the same Standard (PIO-USB host) code path as PI_PICO, RP2040_ZERO, ADAFRUIT_FEATHER, and RP2350_USB_A:

| Board | CMake option | Notes |
|-------|--------------|--------|
| Waveshare RP2350-Zero | `OGXM_BOARD=RP2350_ZERO` | PIO USB D+ = GP10, RGB = GP16; RP2350. |
| Seeed Studio XIAO RP2040 | `OGXM_BOARD=RP2040_XIAO` | PIO USB D+ = GP0, RGB = GP12, LED = GP17. |
| RP2354 | `OGXM_BOARD=RP2354` | **RP2350 + Pi Radio Module 2** (CYW43439, same class as **Pico 2 W**): **Bluetooth** wireless pads + PIO USB host on **GP0/GP1**, LED **GP25**. |

**Files:** `src/Board/Config.h` (board IDs and pin defines), `src/OGXMini/OGXMini.cpp` (init/run/host_mounted tables), `src/OGXMini/Board/Standard.cpp` (extended `#if`), `CMakeLists.txt` (board branches).

---

## 8BitDo XInput (Xbox 360) host fix

**Source:** [Sakura-Research-Lab/OGX-Mini-2026-Testing](https://github.com/Sakura-Research-Lab/OGX-Mini-2026-Testing).

Some 8BitDo wired XInput controllers (VID 0x2DC8, PID 0x3016 or 0x3106) can disconnect or behave oddly if the host leaves the controller LED on. The Xbox 360 host driver now detects these devices by VID/PID and schedules a repeating delayed task (every 1 s) that sends “LED off” to the controller. Other controllers are unchanged (LED stays on as before).

**File:** `src/USBHost/HostDriver/XInput/Xbox360.cpp` — `initialize()` calls `tuh_vid_pid_get()`, and if 8BitDo, queues `TaskQueue::Core1::queue_delayed_task(..., 1000, true, set_led(..., false))`.

---

## PS5 (DualSense) over Bluetooth — reducing perceived delay vs Xbox One

**Why PS5 can feel slower than Xbox One over BT:** DualSense sends larger reports (78 bytes, with gyro/accel), and Bluepad32’s DS5 parser does more work per report (calibration, etc.). Xbox One reports are smaller and the parser is lighter. Bluetooth poll/report rate and radio latency dominate; the adapter’s job is to not add extra delay.

**Change in this firmware:** The PS5 adaptive-trigger toggle (touchpad/mute button) now runs **after** `set_pad_in(gp_in)`. Previously it ran at the start of the callback; when you pressed the touchpad it could send two output reports (left/right trigger effect) before updating gamepad state, which could delay the next main-loop read. Now the gamepad state is always written first, then the trigger effect is sent, so input is not held up by the trigger command.

**If you need minimum latency with a DualSense:** Use the controller **wired** on the PIO USB host port when possible; wired PS5 uses the same low-latency path as other USB host controllers and avoids BT report size and rate limits.

---

## Pico W / Pico 2 W — OG Xbox main-loop timing (#54)

**Problem:** With a **Pico W / Pico 2 W** adapter on **OG Xbox**, some games (**Midnight Club 3**, **Half-Life** / **Half-Life 2**) run extremely slowly whenever the adapter is plugged in — even with **no** Bluetooth controller paired. Unplugging the adapter and using a stock Duke restores normal speed. **1.0.0.6a** was fine; later builds that added Core0 `sleep_ms(1)` for Bluetooth were not.

**Cause:** Core0’s device main loop called **`sleep_ms(1)`** every iteration so Core1 / CYW43 could run. That delayed **`tud_task()`** enough to starve the **Duke USB** interrupt IN path. Timing-sensitive titles stall waiting on USB.

**Fix:** In **`pico_w::run()`** (`src/OGXMini/Board/PicoW.cpp`):

| USB device configured (`tud_mounted()`)? | Bluetooth pad connected? | Yield |
|---|---|---|
| Yes | No | `tight_loop_contents()` (no 1 ms sleep) |
| Yes | Yes | `sleep_us(250)` |
| No | — | `sleep_ms(1)` (pairing / idle OK) |

Removing sleep entirely while mounted fixed games but increased BT disconnects (**OGXBoxSlownessFix**). Restoring a long Core0 yield (**Issue54Test / Test2**) brought the slowdown back. The **250 µs** path is the middle ground.

**Test:** MC3 / HL speed with adapter plugged in (paired and unpaired); Series 1914 (or other) disconnect rate vs older “slowness fix” builds.

---

## Xbox One / Series — Guide press/release (#27)

**Problem:** On **XInput** (Xbox 360) with a wired **Xbox One / Series** pad (`045e:0b12`), a Guide **tap** opened the **shutdown** menu (long-press). Tapping Guide then another button gave normal short-press Guide. **RP2350-USB-A** / PIO USB host.

**Cause:** Guide is GIP **`0x07` VIRTUAL_KEY**, not a bit that clears in the next `0x20` INPUT. Firmware set **`BUTTON_SYS`** and often never pushed a release into `PadIn` unless another report arrived — 360 saw Home held.

**Issue27Test overcorrection:** Forcing SYS off after **~80 ms** fixed taps but made **holds** look like short presses too.

**Fix:** In **`XboxOneHost`**: keep SYS while `0x07` says pressed; **`set_pad_in` on both press and release**; **5 s** orphan clear only if release never arrives; remove broken INPUT `memcmp` (`&prev + 4` was struct stride). **File:** `XboxOne.cpp` / `XboxOne.h`.

**Test:** Tap Guide alone → guide menu (not shutdown). Hold Guide → long-press / shutdown behavior.

---

## Pico W / Pico 2 W — PIO USB wired controller unplug detection

**Problem:** On **Pico W / Pico 2 W**, the **USB gamepad** plugs into a **PIO USB** host. While PIO owns **D+ / D−**, reading line state with **`gpio_get()`** (as in **`pio_usb_bus_get_line_state()`** / **`hcd_port_connect_status()`**) often **does not** show a clean **SE0** after you pull the cable — lines can **float** or sit in a state that still looks like full-speed idle. The firmware could keep thinking the port was **connected**, so **TinyUSB** stayed up, **HostManager** still had a slot, and **Bluetooth** stayed blocked (wired takeover) until a power-cycle or “shorting” the port.

**Approach:** In **`pico_w_pio_usb_bt_mux_tick()`** (`src/OGXMini/Board/PicoW.cpp`), treat **unplug** when **`HostManager::any_mounted()`** is still true **and** **either** of these **hints** fires (share one **debounce**, ~**60 ms** wall time):

1. **`!hcd_port_connect_status(BOARD_TUH_RHPORT)`** — line-based disconnect when the HCD stack *does* see disconnect.
2. **No configured TinyUSB device** — loop device addresses **`1 … CFG_TUH_DEVICE_MAX + CFG_TUH_HUB`** (matches TinyUSB’s internal **`TOTAL_DEVICES`**) and require **`tuh_mounted(d)`** for at least one address. If the stack has dropped configuration, tear down even if the line hint lied.

**[#87](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/87) — idle-input unplug removed:** An earlier third hint (“no `process_report` for N ms”) false-unplugged **report-on-change** pads such as the **8BitDo Ultimate 2** **2.4 GHz** dongle (`2dc8:310B`): quiet for a few seconds looked like a pulled cable, so the mux **`tuh_deinit`**’d the host and input died until the Pico was unplugged from the console. Physical unplug with floating D+/D− may clear more slowly now (stack/HCD only); that is preferable to killing idle dongles.

After debounced confirmation, **`pico_w_usb_host_full_stop()`** runs **`tuh_deinit`**, stops the SOF timer, clears unplug debounce state, and **`board_api_usbh::enable_host_line_irq_monitoring()`** so normal **GPIO unplug/plug** IRQs work again; **Bluetooth** release paths run as before.

**BT quiet during host-up ([#47](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/47) / [#87](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/87)):** Call **`wired_usb_takeover_disconnect_bt()`** when the D+/D− line has been stable long enough to **`tuh_init`**, and keep pairing scans off for the **entire** `s_pio_usb_tuh_inited` window (enumeration + mounted). Previously scans were re-enabled whenever **`!wired_mounted`**, which put **CYW43 BR/LE inquiry** back on during DualShock 4 / **PowerA GIP** enumeration and broke wired pads on Pico W (same hardware OK on non-W UF2s). Release pairing only from **`pico_w_usb_host_full_stop()`** (unplug) or BT-priority teardown.

**Pure DS4 HID OUT keepalive ([#47](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/47)):** **`HostManager::send_feedback()`** treats first-party DS4 as **`ps4_hid_periodic`** when the device has **no XInput** interface (`!has_xinput`). The old `!ps_style_hid` gate never fired for PS4 HID, so pure DS4 never got the **200 ms** LED/rumble OUT refresh.

**8BitDo XInput LED keepalive ([#87](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/87)):** **`Xbox360Host`** schedules periodic LED-off for VID **`0x2DC8`** PIDs **`3016` / `3106` / `310B` / `3107` / `3109`** (Ultimate 2 / Adapter idle & mode IDs).

---

## DualShock 3 — automatic USB programming for Bluetooth pairing

**Background:** The **DualShock 3** does not implement standard Bluetooth pairing. The host’s **BD_ADDR** must be written to the pad over **USB** using a **HID feature report** (report ID **`0xF5`**, 8 bytes: id + padding + 6-byte MAC). This is what the [Bluepad32 sixaxispairer](https://bluepad32.readthedocs.io/en/latest/pair_ds3/) utility does from a PC.

**Behavior in this firmware:** On builds with **`CONFIG_EN_BLUETOOTH`** (e.g. **Pico W / Pico 2 W / RP2354**), after sync wired init (**feature 0xF4** + LED OUT), **`PS3Host::program_ds3_bt_host()`** sends **`SET_REPORT`** **feature 0xF5** with **`uni_bt_get_local_bd_addr_safe()`** using the same **synchronous control path** as **0xF4** (so PIO USB host frames complete the transfer). If the local address is still **all zeros**, a **deferred** flag is set and **`try_deferred_ds3_bt_pair()`** retries from **`process_report()`** until the address is valid, then sends **0xF5** once.

**Regression note:** An earlier PIO USB wired-init change marked init complete before the old async **GET 0xF2 → SET 0xF5** chain ran, so **`0xF5` was never sent**. Pairing no longer depends on that F2 chain.

**User steps:** Plug the **DS3 into the adapter’s USB host port** (wait for it to work wired if you like), then **unplug** and press **PS** to use it wirelessly — same as the manual pairing flow documented for Bluepad32.

**Files:** `src/USBHost/HostDriver/PS3/PS3.cpp`, `PS3.h`.

---

## Pico W / Pico 2 W — DualShock 4 and Classic Bluetooth (BR/EDR) stability

**Context:** On **CYW43439** (Pico W / Pico 2 W), **BLE** and **Classic Bluetooth (BR/EDR)** share one radio. **DualShock 4** uses **Classic ACL** only. **DualSense**, **Xbox Series (BLE)**, and **Switch Pro** (typical pairing) use **LE** — so DS4 was uniquely sensitive to how the stack used the radio at the same time as other activity.

### 1. BLE advertising vs Classic ACL (main DS4 drop fix)

**OGX-Mini** starts **BLE advertising** via **BLEServer** so the **phone / web app** can discover the adapter. If that advertising stays on while a **Classic** gamepad (DS4, DualShock 3 BT, Xbox 360 wireless) holds an **ACL** link, the connection often **dies within seconds** (e.g. blue LED then disconnect).

- **`gap_advertisements_enable(0)`** at the **start of DS4 HID setup** (before lightbar / calibration traffic), and again whenever **any** ready gamepad uses **`GAP_CONNECTION_ACL`**.
- **`gap_advertisements_enable(1)`** when the **last** Classic pad disconnects (BLE-only controllers, e.g. DualSense still connected, are unaffected by this rule).

**Files:** `Firmware/external/bluepad32/.../parser/uni_hid_parser_ds4.c` (OGXM Pico path), `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`.

**User impact:** While a **Classic Bluetooth** controller is connected, the adapter **does not advertise** for the BLE web app. Disconnect that controller (or use **USB** for setup) to use **Bluetooth** in the web app again.

### 2. BR/EDR inquiry while Classic is connected

**Periodic inquiry** (scanning for new gamepads) **plus** an active **Classic ACL** also contends on the same radio and contributed to **short-lived DS4** links.

- **`uni_bt_bredr_scan_stop()`** at the beginning of DS4 setup and when any **ACL** pad becomes **ready**.
- **`uni_bt_bredr_scan_start()`** when the **last** Classic pad disconnects (if scanning is still enabled globally).

**File:** `Bluepad32.cpp` (and DS4 setup in `uni_hid_parser_ds4.c`).

### 3. DS4 virtual “touchpad mouse” disabled on Pico

Bluepad32 can register a **second virtual HID device** (mouse) on the same DS4 link. On Pico W that path was linked to **unstable links**; the OGX build (**`OGXM_BLUEPAD32_PICO_W`**) **does not create** that virtual device. **DualSense** still uses its normal setup (virtual child rejected in `device_ready` where applicable).

**File:** `uni_hid_parser_ds4.c` (CMake defines `OGXM_BLUEPAD32_PICO_W=1` for the Bluepad32 library in `Firmware/RP2040/CMakeLists.txt`).

### 4. PS4 rumble / FF grace period

For **`CONTROLLER_TYPE_PS4Controller`**, **rumble output** is **delayed ~6 seconds** after connect so the host cannot push force-feedback during the fragile init window.

**File:** `Bluepad32.cpp` — `send_feedback_cb` / `device_ready_cb`.

### 5. Related Pico W Bluetooth improvements (see also Summary table)

- **Core0 `sleep_ms(1)`** in the main loop so Core1 (BT stack) gets CPU time ([Team-Resurgent/OGX-Mini](https://github.com/Team-Resurgent/OGX-Mini) pattern).
- **Lock-free Bluetooth input path** (`set_pad_in_from_bluetooth`) so HID callbacks never block on Core0’s mutex. **Joy-Con pairs:** **2-slot** staging ring so back-to-back L/R reports are not overwritten before Core0 drains (see [§ Joy-Con pair merge — latency](#joy-con-pair-merge--latency-when-both-halves-are-active)).
- **Reconnect:** last device disconnect calls **`uni_bt_enable_new_connections_unsafe(true)`** so pairing can resume without power-cycling.
- **8 s input-stall disconnect** (non-virtual, non-BLE-Xbox) to clear zombie links; **BLE Xbox** uses keepalive instead of stall disconnect.

### 6. Connection rumble when the wireless controller is ready *(v1.0.0.9a)*

When Bluepad32 reports **`device_ready`** for a non-virtual gamepad, the firmware plays about **one second** of **dual-motor rumble** (via `play_dual_rumble` when the controller supports it) so you can **feel** that the Bluetooth link is up. **DualShock 4** uses a **~1.2 s delay** before starting that rumble so it does not overlap the fragile post-connect window (aligned with the existing PS4 rumble grace logic). Other controller types use a shorter default delay.

**File:** `src/Bluepad32/Bluepad32.cpp` — `ogxm_play_connection_rumble()`, called from `device_ready_cb`.

---

## Switch Pro — analog stick sensitivity

A configurable sensitivity gain is applied to the analog sticks in Switch Pro emulation. Raw stick values (outside the deadzone) are scaled by **STICK_GAIN_NUM / STICK_GAIN_DEN** (default 120/100 = 1.2×) before mapping to the 12-bit Switch report. The same physical deflection produces slightly larger output for a more responsive feel.

**File:** `src/USBDevice/DeviceDriver/Switch/Switch.cpp` — `gamepad_to_switch_report()`; tune via `STICK_GAIN_NUM` and `STICK_GAIN_DEN`.

---

## Build scripts (new users)

To build firmware without memorizing CMake options, use the interactive build scripts from the **project root**:

| Platform | Command |
|----------|---------|
| **Linux / macOS** | `./scripts/build.sh` |
| **Windows (PowerShell)** | `.\scripts\build.ps1` |

The script checks for required tools (git, python3, cmake, ninja, arm-none-eabi-gcc) and prints install hints if something is missing. It then prompts for: (1) board (Pi Pico, Pico W, Pico 2 W, RP2040-Zero, XIAO, Feather, 4CH I2C, ESP32 hybrid, etc.); (2) default (all modes via combos) or fixed output mode (e.g. Wii, GameCube, N64); (3) Release or Debug. Build output (`.uf2`, `.elf`) is written to **`scripts/build/`**; on failure you can save a log to `scripts/build_log.txt`. See the main [README](../../../README.md) Build section for the full description.

---

## Future planned

Work that is **not implemented** in current firmware but has been researched. These items are **not** a matter of adding a VID/PID to a table — they need new host drivers and/or radio stacks.

### Xbox Wireless Adapter for Windows (`045e:02e6`, `045e:02fe`)

**Devices:**

| VID:PID | Model | USB product string | Status |
|---------|-------|-------------------|--------|
| **`045e:02e6`** | 1713 | Xbox Wireless Adapter for Windows (older) | **Not supported** |
| **`045e:02fe`** | 1790 | **XBOX ACC** (newer) | **Not supported** |

These are **USB dongles** that receive **Xbox Wireless** (proprietary 2.4 GHz) from Xbox One and Xbox Series controllers. They are **not** gamepads and do **not** enumerate as XInput devices.

**Do not confuse with the Xbox 360 PC receiver** — that **is supported** today:

| VID:PID | Device | Why it works |
|---------|--------|--------------|
| **`045e:0719`** (typical) | Xbox 360 wireless **PC receiver** | Presents a standard XInput-style USB interface (`bInterfaceSubClass=0x5D`, `bInterfaceProtocol=0x81`). Handled by `tuh_xinput` / `Xbox360WHost`. |

The One/Series dongles use a **completely different USB shape** (vendor bulk, not XInput interrupt).

#### What the dongle looks like on USB

From hardware dumps (e.g. [ControllersInfo adapter descriptor](https://github.com/DJm00n/ControllersInfo/blob/master/xboxone/DescriptorDump_Adapter%20(Xbox%20Wireless%20Adapter%20for%20Windows).txt)):

- **Device class:** vendor-specific (`0xFF/0xFF/0xFF`), product name **"XBOX ACC"**
- **Endpoints:** **bulk** IN/OUT (512-byte packets on USB 2.0), not the interrupt endpoints used by **wired** Xbox One pads
- **Inside the stick:** a **Mediatek MT76** Wi‑Fi chipset that must run **dongle firmware** before it can talk to controllers

OGX-Mini’s existing **wired Xbox One GIP** path (`tuh_xinput`, subclass `0x47` / protocol `0xD0`, `XboxOneHost`) parses GIP **after** the controller is already connected over USB. The wireless adapter never exposes that interface — the host must drive the **dongle radio** first.

#### Why this is not a simple port

1. **Firmware upload** — Linux [xone](https://github.com/medusalix/xone) / [xow](https://github.com/medusalix/xow) load **`xow_dongle.bin`** / variant blobs (e.g. **`xone_dongle_02fe.bin`**) into the MT76 chip over USB before the device is useful. That blob must be shipped in flash and the load sequence reimplemented on RP2040.

2. **Wireless stack, not HID** — The driver brings up **MT76** (channels, pairing scan, client join/leave, optional encryption), wraps **GIP** payloads in **802.11-style data frames**, and sends them on bulk OUT queues. Inbound bulk IN carries WLAN frames that must be parsed to extract GIP input. This is the bulk of [xone `transport/dongle.c`](https://github.com/medusalix/xone/blob/master/transport/dongle.c) + [`transport/mt76.c`](https://github.com/medusalix/xone/blob/master/transport/mt76.c) — thousands of lines, not a report-descriptor tweak.

3. **Resource cost** — xone uses many bulk URBs, WLAN buffers up to tens of KB per packet, and ongoing radio work. RP2040 flash/RAM and Core1 USB host timing are tight compared to a PC kernel driver.

4. **Pairing model** — Controllers must be **paired to the dongle** (Sync on the pad while the dongle is in pairing mode). Pads previously used over **USB** or **Bluetooth** will not auto-attach until re-paired to the dongle — same as on Windows with xone.

5. **Reuse is partial only** — Once a wireless client is connected and GIP input arrives, decoding could **reuse** existing GIP constants and mapping from `Descriptors/XboxOne.h` / `XboxOneHost.cpp`. Everything **before** that (USB dongle + radio + framing) is new work — comparable in scope to adding Switch 2 bulk bring-up, but **larger** because of firmware + Wi‑Fi.

**Rough implementation phases (if pursued):**

1. New TinyUSB **vendor bulk** class driver; claim `045e:02e6` / `045e:02fe`; embed firmware; port MT76 init from xone.  
2. Bulk I/O loops, pairing, client add/remove (mirror `Xbox360W` connect callbacks in `HostManager`).  
3. Bridge decoded GIP `0x20` / `0x07` reports into `XboxOneHost` (or a shared GIP parser).  
4. Rumble/LED over wireless GIP OUT path.

**Practical alternatives today:**

- **Xbox 360 wireless receiver** + 360 pads — already supported (`Xbox360WHost`).  
- **Xbox One / Series controller** — **Bluetooth** on Pico W / Pico 2 W (Bluepad32), or **wired USB** (`XboxOneHost`).  

**References:** [medusalix/xone](https://github.com/medusalix/xone), [medusalix/xow](https://github.com/medusalix/xow), [SDL discussion of 02fe vs XInput PID](https://github.com/libsdl-org/SDL/pull/8683), [ControllersInfo dongle descriptor dump](https://github.com/DJm00n/ControllersInfo/tree/master/xboxone).

**Files that would be touched (when implemented):** new `tuh_xbox_dongle` (or similar) under `USBHost/HostDriver/XInput/`, `HostManager.h`, `tuh_callbacks.cpp`, `tusb_config.h`; possible shared GIP layer with `XboxOne.cpp`.

---

## Summary

| Area | Improvement |
|------|-------------|
| **XInput (360)** | XSM3 authentication and descriptors aligned with joypad-os; adapter works on Xbox 360 with BT controllers (PS5, Xbox One). **360 wireless PC receiver** supported (`Xbox360WHost`). **v1.0.0.11a:** PIO USB wired receiver — **4 XInput instances** when `MAX_GAMEPADS=1`, port **priming**, **0x80** connect detect, byte-offset decode. **Razer Atrox XBO** (`1532:0a00`) — vendor GIP arcade path via **XBOFS** init + **30-byte** IN + XBOFS input layout; **POWER_ON** once only. **Xbox One/Series wireless dongle (`045e:02e6` / `02fe`)** — see [Future planned](#future-planned). 8BitDo wired fix: LED keepalive for VID 0x2DC8 / PID 0x3016 or 0x3106. **v1.0.0.12a (#38):** default (non–WebApp-customized) sticks use **stock 360-like** feel — see [§ XInput stock stick feel](#xinput--xbox-360--stock-stick-feel). |
| **PS3** | Stuck inputs and delays addressed via L2/R2 axes; DS3-accurate sticks (0–255, center 0x80, ~1.5% deadzone); D-pad and face button mapping; Home (PS) button with 8-frame latch for BT controllers. **v1.0.0.9a:** PC host rumble deadzone + strict small-motor `0`/`1`. **v1.0.0.11a:** **DualShock 3 wired USB host** — USB Host Shield init, sync control xfer + PIO service, **1 Hz keepalive**. **v1.0.0.12a:** motion passthrough (see Motion row); **DS3 USB→BT auto-pair** sync **0xF5** restored on BT boards. |
| **PS2 (GPIO)** | Home only = IGR (L1+L2+R1+R2+Start+Select); Home+Start = shutdown (L1+L2+R1+R2+L3+R3). OPL and protocol stability (first response byte = mode byte). |
| **OG Xbox** | Guide only = IGR. Shutdown = LT+RT+Back+White via **Guide+Start** or **Guide+View (Back)**; Xbox BT often omits Start while Guide is held. Shutdown report strips Start so the chord matches BIOS/softmod expectations. |
| **Switch Pro** | Analog stick sensitivity gain (default 1.2×) for more responsive sticks; configurable in `Switch.cpp`. **v1.0.0.11a:** **Switch 1/2 Pro wired** on PIO USB host — Chromium **0x80** init, bulk bring-up, mapping fixes. **v1.0.0.12a:** **HD rumble** from console → `PadOut` for BT/USB input pads. See [§ Switch HD rumble](#switch-mode--hd-rumble-passthrough) and [§ PIO USB host — wired connection fixes](#pio-usb-host--wired-connection-fixes-waveshare-rp2350-usb-a). |
| **Switch 2** | **Pro 2** (wired **0x2069**): **Switch2ProHost** + bulk bring-up (v1.0.0.11a). **Pro 2 + Joy-Con 2 L/R** (BLE): **uni_hid_parser_switch2** — GATT pairing, 63-byte input, rumble keepalive, Home → SYS latch, **L+R pair merge**, **dual-half latency fixes**. **v1.0.0.12a (#64):** anti-deadzone idle gate + L3/R3 / wired disconnect keepalives. See [§ Nintendo Switch 2 — Bluetooth](#nintendo-switch-2--bluetooth-pico-w--pico-2-w), [§ Joy-Con pair latency](#joy-con-pair-merge--latency-when-both-halves-are-active), [§ anti-deadzone](#switch-2-pro--anti-deadzone-and-l3r3). |
| **Steam Controller 2026** | **Triton BLE** (`28de:1303`) on Pico W / Pico 2 W / RP2354 — HOGP parser, **LE Secure Connections**, lizard-off, rumble, **~7.5–11 ms** conn interval, View→Start / Menu→Back. See [§ Steam Controller 2026](#steam-controller-2026-triton--bluetooth). |
| **Boards** | RP2350_ZERO, RP2040_XIAO supported (Standard/PIO-USB host). **RP2354:** RP2350 + **Pi Radio Module 2** — **Bluetooth** + PIO USB host (Pico 2 W firmware path). **RP2350-USB-A (Waveshare):** v1.0.0.11a validation for **Switch Pro, DualShock 3, 360 wireless receiver**, and **Razer Atrox XBO** wired host. |
| **Latency** | Main loop delay default **0 µs**; `tud_task()` before `process()` so reports send every loop when ready; XInput/Switch/PS3 send latest state when USB ready (no `new_pad_in()` gate). Switch Pro and PS3 always build report every loop so host poll (`get_report`) and IN push both see current state — only remaining delay is BT radio when wireless. |
| **STEAM (SteamOS / Bazzite)** | **Start + LB + D-pad Up** (~3 s), web app, or **`-DOGXM_FIXED_DRIVER=STEAM`**. USB **DualSense** (`054c:0ce6`) + **HID mouse**. **DualSense input:** passthrough report + **touchpad → mouse** (BT or wired USB host). **Other pads:** synthesized DualSense report; **no stick-mouse fallback**. See [§ STEAM mode](#steam-mode--steamos--bazzite-linux-desktop). |
| **Motion (PS3/PS4 out)** | **v1.0.0.12a:** Accel/gyro passthrough into emulated DS3/DS4 from DS4/DS5/Switch Pro/SW2/Wii Remote. Console use needs **Brook Wingman XE 2** (or similar). See [§ motion passthrough](#ps3--ps4-output--motion-passthrough). |
| **Build** | Interactive scripts `scripts/build.sh` (Linux/macOS) and `scripts/build.ps1` (Windows) for board selection, fixed/default mode (**STEAM**, PS4, Wii, etc.), and Release/Debug; output in `scripts/build/`. See [README](../../../README.md) Build section. |
| **Bluetooth (Pico W / 2 W / RP2354)** | **DS4 / Classic ACL:** BLE advertising **paused** while Classic pad connected; **BR inquiry stopped** during ACL; **no DS4 virtual mouse**; **6 s PS4 rumble** grace. **Xbox Series (BLE):** no stall disconnect when idle; keepalive 12 s. **v1.0.0.12a:** after last **ready** pad disconnects, restore pairing mode then **watchdog reboot** (~500 ms); **USB resume** + **45 s** idle scan watchdog restore pairing without reboot; **LE Secure Connections** for pads that require it (e.g. Triton); **Steam Controller 2026** HOGP parser. **Switch 2 (BLE):** Pro 2 + Joy-Con 2 L/R — custom GATT parser, rumble keepalive, Home latch, **L+R merge**, **dual-half latency fixes**. **Switch 1 Joy-Con (Classic BT):** L+R merge, IMU off when paired. **General:** `sleep_ms(1)` main loop; lock-free BT pad-in (**2-slot** staging for Joy-Con pairs). **v1.0.0.9a:** ~**1 s connection rumble** at `device_ready` (DS4 delayed start). |
| **PIO USB host (Pico W)** | **Wired unplug:** Debounced combo of **HCD connect**, **`tuh_mounted` over all device addresses**, and **no `process_report` / setup activity** (~**3 s**) so disconnect registers when D+/D− line state is wrong under PIO; **`tuh_deinit`** + restore GPIO line IRQs + BT release. **v1.0.0.11a (all PIO host boards):** **`pio_usb_host_frame()`** after feedback OUT; wired **Switch Pro / PS3 / 360 receiver / Razer Atrox XBO** connection fixes — see [§ PIO USB host — wired connection fixes](#pio-usb-host--wired-connection-fixes-waveshare-rp2350-usb-a). |
| **DS3 + Bluetooth** | **USB auto-pair:** After sync **0xF4** + LED, **feature `0xF5`** programs the **DS3** with the adapter’s **BD_ADDR** (`CONFIG_EN_BLUETOOTH`); deferred if BT address not ready. **v1.0.0.12a:** restored after PIO USB wired-init regression (sync SET, no F2 chain). See [§ DualShock 3 — automatic USB programming for Bluetooth pairing](#dualshock-3--automatic-usb-programming-for-bluetooth-pairing). |
| **Multi-adapter (same console)** | **Known limitation:** Two OGX-Mini units plugged into the **same** console usually collide — identical **VID/PID**, shared USB serial (e.g. XInput `"1.0"`), and on **Xbox 360** the same **XSM3 ID**. Workarounds without firmware changes: **one OGX + one native/other-brand pad**, or a **single multi-port** host (e.g. 360 wireless receiver). Unique per-unit identity is not implemented yet. |
