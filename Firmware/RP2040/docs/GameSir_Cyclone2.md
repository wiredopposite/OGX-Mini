# GameSir Cyclone 2

Physically tested support status for the **GameSir Cyclone 2** on OGX-Mini (this fork).

The controller is supported across:

* Direct wired USB
* GameSir 2.4 GHz USB dongle
* Direct Bluetooth (Pico W / Pico 2 W / RP2354)

Support varies slightly by transport. All personalities are owned by the dedicated **`GameSirCyclone2Host`** (USB) and related Cyclone Bluetooth helpers. Shared protocol helpers (e.g. `SwitchPro::*` / Bluepad32 Switch/DS4 parsers) may be reused; **do not patch Xbox / PS4 / Switch drivers for Cyclone-only quirks** when a dedicated personality path exists.

---

## Compatibility summary

| Input Mode  | Wired USB | 2.4 GHz Dongle | Direct Bluetooth |
| ----------- | --------: | -------------: | ---------------: |
| XInput      | ✅ Working |              — |                — |
| Switch      | ✅ Working |      ✅ Working |        ✅ Working |
| DS4         | ✅ Working |      ✅ Working |        ✅ Working |
| Android/iOS |         — |      ✅ Working |        ✅ Working |

```text
GAMESIR CYCLONE 2

WIRED
✓ XInput
✓ Switch
✓ DS4

2.4 GHZ DONGLE
✓ Switch
✓ DS4
✓ Android/iOS

DIRECT BLUETOOTH
✓ Switch
✓ DS4
✓ Android/iOS
```

Direct Bluetooth XInput is **not** supported (GameSir does not expose usable BT XInput). Use Switch, DS4, or Android/iOS over Bluetooth and let OGX output Xbox 360 / XInput (or any other device mode) independently.

---

## Transports

### Wired USB

```text
Cyclone 2
    ↓ USB
OGX Mini
```

### 2.4 GHz Dongle

```text
Cyclone 2
    ↓ proprietary 2.4 GHz
GameSir USB receiver
    ↓ USB
OGX Mini
```

The GameSir receiver handles the proprietary wireless link. OGX talks to the receiver as a USB input device. Document the dongle as a **separate transport** from direct wired USB even when it exposes similar protocol personalities.

### Direct Bluetooth

```text
Cyclone 2
    ↓ Bluetooth
Pico W / OGX Mini
```

Does **not** require the GameSir USB receiver. Requires a Bluetooth-capable board (**Pico W**, **Pico 2 W**, or **RP2354**).

---

## Output translation

Controller **input** mode and OGX **output** mode are independent. Examples:

```text
Cyclone Switch input
        ↓
OGX
        ↓
Xbox 360 output
```

```text
Cyclone DS4 Bluetooth input
        ↓
OGX
        ↓
Xbox 360 output
```

```text
Cyclone Android/iOS Bluetooth input
        ↓
OGX
        ↓
Xbox OG output
```

A controller does not need to use XInput as its input protocol for OGX to emulate an Xbox controller on the output side.

---

## Mode USB IDs (wired / 2.4 GHz receiver)

| Home LED | Mode | Typical USB ID | Notes |
|----------|------|----------------|--------|
| Green | PC / XInput | `3537:100B` (or `3537:1053`) | Working wired |
| Blue | DS4 | `054C:09CC` | Working wired and via dongle (session-affined) |
| Red | NS / Switch | `057E:2009` | Working wired and via dongle |
| Yellow | Android/iOS (internal: HID) | `3537:0575` | Working via dongle / BT; see idle-receiver note below |

Mode combos: `Home+X` XInput, `Home+Y` Switch, `Home+A` Android/iOS (HID), `Home+B` DS4. Wired / 2.4 GHz also cycle with **View + Menu**.

### Transport model (implementation)

```text
physical_driver = GAMESIR_CYCLONE2
transport       = WIRED | 2.4GHZ_RECEIVER | BLUETOOTH
mode            = XINPUT | DS4 | SWITCH | HID
```

User-facing **Android/iOS** is the yellow personality; firmware identifiers use **`HID`** / `HID_PASSIVE` where applicable. Do not rename those identifiers for docs alone.

- **`3537:0575` alone does not always mean a live gamepad.** An idle 2.4 GHz receiver can enumerate as this ID with the pad powered off. OGX claims it as Cyclone and may listen passively without mapping PadIn until a live personality is present.
- After `0575` in-session, remount as Switch / DS4 / HID is tagged `2.4GHZ_RECEIVER` and reuses the Cyclone protocol engines.
- Cold-plug green XInput without prior `0575` stays `WIRED`.

