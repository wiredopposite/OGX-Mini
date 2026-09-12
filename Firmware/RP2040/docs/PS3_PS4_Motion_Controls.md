# PS3 / PS4 motion controls

**PS3** and **PS4 (DualShock 4 USB)** output modes can forward **tilt / motion** from compatible input controllers into the emulated report (Sixaxis on PS3, accelerometer on PS4). **Switch** output mode does **not** pass through motion for now.

| | |
|--|--|
| **Select PS3 mode** | **Start + D-pad Left** (~3 s) |
| **Select PS4 mode** | **Start + Left Bumper + D-pad Left** (~3 s) |
| **Supported input (BT — Pico W / Pico 2 W)** | DualShock 4, DualSense, Switch Pro, **Wii Remote** (accelerometer) |
| **Supported input (wired USB host)** | DualShock 4, DualSense, Switch 1 Pro, Switch 2 Pro |
| **Wii Remote** | Point the **IR end at the TV**; motion is enabled automatically when motion output is active |
| **Play on a real PS3 or PS4** | OGX-Mini does **not** plug straight into the console for this — use a **USB adapter** between OGX-Mini and the console. **Tested with [Brook Wingman XE 2 Converter](https://www.brookaccessories.com/products/wingman-xe2).** Works on **PS3** (PS3 mode) and **PS4** (PS4 mode on OGX-Mini → Brook → PS4). |
| **PC testing (PS3/PS4)** | No Brook required — use PS3/PS4 mode on a PC/emulator to verify tilt |

**Typical chain for a PS3 motion game (e.g. *Flower*):**

```text
Wii Remote or DualSense (Bluetooth) → OGX-Mini (Pico W) → USB → Brook Wingman XE 2 → PS3
```

Technical detail: [IMPROVEMENTS.md — motion passthrough](IMPROVEMENTS.md#ps3--ps4-output--motion-passthrough).
