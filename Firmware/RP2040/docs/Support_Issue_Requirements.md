# Opening a support issue — required information

**Read this before opening a GitHub issue.**

Issues that omit the details below **may be closed without investigation**, or **support may be delayed** until the missing information is provided. Incomplete reports force guesswork; maintainer time is limited and reserved for reproducible, in-scope problems.

Also read the [Support policy](../../../README.md#support-policy) in the main README. Out-of-scope boards or third-party controllers without donated/shipped hardware are routinely closed as out of scope even when details are complete.

**Related**

| Topic | Document |
|-------|----------|
| Support policy (controllers / boards) | [README — Support policy](../../../README.md#support-policy) |
| Adding a controller yourself | [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) |
| Wired pad lists | [Wired_Controllers.md](Wired_Controllers.md) |
| Full HID report capture | [Tools/controller_capture](../../../Tools/controller_capture/README.md) |

---

## Incomplete information — consequences

| Situation | Likely outcome |
|-----------|----------------|
| Missing board, firmware version, or clear steps to reproduce | Issue **closed** or left waiting with a request for details (**delayed**) |
| Vague “doesn’t work” with no expected vs actual behavior | **Closed** or delayed until clarified |
| Third-party / unsupported pad with no donation or shipping agreement | **Closed** as out of scope |
| Board not on the maintainer-tested list, asking for a fix without a PR | Directed to **clone → fix → PR**; issue may be **closed** |
| Controller bug with no VID/PID (when USB) and no mode | Delayed or closed until IDs and mode are supplied |
| Mapping / “wrong buttons” with no full HID report dumps | Delayed until [full reports](Adding_Supported_Controllers.md#step-2--capture-full-hid-reports-required-for-driver-mapping) are attached |

Providing everything in the first post is the fastest path to a useful reply.

---

## Before you open an issue

1. Confirm your **board** and **controller** are in scope under [Support policy](../../../README.md#support-policy).
2. Check [open and closed issues](https://github.com/MegaCadeDev/OGX-Mini-2026/issues) for duplicates.
3. Try a **known-good** path if possible (e.g. another first-party pad, Release build, default output mode).
4. If you can fix it yourself (especially on non-maintainer boards), prefer a **pull request** over an issue asking for untested work.

---

## Required for every support issue

Fill in **all** of the following. Use the copy-paste template at the bottom.

### 1. Summary

- One-line title of the problem (what fails, on what).

### 2. Hardware

| Field | Required |
|-------|----------|
| **Board** (exact model) | Yes — e.g. Adafruit Feather USB Host, Pico W, Pico 2 W, Waveshare RP2350-USB-A. Do not say only “Pico” if it is a W / 2 W / Zero / clone. |
| **Board revision / seller** | If not an official Pi / Adafruit / Waveshare unit, say so (clones are unsupported). |
| **How the controller connects** | Wired USB host port / Bluetooth / other |
| **Host / console** | PC OS, Xbox, Switch, PS3, Steam Deck, etc. |

### 3. Firmware

| Field | Required |
|-------|----------|
| **Firmware version** | Yes — release tag, UF2 name, or git commit hash |
| **Where you got the UF2** | Release asset / self-built / someone else’s build |
| **Build type** | Release or Debug |
| **`OGXM_BOARD` / fixed driver** | If self-built: board CMake value; if fixed mode, `OGXM_FIXED_DRIVER=…` |
| **Output mode in use** | e.g. XInput, OG Xbox, PS3, Switch, STEAM, PS1/PS2 GPIO, … |

### 4. Controller (input)

| Field | Required |
|-------|----------|
| **Exact retail / model name** | Yes |
| **First-party or third-party** | Yes |
| **Connection** | USB wired / Bluetooth Classic / BLE / wireless dongle |
| **Controller mode** | XInput / DInput / Switch / Android / macOS / etc. (multi-mode pads **must** state the mode used) |
| **VID and PID** (USB) | Yes for wired USB — hex, e.g. `054C:0CE6`. How to find: [Adding_Supported_Controllers — VID/PID](Adding_Supported_Controllers.md#step-1--get-vid-and-pid) |
| **Works on a PC without the adapter?** | Yes / no / N/A — briefly |

### 5. Problem description

| Field | Required |
|-------|----------|
| **Expected behavior** | Yes |
| **Actual behavior** | Yes |
| **When it started** | Always / after update / after mode change / intermittent |
| **Frequency** | Always / sometimes (how often) |

### 6. Steps to reproduce

Numbered steps another person can follow from power-on to the failure. Include:

1. Flash / power sequence  
2. Output mode selection (combo or fixed build)  
3. Plug / pair order (adapter to console vs pad to adapter)  
4. Exact actions that trigger the bug  

### 7. What you already tried

List briefly: other modes, other pads, other cables/hubs, Release vs Debug, web-app settings, etc.

---

## Required when the issue is about input mapping, wrong buttons, or a new pad

Attach **full HID report** evidence — SDL / “gamepad tester” screenshots alone are **not** enough.

- Prefer a log from `Tools/controller_capture/hidraw_full_report_dump.py` (Linux), **or**
- Debug firmware UART dumps of the **full** report hex while pressing one control at a time  

See [Adding_Supported_Controllers — full reports](Adding_Supported_Controllers.md#step-2--capture-full-hid-reports-required-for-driver-mapping).

Without full reports, mapping bugs and “add this controller” requests are likely to be **delayed or closed**.

---

## Strongly recommended attachments

Provide what applies; more evidence = faster triage.

| Evidence | When |
|----------|------|
| Photo of the board (and USB host wiring if PIO USB / Feather) | Wiring / “not detecting” |
| UART log from a **Debug** build | Mount failures, disconnects, BT pairing |
| Video (short) of the failure | Timing / disconnect / LED behavior hard to describe |
| `lsusb` / Device Manager Hardware Ids | VID/PID disputes |
| Capture file from `controller_capture` **plus** full report dump | New controller / wrong mapping |
| GitHub commit or release link | Version clarity |

Debug UART pins and baud: [Adding_Supported_Controllers — UART](Adding_Supported_Controllers.md#uart-pins-and-baud) (**115200**; Pico W / 2 W: **GP4/GP5**; most other boards: **GP0/GP1**).

---

## Bluetooth-specific extras

If the pad is wireless on Pico W / Pico 2 W / RP2354:

- Controller was paired to a phone/PC before? (yes/no — and did you forget/unpair?)  
- LED behavior on the pad and on the board  
- Distance / 2.4 GHz interference notes if relevant  
- Whether the same pad works **wired** into the adapter’s USB host  
- Debug UART lines around connect / disconnect (`[BP32 …]`, `BT:`, `SW2:`, etc.)

---

## Feature requests / new board or controller support

- State whether you can **test a PR** yourself.  
- For maintainer-led work on unsupported hardware: confirm [Support policy](../../../README.md#support-policy) (donation earmarked for purchase, or shipping agreement).  
- Prefer implementing it and opening a **pull request** — see [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md).

Issues that only say “please add X” with no hardware path and no PR are often closed.

---

## Copy-paste issue template

```markdown
### Summary
<!-- One sentence -->

### Board
- Model:
- Official / clone:
- USB host connection (Feather Type-A / PIO pins / RP2350-USB-A / …):

### Firmware
- Version / commit / UF2 name:
- Source (release / self-built):
- Build type (Release / Debug):
- OGXM_BOARD (if self-built):
- Fixed driver (if any):
- Output mode in use:

### Input controller
- Name / model:
- First-party / third-party:
- Wired USB / Bluetooth / dongle:
- Mode (XInput / DInput / Switch / …):
- VID:PID (USB, hex):
- Works on PC without adapter? (yes/no):

### Expected behavior


### Actual behavior


### Steps to reproduce
1.
2.
3.

### Frequency
Always / intermittent (describe):

### Already tried


### Attachments
- [ ] Full HID report dump or Debug UART hex (if mapping / new pad / wrong buttons)
- [ ] UART log (if connect / disconnect / init)
- [ ] Photos / video (optional)
- [ ] Other:

### Scope check
- [ ] I read the Support policy
- [ ] My board is maintainer-supported **or** I am opening this for discussion / will submit a PR
- [ ] My controller is first-party **or** I have arranged donation / shipping **or** I will implement support myself
```

---

## After you file

- Watch for maintainer questions and answer with the same level of detail.  
- If asked for logs or full reports, attach them in a follow-up — do not reply “it still doesn’t work” alone.  
- Issues left without the requested information may be **closed**. You can reopen (or open a new issue) once the checklist is complete.
