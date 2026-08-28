# Wii Mode Guide

This guide covers **Wii mode** on OGX-Mini when using a **Pico W** or **Pico 2 W**. In this mode the adapter appears as a **Nintendo Wiimote** over Bluetooth to a Wii or Wii U, while your **input** comes from a **USB gamepad** plugged into the board’s **external USB host port**.

**Wii mode is build-option only:** you cannot switch into or out of Wii with button combos. Build with `-DOGXM_FIXED_DRIVER=WII` to get a Wii-only firmware; combos are disabled in that build.

For hardware wiring and USB port setup (PIO USB, pins, build), see [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md).

---

## 1. How Wii mode works

- The adapter’s **Bluetooth** is used only for the **Wiimote** link to the console (Wii or Wii U).
- A **USB controller** must be connected to the **external USB host port** (PIO USB on GPIO). The built-in USB port is for programming and is **not** used for the gamepad in Wii mode.
- The firmware converts USB gamepad input into Wiimote (and extension) reports and sends them over Bluetooth. The console sees a normal Wiimote; you control it with your USB pad.

---

## 2. USB connection (controller input)

- **Use the external USB port** (PIO USB). On Pico W / Pico 2 W this is the connector wired to **D+/D− on GP0/GP1** (or swapped per board config). See [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md) for pins and wiring.
- **Do not** plug the gamepad into the Pico’s **built-in** USB port (the one used for flashing). That port is not used as host in Wii mode.
- Power the board and plug the USB controller into the external port. When it enumerates, the controller’s LED (if any) should indicate it’s active. The firmware waits for a controller before starting the Wiimote connection flow.

---

## 3. Connecting to the Wii / Wii U

### First-time sync

1. **Build** the firmware with Wii mode: `-DOGXM_FIXED_DRIVER=WII` (see [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md) for full build steps).
2. Power the adapter and plug in the USB controller on the **external** port.
3. On the **Wii or Wii U**: open **Wii Remote** (sync) from the system menu.
4. The adapter appears as **“Nintendo RVL-CNT-01”**. When the console asks to press **Sync**, trigger sync on the **adapter** (or follow on-screen instructions). The adapter may also **auto-connect** if it was previously paired (see below).
5. After pairing, the console treats the adapter as player 1’s Wiimote; input comes from the USB gamepad.

### Auto-connect (after first sync)

- The adapter **saves the last paired console’s Bluetooth address** in flash.
- On later power-ups it will **automatically try to connect** to that console (like a normal Wiimote). You do **not** need to press Sync again unless you want to pair a different console or re-pair after the console “forgot” the remote.
- If auto-connect fails (e.g. authentication error because the console removed the remote), the saved address is cleared and the adapter stays **discoverable** so you can sync again from the Wii/U menu.

---

## 4. Wii report modes (No Extension, Nunchuk, Classic)

The adapter can emulate three Wiimote “shapes” that many games support. You **cycle** between them with a button combo; the **LED flashes** to show the current mode after a change.

| Mode              | LED flashes | Description |
|-------------------|------------|-------------|
| **No Extension**  | 1          | Wiimote only (can be used horizontal/sideways). |
| **Nunchuk**       | 2          | Wiimote + Nunchuk (stick, C, Z). |
| **Classic**       | 3          | Classic Controller (two sticks, triggers, full button set). |

- **Default at boot:** **No Extension** (1 flash when you switch to it).
- **Cycle:** Hold **Home (Guide) + D-pad Down** for about **1 second** (10 frames). The mode advances in order: **No Extension → Nunchuk → Classic → No Extension** (repeat). Each time you change mode, the LED flashes 1, 2, or 3 times to indicate which mode you’re in.

---

## 5. Button and stick mapping by mode

Mappings assume an **Xbox-style** USB gamepad (A/B/X/Y, Start, Back, Guide, D-pad, bumpers, triggers, two sticks). Other USB pads are mapped to the same logical layout by the host driver.

For the **full mapping reference** (all output modes, Bluetooth inputs including **Wii Remote vertical**, Switch Pro swaps, PS3/PS4, etc.), see **[Controller_Mappings.md](Controller_Mappings.md)**.

### No Extension (Wiimote only)

| Wiimote     | USB gamepad   |
|------------|----------------|
| A, B       | A, B           |
| 1, 2       | X, Y           |
| +, −       | Start, Back    |
| Home       | Guide (SYS)    |
| D-pad      | D-pad          |
| IR pointer | Left stick     |
| Sideways   | Home + LB (toggle horizontal mode) |

### Nunchuk (Wiimote + Nunchuk)

| Wiimote     | USB gamepad   |
|------------|----------------|
| A, B, 1, 2, +, −, Home, D-pad | Same as above |
| IR pointer | **Right stick** |
| Nunchuk stick | **Left stick** |
| C         | Left trigger &gt; half |
| Z         | Right bumper (RB) |
| Shake     | Left bumper (LB) held |

### Classic Controller

| Classic     | USB gamepad   |
|------------|----------------|
| A, B, X, Y | A, B, Y, X (X↔Y) |
| +, −, Home | Start, Back, Guide |
| D-pad      | D-pad          |
| L/R bumpers | LB, RB        |
| L/R triggers | Left/right triggers |
| Left stick  | Left stick    |
| Right stick | Right stick (also used for IR pointer) |

In all modes, **IR pointer** and sticks use a small **deadzone** so the cursor or character doesn’t drift when the stick is at rest.

---

## 6. Supported controllers for input (USB host)

In Wii mode the adapter uses the **USB host** on the external port and supports the same controllers as in other OGX-Mini host modes. Typically tested/supported USB gamepads include:

- **Xbox One** (wired USB)
- **Xbox 360** (wired USB)
- **Xbox 360 Wireless** (with USB receiver)
- **Generic Xinput / DirectInput (DInput)** pads
- **PlayStation** (PS3, PS4, PS5) over USB, when supported by the host stack
- **Nintendo Switch Pro** (wired USB), when supported
- Other HID gamepads that enumerate as a single gamepad

Only **one** controller is used (player 1). It must enumerate on the **external** PIO USB port. If your pad is not recognized, check wiring and [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md) (e.g. D+/D− swap).

---

## 7. Wii is build-option only (no combos)

Wii mode is **not** selectable by button combo. To use Wii you must **build** with:

- **`-DOGXM_FIXED_DRIVER=WII`**

Combos are **disabled** in Wii builds (you cannot switch to another mode at runtime). To use a different mode (Switch, XInput, etc.), reflash a firmware built without `OGXM_FIXED_DRIVER` or with a different fixed driver.

---

## 8. Summary

| Topic | Detail |
|-------|--------|
| **Input** | USB gamepad on **external** USB port (PIO USB). |
| **Output** | Wiimote over Bluetooth to Wii / Wii U. |
| **Modes** | No Extension (1 flash), Nunchuk (2), Classic (3). Cycle with **Home + D-pad Down** (~1 s). |
| **Sync** | First time: sync from console. After that, adapter auto-connects to last console. |
| **Wii build** | Build with `-DOGXM_FIXED_DRIVER=WII`; no combos to switch mode. |

For wiring, pins, and build instructions, see [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md).
