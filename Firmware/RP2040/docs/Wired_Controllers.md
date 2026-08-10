# Wired controllers (USB input)

This document lists controllers supported when connected to the OGX-Mini adapter **via a wired (USB) connection**. These work as input when the adapter is outputting to any supported platform (Original Xbox, Xbox 360, Switch, PS3, DInput, PS1/PS2 GPIO, Dreamcast, GameCube, N64, etc.).

**Note:** Some third‑party controllers change their VID/PID depending on mode; they may need to be set to the correct mode (e.g. XInput, DInput, or Switch) to be recognized.

---

## By driver type

### XInput (Xbox 360, Xbox One, Xbox Series)

- **Microsoft:** Xbox 360 (wired and **360 PC wireless receiver**), Xbox One / Series **wired USB**, Xbox Elite
- **Not supported (planned):** Xbox One / Series **wireless USB dongle** (`045e:02e6`, `045e:02fe`) — see [Planned_Additions.md](Planned_Additions.md#xbox-wireless-adapter-for-windows-045e02e6-045e02fe)
- **Third‑party:** Controllers that identify as XInput over USB (e.g. many 8BitDo, PowerA, PDP, Afterglow when in XInput mode)
- **Razer Atrox Arcade Stick:** **Xbox One** (`1532:0a00`, vendor GIP); **Xbox 360** (`24c6:5000`, standard XInput, digital LT/RT)
- **Other Xbox One GIP arcade sticks:** Mad Catz FightStick TE 2 (`0738:4a01`), PDP Xbox One Arcade Stick (`0e6f:015c`), Hori RAP Hayabusa / V Kai / Fighting Commander ONE — see `XBOX_ONE_GIP_IDS` in `XboxArcadeStick.h`

*Matched by USB class for standard XInput pads; arcade sticks above use explicit VID/PID + GIP init. UsbdSecPatch is not required on Xbox 360.*

**Razer Atrox arcade stick (wired USB host):** **Xbox One** model (`1532:0a00`) uses GIP on interface 0. Init follows **XBOFS.win** (`05 20 00 01 00`, wait for OUT, then **30-byte** IN read loop); input uses XBOFS byte layout (face/LT/RT on byte 22). Standard Xbox One pads use 64-byte Linux xpad GIP. **Xbox 360** model (`24c6:5000`) uses standard XInput with digital trigger bytes.

**Xbox 360 wireless PC receiver (`045e:0719`), wired to adapter USB host:** Plug the **receiver** into the adapter’s host port; sync a **wireless 360 pad** to any quadrant. On **PIO USB host** boards (Waveshare RP2350-USB-A, Pico, Feather, etc.), v1.0.0.11a polls **all four RF ports** even when `MAX_GAMEPADS=1` and primes idle ports for sync. See [IMPROVEMENTS.md — PIO USB host wired fixes](IMPROVEMENTS.md#pio-usb-host--wired-connection-fixes-waveshare-rp2350-usb-a).

---

### PlayStation 3 (DualShock 3 and compatible)

- Sony DualShock 3 (Dualshock 3 / Batoh)
- Sony DualShock 3 (alternate/clone VID)
- Razer Panthera (PS3)
- Afterglow PS3 (multiple models)
- Bigben, Gioteck, Hori (Fighting Commander, Horipad, Fightstick), Mad Catz (Fightstick, Fightpad), PDP, PowerA, Nyko, and other PS3‑protocol controllers

*See `PS3_IDS` in `HardwareIDs.h` for the full VID/PID list.*

**Button mappings (all modes):** [Controller_Mappings.md](Controller_Mappings.md)

**DualShock 3 over USB cable (PIO USB host):** v1.0.0.11a sends **SET feature 0xF4** and a **player LED OUT** report at connect (USB Host Shield sequence) with **`pio_usb_host_frame()`** during init, plus a **1 Hz keepalive OUT** so the pad stays connected. On **Pico W / Pico 2 W / RP2354** BT builds, **v1.0.0.12a** sync-sends Bluetooth **auto-pair** (feature **0xF5** with the adapter BD_ADDR) right after that wired init (deferred if the radio address is not ready yet). See [IMPROVEMENTS.md — DualShock 3 wired USB host](IMPROVEMENTS.md#dualshock-3--wired-usb-host) and [automatic USB programming for Bluetooth pairing](IMPROVEMENTS.md#dualshock-3--automatic-usb-programming-for-bluetooth-pairing).

---

### PlayStation 4 (DualShock 4 and compatible)

- Sony DualShock 4 (all revisions)
- Sony DS4 wireless adapter (USB dongle)
- Hori Fighting Commander 4, Hori PS4 Mini
- Brook, Nacon, PDP, PowerA, Razer (Raiju), Scuf, Victrix, and other DS4‑protocol controllers

*See `PS4_IDS` in `HardwareIDs.h` for the full VID/PID list.*

---

### PlayStation 5 (DualSense and compatible)

- Sony DualSense (PS5)
- Sony DualSense Edge (PS5)
- Sony PS5 Access Controller

*See `PS5_IDS` in `HardwareIDs.h` for the full VID/PID list.*

---

### Nintendo Switch (Pro and wired)

- **Switch Pro / Joy‑Con / Wii U Pro:**  
  Nintendo Switch Pro Controller, Wii U Pro Controller, Joy‑Con (L/R), Nintendo Switch 2 Pro Controller, Joy‑Con 2 (L/R)
- **Wired Switch:**  
  PowerA (wired, Enhanced, Fusion, Spectra, Arcade Stick, Fusion Pro), Hori (Pokken, Horipad for Switch / Switch 2), Afterglow Deluxe, Faceoff Deluxe, and other wired Switch controllers

*See `SWITCH_PRO_IDS` and `SWITCH_WIRED_IDS` in `HardwareIDs.h` for the full VID/PID list.*

**Switch 1 Pro (PID 0x2009), wired USB:** Uses **Chromium-style wired USB init** (report **0x80** subcommands: MAC, handshake, baud, disable timeout; then **0x12** + subcommand **0x33** probe, player LED, full report mode, IMU). **Not** the Bluetooth **0x11** handshake path. After init, standard **SwitchProHost** decodes the **10-byte** `SwitchPro::InReport` payload (report IDs **0x30** / **0x31** or raw 10 bytes). D-pad and **L3/R3** mapping corrected for wired layout. See [IMPROVEMENTS.md](IMPROVEMENTS.md#pio-usb-host--wired-connection-fixes-waveshare-rp2350-usb-a).

**Switch 2–family USB (Pro Controller 2, Joy‑Con 2, NSO GameCube, etc.):**  
The firmware runs the same vendor **bulk OUT** bring‑up on **USB configuration 1, interface 1** as the PC capture tool (`Tools/controller_capture/switch2_usb_init.py` / [HandHeldLegend procon2tool](https://github.com/HandHeldLegend/procon2tool)). **Switch 2–family devices do not run Switch 1 `0x80` HID init after bulk** — bring-up completes at **`DONE`** and HID input reports begin immediately. Wired input reports may use report ID **0x09** (with a 1‑byte counter after the ID).

**Switch 2 Pro (PID 0x2069), wired USB:** After the same **bulk OUT** bring‑up on **config 1 / interface 1**, **Switch2ProHost** parses the standard **64‑byte** HID report (ID **0x09**, 1‑byte counter, then **10‑byte** `SwitchPro::InReport` payload). Digitals use **Switch‑2‑specific** bit positions (not classic **Buttons0/1/2**): face **B/A/Y/X**, **RB**, **LB** (Home bit), **Minus** → Back, **Plus** / d‑pad directions, **L3**, **Start** (classic R), **R3** (classic ZR bit), **SYS** (classic `bl` d‑pad up), **ZL/ZR** → **LT/RT** (digital full press from captured bits), and **Capture** is still not mapped to **MISC**. **GL**, **GR**, and **Chat** are **documented in source** (`Switch2ProHost.cpp`) but **left unmapped**. Extended report bytes are not used for decoding (avoids IMU coupling / flicker). **Bluetooth LE** for **Pro 2** and **Joy-Con 2** (**0x2066** / **0x2067**) on **Pico W / Pico 2 W** is documented in [IMPROVEMENTS.md](IMPROVEMENTS.md#nintendo-switch-2--bluetooth-pico-w--pico-2-w) (custom GATT — not wired USB). Other Switch 2–family PIDs keep **SwitchProHost**.

---

### PlayStation Classic

- PlayStation Classic controller (official)

*See `PSCLASSIC_IDS` in `HardwareIDs.h`.*

---

### Nintendo 64 (USB)

- Retrolink N64 USB gamepad
- Hyperkin Admiral N64, Hyperkin N64 Controller Adapter
- Mayflash N64 Adapter

*See `N64_IDS` in `HardwareIDs.h` for the full VID/PID list.*

---

### Original Xbox (USB host)

- Original Xbox Duke and S (via USB adapter/converter)

*Used when the adapter is in OG Xbox output mode with a compatible USB host path.*

---

### DInput / Generic HID

Controllers that use the standard HID gamepad (DInput) protocol, including:

- **Logitech:** F310, F510, F710, RumblePad 2, Dual Action, WingMan, ChillStream, etc.
- **8BitDo:** Many models in DInput/HID mode (e.g. Pro 2, Pro 3, Ultimate, Lite, FC30 Pro, F30 Arcade, GameCube, Dogbone)
- **Hori:** Fightsticks, Fighting Commander, Horipad (various), Taiko Controller, etc.
- **Mad Catz:** Fightsticks, Fightpad, CTRLR, etc.
- **Qanba, Mayflash, Brook:** Arcade sticks and adapters
- **PowerA, PDP, Afterglow:** Various Xbox/PC controllers in HID mode
- **GameSir:** G3, G4, T3, T4, G7 Pro, etc.
- **Razer:** Kishi, Hydra, Serval, Raiju (PC/HID)
- **Steam:** Steam Controller, Steam Deck (when presenting as gamepad). **Steam Controller 2026** body USB is **`28de:1302`** (generic HID if supported by the host stack). **Wireless BLE** (`28de:1303`) on Pico W / Pico 2 W / RP2354 is documented in [IMPROVEMENTS — Steam Controller 2026](IMPROVEMENTS.md#steam-controller-2026-triton--bluetooth).
- **Sony:** DualShock 2 (via USB adapter), Steam Virtual Gamepad
- **Flydigi:** Vader 4 Pro (DInput mode)
- **Scuf:** Envision (Linux/HID)
- **Elecom:** Various gamepads and adapters
- **Other:** ThrustMaster, BigBen, Capcom Home Arcade, Cthulhu, XinMo, Zenaim, Datel, and other generic DInput/HID gamepads and arcade sticks

*See `DINPUT_IDS` in `HardwareIDs.h` for the full VID/PID list. This list is large and includes many fightsticks and third‑party pads.*

---

### Generic HID (unspecified mapping)

- Other HID gamepad‑like devices may work; button and axis mappings might need to be adjusted in the [web app](https://megacadedev.github.io/OGX-Mini-2026-WebApp/).

---

## Reference

- **VID/PID lists:** `Firmware/RP2040/src/USBHost/HardwareIDs.h`
- **Host drivers:** `Firmware/RP2040/src/USBHost/HostDriver/`
- **Adding a new controller:** [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) (capture, drivers, Debug UART)
- **Platform selection:** See the main [README](../../../README.md) for button combos to change output platform.
- **SteamOS / Bazzite (STEAM output):** USB presents **DualSense** (`054c:0ce6`) + **HID mouse**. **DualSense** input (wired or BT): passthrough gamepad + **touchpad → cursor**; other pads get synthesized DualSense mapping (gamepad only — no stick mouse). **Start + LB + D-pad Up** selects mode. See [SteamOS / Bazzite output mode](SteamOS_Bazzite_Output_Mode.md), [IMPROVEMENTS — STEAM mode](IMPROVEMENTS.md#steam-mode--steamos--bazzite-linux-desktop), and [Controller_Mappings — STEAM](Controller_Mappings.md#steamos--bazzite-steam-mode).