---

## Wired USB

| Mode   | Wired USB |
| ------ | --------- |
| XInput | Working   |
| Switch | Working   |
| DS4    | Working   |

### XInput (green) — Working

The controller operates with its XInput/PC personality (`3537:100B` / `3537:1053`). Normal input translates correctly through OGX.

Do not regress. Optional vendor `0F F2` / report `0x12` for L4/R4/M (XInput only).

### Switch (red) — Working

`Cyclone2SwitchWired` → `SwitchProHost` MinimalReportMode.

The Cyclone 2 physically uses an Xbox-style face-button layout while speaking Switch protocol. OGX applies Cyclone-specific face mapping when translating Switch input to Xbox-style output. **Do not remove this controller-specific mapping.**

Current working physical behavior when targeting Xbox / XInput output:

```text
Physical A -> Xbox A
Physical B -> Xbox B
Physical X -> Xbox X
Physical Y -> Xbox Y
```

### DS4 (blue) — Working

`Cyclone2Ds4Wired`. DS4-compatible wired mode translates correctly. No `0F F2`.

---

## GameSir 2.4 GHz Dongle

| Mode        | 2.4 GHz Dongle |
| ----------- | -------------- |
| Switch      | Working        |
| DS4         | Working        |
| Android/iOS | Working        |

### Switch via Dongle — Working

Confirmed. Reuses Cyclone Switch-compatible processing and the same controller-specific face mapping as wired Switch.

### DS4 via Dongle — Working

Confirmed.

### Android/iOS via Dongle — Working

Confirmed. User-facing name: **Android/iOS**. Internal designation: **HID** (`3537:0575` when presenting as that personality).

Receiver behavior notes (still useful for debugging):

| Situation | Expectation |
|-----------|-------------|
| Receiver in, pad OFF | Often `3537:0575`; no phantom PadIn while idle |
| Pad ON in a supported mode | Remount / link → Cyclone personality engine, input active |
| Pad OFF again | Slot waits; receiver stays monitored |
| Mode cycle on dongle | Re-detect personality; shared Cyclone parsers |

---

## Direct Bluetooth

Requires a Bluetooth-capable board. Put the controller in the desired Bluetooth personality, then pair to OGX.

| Mode        | Direct Bluetooth |
| ----------- | ---------------- |
| Switch      | Working          |
| DS4         | Working          |
| Android/iOS | Working          |
| XInput      | Not supported    |

### Bluetooth DS4 (blue) — Working

Connects as DS4-compatible Bluetooth. OGX receives input via Bluepad32 and may translate to any supported output driver independently of the input protocol.

Example:

```text
Cyclone 2
Bluetooth DS4 input
        ↓
OGX normalized input
        ↓
Xbox 360 / XInput output
```

Official advertising may use a shared name such as `Wireless Controller`. Cyclone BT helpers avoid claiming unrelated DualShock 4 pads solely by that name.

### Bluetooth Switch (red) — Working

Direct Bluetooth Switch personality — **not** via the GameSir USB dongle. Bluepad32 Switch parsing plus Cyclone-specific face-button handling when translating to Xbox-style output (same physical A/B/X/Y → Xbox A/B/X/Y goal as wired/dongle Switch).

### Bluetooth Android/iOS (yellow) — Working

Direct Bluetooth without the GameSir dongle. User-facing: **Android/iOS**. Internal: **HID** (e.g. strong name `GameSir-Cyclone 2`).

### Direct Bluetooth XInput — Not supported

GameSir does not support usable direct Bluetooth XInput. Advertising as **"Game Pair Mode"** is ignored / cleaned up (`Cyclone2BtProbe`) so it does not hang discovery.

Working wireless Xbox-style use case:

```text
supported Bluetooth input personality (Switch / DS4 / Android/iOS)
        ↓
OGX
        ↓
XInput / Xbox 360 output
```

---

## Implementation notes

- Dedicated host: `GameSirCyclone2Host` and personality drivers under `USBHost/HostDriver/GameSirCyclone2/`.
- Bluetooth probe / Game Pair Mode ignore / BT Switch face-map arm: `Cyclone2BtProbe`.
- Switch face remap for Xbox output is gated by Cyclone Switch-input active state (wired, dongle, and Bluetooth Switch paths).
- Enhanced GameSir XInput extras (`0F F2` / report `0x12`) apply to XInput personality only.
- Yellow USB `3537:0575` may be idle receiver **or** HID/Android presentation; treat carefully in logs and claim logic.

## Related

- [Wired Controllers](Wired_Controllers.md)
- [Adding supported controllers](Adding_Supported_Controllers.md)
