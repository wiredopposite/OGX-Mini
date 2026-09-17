# Victrix Gambit Tournament Controller

Wired USB support for the **Victrix Gambit Tournament Controller** / **Gambit Prime Wired Controller** on OGX-Mini (this fork).

```text
VICTRIX GAMBIT TOURNAMENT CONTROLLER

Connection:
✓ Wired USB

Protocol:
✓ Xbox GIP

Status:
✓ Fully working

Important:
Press the Home / Xbox button once after connecting the controller.
The controller may remain dark and inactive until Home is pressed.
```

| Field | Value |
|-------|--------|
| Connection | **Wired USB only** (no Bluetooth / 2.4 GHz) |
| Status | **Supported** — fully working |
| Protocol | Xbox GIP |
| Physical driver | `VictrixGambitHost` (`VICTRIX_GAMBIT`) |

---

## IMPORTANT: Wake with Home / Xbox after plug-in

> **NOTE:**  
> After connecting the Victrix Gambit to OGX Mini, press the controller's **Home / Xbox button once** to wake and initialize the controller.  
>  
> The controller may remain **unlit and inactive** immediately after being plugged in. **This is normal behavior** for this controller — not a firmware failure.  
>  
> After pressing Home, the controller completes its Xbox GIP connection and operates normally.

### Expected connection procedure

1. Connect the Victrix Gambit to the OGX Mini USB host port.
2. Wait for USB enumeration.
3. Press the Gambit's **Home / Xbox** button once.
4. The controller wakes and completes its GIP initialization.
5. The controller is ready for use.

There is no need to unplug/replug, reset OGX, hold Home, press multiple buttons, or change controller modes. A normal Home/Xbox press is sufficient.

### Troubleshooting

**Controller has no lights or input after plugging it in:**

Press the **Home / Xbox** button once.

The Gambit does not automatically become active immediately after USB connection. Once Home is pressed, it should complete initialization and begin working normally.

Only if pressing Home does not wake the pad should you check cable/port, firmware build, or UART logs (`physical_driver=VICTRIX_GAMBIT`).

---

## Compatibility

| Controller | Connection | Status | Notes |
| ---------- | ---------- | ------ | ----- |
| Victrix Gambit Tournament Controller / Gambit Prime | Wired USB | ✅ Working | Press Home/Xbox **once** after connecting to wake the controller |

---

## Physically verified hardware

| Field | Value |
|-------|--------|
| VID | `0E6F` |
| PID | `0250` |
| Manufacturer | Performance Designed Products |
| Product | Victrix Gambit Prime Wired Cont |
| bcdDevice | `0401` |

This exact hardware revision is **physically verified working**.

### Known USB IDs

| VID:PID | Notes |
|---------|--------|
| **`0E6F:0250`** | Victrix Gambit Prime Wired Controller — **physically verified** by OGX project testing |
| `0E6F:02D6` | Externally documented Gambit revision (e.g. SDL) — **not** physically tested here |

Do **not** claim other PDP / Victrix VID/PIDs. There is no broad `VID=0E6F` wildcard.

---

## Working features

Confirmed working for normal gamepad use:

| Controls | Status |
|----------|--------|
| A / B / X / Y | ✅ |
| D-pad | ✅ |
| LB / RB | ✅ |
| LT / RT | ✅ |
| L3 / R3 | ✅ |
| View | ✅ |
| Menu | ✅ |
| Home / Xbox button | ✅ |
| Left / right analog sticks | ✅ |
| Xbox GIP → OGX PadIn translation | ✅ |

**Share / paddles / rumble:** Share offset for `0250` remains capture-dependent; programmable rear paddles remap in hardware to normal buttons (no independent paddle bits yet). Dual-motor rumble uses shared GIP rumble — test on your build if needed. Audio / 3.5 mm is out of scope.

---

## Architecture

Dedicated physical driver — **not** an extension of `XboxOneHost`. Follows the project's dedicated-controller-driver rule.

```text
Victrix Gambit
      ↓
VictrixGambitHost (VICTRIX_GAMBIT)
      ↓
Xbox GIP protocol (shared tuh_xinput helpers)
      ↓
OGX normalized controller state
      ↓
selected OGX output mode
```

On bind, the input slot reports:

```text
transport=USB
driver=VICTRIX_GAMBIT
mode=XINPUT
protocol_engine=XBOX_GIP
```

---

## USB / protocol

Physically verified gamepad interface:

```text
Interface 0
Class:    0xFF
Subclass: 0x47
Protocol: 0xD0

IN endpoint:  0x81
OUT endpoint: 0x01
```

The controller also exposes a **secondary interface** (Interface 1, alternate settings) associated with audio. Normal gamepad translation uses **Interface 0 only**. Interface 1 is placed on alternate setting 0 during startup and is otherwise ignored (no headset/mic support).

---

## GIP discovery

After the controller is awakened (Home press), it performs the normal Xbox GIP discovery sequence. Physically observed:

- Device **ANNOUNCE** with embedded identity **VID=`0E6F` / PID=`0250`**
- Host **IDENTIFY** request; device returns a (possibly **chunked**) descriptor with ACK handling
- Gamepad class: `Windows.Xbox.Input.Gamepad`
- Controller-specific identity: `Victrix.Xbox.Gamepad.Gambit`
- Then gamepad bring-up (POWER / LED / AUTH) and normal **INPUT** (`0x20`)

The dedicated driver owns this discovery FSM and reuses shared GIP transport (sequence allocation, OUT completion, IN arm/re-arm, ACK).

Historical bring-up notes (enumeration dumps, ANNOUNCE/IDENTIFY/chunk/ACK sequencing, audio `SET_INTERFACE`) remain useful for maintenance; they describe **resolved** protocol work, not incomplete support.

---

## Intentionally deferred

| Feature | Notes |
|---------|--------|
| Rear paddles as independent buttons | User-programmable; firmware remaps to normal buttons. No independent paddle bits yet. |
| Function button | Local audio / paddle programming — not a separate gamepad button until captures prove otherwise. |
| Trigger rumble | Optional; dual-motor rumble via shared GIP rumble first. |
| Audio / 3.5 mm | Out of scope. |
| Share bit on `0250` | Confirm offset from idle vs pressed report capture before documenting as verified. |

---

## Related

- [Wired Controllers](Wired_Controllers.md)
- SDL reference: `SDL3/src/joystick/hidapi/SDL_hidapi_xboxone.c`
