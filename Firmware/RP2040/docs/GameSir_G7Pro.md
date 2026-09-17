# GameSir G7 Pro

Physically tested support for the **GameSir G7 Pro** on OGX-Mini (this fork).

```text
GAMESIR G7 PRO

Connection:
✓ Wired USB (HID, VID:PID 3537:1022)
✓ Classic Bluetooth (name: GameSir-G7 Pro)

Drivers:
✓ GameSirG7ProHost (USB)
✓ GameSirG7ProBt (Bluetooth)

Status:
✓ Supported

Important (USB):
Press the Home button once after connecting via USB
so the controller finishes connecting properly.
```

| Field | Value |
|-------|--------|
| Wired USB | **Supported** — dedicated `GameSirG7ProHost` (`3537:1022`) |
| Bluetooth | **Supported** — Classic BT via `GameSirG7ProBt` (claimed by name `GameSir-G7 Pro`; SDP VID/PID often times out) |
| Wired report | 9-byte HID (no report ID): buttons, hat, sticks, triggers |
| Bluetooth report | Report ID `0x07` (OpenMicro-style layout) |

---

## IMPORTANT: Press Home after USB connect

> **NOTE:**  
> After connecting the GameSir G7 Pro to the OGX Mini **over USB**, press the controller’s **Home** button once so it finishes connecting properly.  
>  
> Without that press, the pad may enumerate but not present usable input until Home is pressed. **This is expected** for this controller on the adapter.

### Expected wired connection procedure

1. Connect the G7 Pro to the OGX Mini USB host port (PC / XInput or HID mode on the pad).
2. Wait for USB enumeration.
3. Press **Home** once.
4. The controller is ready for use.

---

## Wired USB

- Dedicated host path — **do not** fold G7 quirks into DInput or shared Xbox drivers.
- Skips the vendor HID interface (`0xFFF0`); gamepad HID is used for input.
- Prefer **PC / XInput** or the HID personality the adapter already claims as `3537:1022`.

## Bluetooth (Pico W / Pico 2 W / RP2354)

- Put the controller in Bluetooth pairing mode; connect to the adapter (not a PC/phone bond first if the link fails).
- Claimed by Bluetooth name **`GameSir-G7 Pro`** when SDP VID/PID lookup times out.
- Uses Report ID **`0x07`**.

---

## Reference

- Host: `Firmware/RP2040/src/USBHost/HostDriver/GameSirG7Pro/`
- Descriptors: `Firmware/RP2040/src/USBHost/Descriptors/GameSirG7Pro.h`
- Wired list: [Wired_Controllers.md](Wired_Controllers.md)
- Main README: [Supported devices](../../../README.md#supported-devices)
