# Pico 2 W – Wii Mode with USB Controller

This document describes how **Pico 2 W** (and Pico W) use a **USB gamepad** on an **external USB host port** when the adapter is in **Wii (Wiimote) mode**. The approach is the same as in [PicoGamepadConverter](https://github.com/wiredopposite/PicoGamepadConverter): the single Bluetooth radio is used for the Wiimote connection to the Wii, so the controller must be USB and must be on **PIO USB** (external port), not the built-in USB.

---

## 1. Hardware constraints

- **Pico 2 W** (and Pico W) have a single **Bluetooth radio** (CYW43439).
- In **Wii mode** that radio is used for the **Wiimote connection** (Pico → Wii).
- You **cannot** use Bluetooth for both the Wii and a wireless controller at the same time.
- **Solution:** Use a **USB controller** on an external USB host port (PIO USB on GPIO). The controller must be plugged into the **external** USB connector (D+/D− on GPIO), not the Pico’s built-in USB port.

---

## 2. USB host: use PIO USB (external port) in Wii mode

### The bug (in projects that get it wrong)

If the firmware uses the **wrong USB port** for the controller in Wii mode:

- **`tuh_init(0)`** → **native USB (roothub 0)** = the Pico’s **built-in USB port** (the one used for programming).
- The controller is on the **external USB** on **GP0/GP1** (**PIO USB, roothub 1**), which is then **never used**.
- Result: controller gets power (e.g. vibrate) but is **never enumerated** → no LED, no input.

### The fix in OGX-Mini

When the output mode is **WII** (Wiimote) on Pico W / Pico 2 W, the firmware uses **PIO USB** (roothub port **1**) for the host:

- Configure PIO USB with the chosen D+/D− pins (and optional swap).
- Call `tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg)` then `tuh_init(1)`.
- Run `tuh_task()` in the host loop.

So the controller **must** be plugged into the **external** USB port (PIO USB). The built-in USB is not used for the host in Wii mode.

---

## 3. PIO USB pin configuration (Pico W / Pico 2 W)

For OGX-Mini, Pico W and Pico 2 W use the following when USB host is used (e.g. Wii mode). Defined in `Board/Config.h` for `CONFIG_OGXM_BOARD_PI_PICOW` / `PI_PICO2W`:

- **`PIO_USB_DP_PIN`** – Base pin for D+ (e.g. `0` → D+ = GP0, D− = GP1 with default pinout).
- **`PIO_USB_SWAP_DP_DM`** – Optional. Set to `1` if the controller gets power but does not enumerate (D+/D− swapped on the cable).

**Wiring:**

| Config              | D+   | D−   |
|---------------------|------|------|
| Swap = 0, PIN = 0   | GP0  | GP1  |
| Swap = 1             | GP1  | GP0  |

Also connect USB **GND** and **VBUS** (5 V) to the external connector.

---

## 4. Building for Pico 2 W (Wii mode)

Wii mode is **build-option only**: you must build with **`-DOGXM_FIXED_DRIVER=WII`**. Button combos cannot switch into or out of Wii; the firmware is Wii-only and combos are disabled.

- Set **`OGXM_BOARD`** to **`PI_PICO2W`** (e.g. `-DOGXM_BOARD=PI_PICO2W`).
- Set **`OGXM_FIXED_DRIVER`** to **`WII`** (e.g. `-DOGXM_FIXED_DRIVER=WII`).
- On Windows, use the project’s **build script** (e.g. `scripts/build-with-vs.ps1 -Board PI_PICO2W -Debug`) or a **Visual Studio Developer Command Prompt** so host tools build correctly.

Example (from `Firmware/RP2040`):  
`.\scripts\build-with-vs.ps1 -Board PI_PICO2W -FixedDriver WII -Debug`

---

## 5. Usage summary (Wii mode on Pico 2 W)

1. **Wire** the external USB connector: D+ and D− to the chosen GPIOs (e.g. GP0/GP1), plus GND and 5 V (VBUS).
2. **Build** firmware with **`-DOGXM_FIXED_DRIVER=WII`** for **PI_PICO2W** and **flash** the `.uf2`.
3. **Plug** the USB controller into the **external** port; the controller LED should turn on when it enumerates.
4. **Sync** the Pico to the Wii (Wii Sync, then Pico as needed). The Pico appears as a Wiimote; input comes from the USB gamepad.

---

## 6. External USB in other output modes (Pico W / Pico 2 W)

For firmware built for **Pico W / Pico 2 W** with **USB host** enabled but **not** Wii-only (e.g. combo builds, Xbox/Switch/PS output over Bluetooth or USB device), the **same external PIO USB port** (D+/D− on GP0/GP1 per §3) can host a **wired gamepad**. Input is merged with the normal gamepad path and appears on the chosen output.

- **Mutual exclusion with Bluetooth:** If **any** Bluetooth gamepad is already connected, the firmware does **not** bring up PIO USB host for that tick; wired devices on the external port are **ignored** until **all** Bluetooth gamepads disconnect. Conversely, when a wired controller **enumerates** on the external port, **all** Bluetooth gamepads are disconnected and **new** Bluetooth connections are disabled until that wired device is **unplugged** (then Bluetooth pairing/connect is allowed again).
- **No external port required:** If you do **not** wire the external connector, the firmware **debounces** D+/D− activity before starting the host; builds behave like **Bluetooth-only** adapters with no dependency on the port being present.

**Wii-only builds** (`-DOGXM_FIXED_DRIVER=WII`) still use the dedicated Wii USB host path on Core 1 as before; this section applies to **non-Wii** Pico W / Pico 2 W builds that include `CONFIG_EN_USB_HOST`.

---

## 7. Debug / UART logging (crash debugging)

To see where the firmware gets to when switching to Wii mode (or if it crashes), build in **Debug** so that **UART logging** is enabled. Logs go to **UART1** on **GP4 (TX)** and **GP5 (RX)** so they do not conflict with PIO USB on GP0/GP1. (GP10/11 are not valid UART pins on RP2040/RP2350.)

1. **Build Debug** for Pico 2 W: `cmake -G Ninja -DOGXM_BOARD=PI_PICO2W -DCMAKE_BUILD_TYPE=Debug <path-to-RP2040>` then `ninja`. Or use the build script with a Debug build directory.
2. **Connect a USB–UART adapter** (3.3 V) to **GP4 (TX)** and **GP5 (RX)** and GND. Open a terminal at **115200** baud.
3. **Flash** the Debug `.uf2`, switch to Wii mode, and watch the log. You should see: `PicoW init: start` … `driver inited`; `PicoW run: wii_mode=1` … `Core1 launched`; `WII Core1: entry` … `HostManager init done` … `tuh_init done` … `entering loop`; `PicoW run: Wii main loop tick`. The **last line printed** before a crash or hang shows how far the code got.

---

## 8. Troubleshooting

| Symptom | Likely cause | What to do |
|--------|----------------|------------|
| Controller vibrates, no LED, doesn’t sync | Host using native USB instead of PIO USB | Ensure Wii mode uses PIO USB (roothub 1) as in §2. |
| Same | D+ and D− swapped on cable | Set `PIO_USB_SWAP_DP_DM 1` in config and rebuild, or swap D+ and D− wires. |
| No response at all | Wrong port or no VBUS | Use the **external** USB (PIO USB) and ensure VBUS has 5 V. |
| Crash when switching to Wii | Debug with UART (GP4/5) | Build Debug, connect UART to GP4/5 @ 115200; see §7. |

---

This setup gives **Wii (Wiimote) output** with a **USB controller** on the Pico 2 W’s external USB host port, with the single Bluetooth radio reserved for the Wii connection.
