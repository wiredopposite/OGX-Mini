# PS3 / PS4 motion controls

**PS3** and **PS4** modes can forward **tilt / motion** from compatible input controllers into the emulated report (Sixaxis on PS3, accelerometer on PS4). **Switch** output mode does **not** pass through motion for now.

## Important — what “PS4 mode” is (and is not)

**PS4 mode is made for motion controls to work on authentication dongles.** It enumerates a DualShock 4–style USB gadget so a licensed converter (e.g. Brook Wingman) can authenticate to the console and pass through buttons + motion.

It does **not** enable native **PlayStation 4 console output** by itself. **PS4 output requires authentication from a licensed dongle**, and that authentication is **not supported directly by this firmware**. Do not expect plugging OGX-Mini alone into a PS4 USB port to work as a DualShock 4.

| | |
|--|--|
| **Select PS3 mode** | **Start + D-pad Left** (~3 s) |
| **Select PS4 mode** | **Start + Left Bumper + D-pad Left** (~3 s) |
| **Supported input (BT — Pico W / Pico 2 W)** | DualShock 4, DualSense, Switch Pro, **Wii Remote** (accelerometer) |
| **Supported input (wired USB host)** | DualShock 4, DualSense, Switch 1 Pro, Switch 2 Pro |
| **Wii Remote** | Point the **IR end at the TV**; motion is enabled automatically when motion output is active |
| **Play on a real PS3 or PS4** | OGX-Mini → **USB authentication dongle** → console. **Tested with [Brook Wingman XE 2 Converter](https://www.brookaccessories.com/products/wingman-xe2).** **PS3** mode → Brook → PS3; **PS4** mode → Brook → PS4. |
| **PC testing (PS3/PS4)** | No Brook required — use PS3/PS4 mode on a PC/emulator to verify tilt |

**Typical chain for a PS3 motion game (e.g. *Flower*):**

```text
Wii Remote or DualSense (Bluetooth) → OGX-Mini (Pico W) → USB → Brook Wingman XE 2 → PS3
```

**Typical chain for PS4 motion (auth dongle required):**

```text
Input pad → OGX-Mini (PS4 mode) → USB → licensed auth dongle (e.g. Brook) → PS4
```

Technical detail: [IMPROVEMENTS.md — motion passthrough](IMPROVEMENTS.md#ps3--ps4-output--motion-passthrough).
