# Flydigi APEX 4 Elite — Black Myth: Wukong Edition

Physically tested support status for the **Flydigi APEX 4 Elite Gaming Controller — Black Myth: Wukong Edition** on OGX-Mini (this fork).

Status wording:

| Term | Meaning |
|------|---------|
| **Supported** | Everything tested for that connection/mode works. |
| **Supported with limitation** | Normal use works; a documented control or function is unavailable. |
| **Untested / Unknown** | Required hardware was not available; compatibility is unknown. |

Do **not** treat “Supported with limitation” as fully supported.

---

## Summary

| Controller | Connection | Mode | Status | Known limitations |
|------------|------------|------|--------|-------------------|
| Flydigi APEX 4 Black Myth: Wukong | Wired USB | PC | Supported | None known |
| Flydigi APEX 4 Black Myth: Wukong | Bluetooth | Android | Supported with limitation | Home button does not work |
| Flydigi APEX 4 Black Myth: Wukong | Bluetooth | iOS | Supported with limitation | Home button does not work |
| Flydigi APEX 4 Black Myth: Wukong | Bluetooth | PC | Supported | None known |
| Flydigi APEX 4 Black Myth: Wukong | Bluetooth | Switch | Supported with limitation | Back Remap Buttons do not work |
| Flydigi APEX 4 Black Myth: Wukong | 2.4 GHz USB dongle | — | Untested / Unknown | Dongle unavailable during implementation |

---

## Wired USB

### PC Mode — Supported

Tested over a direct wired USB connection in **PC Mode**.

Working:

- Left Stick / Right Stick
- D-pad
- A / B / X / Y
- LB / RB
- LT / RT
- L3 / R3
- Start / Menu
- Back / View
- Home
- Back Remap Buttons
- Normal controller input

**Result:** Everything working.

Firmware uses a dedicated wired host path for this edition (`FlydigiApex4WukongHost`). See also [Wired Controllers](Wired_Controllers.md).

---

## Wireless — Bluetooth

Requires a Bluetooth-capable board (**Pico W**, **Pico 2 W**, or **RP2354**). Put the controller in the desired Bluetooth mode, then pair to the adapter (do not assume PC/phone pairing first).

### Android Mode — Supported with limitation

Working:

- Left Stick / Right Stick
- D-pad
- A / B / X / Y
- LB / RB
- LT / RT
- L3 / R3
- Start / Menu
- Back / View
- Back Remap Buttons
- Other normal controller input

Not working:

- **Home button**

Notes: Connects and plays normally. Rear/back remappable buttons work. Only known limitation: Home.

### iOS Mode — Supported with limitation

Working:

- Left Stick / Right Stick
- D-pad
- A / B / X / Y
- LB / RB
- LT / RT
- L3 / R3
- Start / Menu
- Back / View
- Back Remap Buttons
- Other normal controller input

Not working:

- **Home button**

Notes: Connects and plays normally. Rear/back remappable buttons work. Only known limitation: Home.

### PC Mode — Supported

Tested using the controller’s Bluetooth **PC Mode**.

Working:

- Left Stick / Right Stick
- D-pad
- A / B / X / Y
- LB / RB
- LT / RT
- L3 / R3
- Start / Menu
- Back / View
- Home
- Back Remap Buttons
- Normal controller input

**Result:** Everything working.

### Switch Mode — Supported with limitation

Working:

- Left Stick / Right Stick
- D-pad
- A / B / X / Y
- LB / RB
- LT / RT
- L3 / R3
- Start / Menu
- Back / View
- Home
- Other normal controller input

Not working:

- **Back Remap Buttons**

Notes: Connects and otherwise functions correctly. Rear/back remappable buttons are not currently exposed or translated. Everything else tested works.

---

## 2.4 GHz USB dongle — Untested / Unknown

**2.4 GHz USB Dongle: Untested — the wireless dongle was not available during implementation, so compatibility is currently unknown.**

- Do not list 2.4 GHz as supported.
- Do not list 2.4 GHz as unsupported.
- No dedicated support work was performed for this transport.

---

## Related docs

- [Wired Controllers](Wired_Controllers.md) — wired USB lists
- [Main README — Supported devices](../../../README.md#supported-devices)
- [Adding supported controllers](Adding_Supported_Controllers.md)
