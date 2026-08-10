# Controller mapping capture (`controller_capture.py`)

Interactive helper for **OGX-Mini** users who want a **new gamepad** supported. It records how your PC sees the controller through **SDL / pygame** (button indices, axis indices, hat values, SDL GUID) and **USB VID:PID**, then writes a **single text file** you can attach to an issue or send to a maintainer.

**USB and Bluetooth** are both supported: if the OS exposes the pad to SDL (pair Bluetooth in system settings first), it will appear in the list.

### Nintendo Switch 2 Pro / Joy-Con 2 (USB)

**Switch 2–family** pads (e.g. **Pro Controller 2**, **Joy-Con 2**) often **do not show up in SDL** on a PC until a host sends Nintendo’s **USB bulk** bring-up sequence on **interface 1** (same approach as [HandHeldLegend’s ProCon 2 Enabler](https://github.com/HandHeldLegend/handheldlegend.github.io) — `procon2tool`).

In this tool, pick the platform:

**“Nintendo Switch 2 Pro / Joy-Con 2 (USB — libusb handshake, then capture)”**

The script will run **`switch2_usb_init.py`** (PyUSB + **libusb**) first, then start pygame so you can map controls. You still need **PyUSB** and OS access to the device (Linux: `libusb-1.0` + permissions; Windows: may need **WinUSB** on the bulk interface via Zadig).

This does **not** flash firmware; it only runs on your computer.

## Requirements

- **Python 3.10+**
- `pygame` 2.x  
- **Windows, macOS, or Linux** (same script; VID/PID auto-detection is only attempted on Linux)

```bash
cd Tools/controller_capture
python3 -m pip install -r requirements.txt
python3 controller_capture.py
```

On Windows you may use `py` instead of `python3`.

Optional CLI flags:

- `python3 controller_capture.py -o my_report.txt` — set the output path up front (`-o` wins over auto-naming).
- `python3 controller_capture.py --use-sdl-name` — use the **SDL joystick name** as `controller_full_name` (no typing).
- `python3 controller_capture.py --auto-output-name` — save as **`<sanitized_name>_<timestamp>.txt`** without filename prompt (implies save; only when `-o` is not set).
- Combine: `python3 controller_capture.py --use-sdl-name --auto-output-name` for a quick non-interactive name + file.
- **`python3 controller_capture.py --raw-hid-only`** — **Linux only**: skips SDL/pygame entirely; runs only the **hidraw** hex capture (pick the same platform layout for prompts; use the **Switch 2 USB** platform if you need the bulk handshake first). Pair with `--auto-output-name` to save without prompts.

Interactive shortcuts (without flags):

- At the controller name prompt, **press Enter** to use the SDL-reported name.
- When saving, the suggested default filename is **`<sanitized controller name>_<YYYYMMDD_HHMMSS>.txt`** (press Enter to accept).

## What you’ll do

1. Choose the **platform layout** your controller matches (Xbox, PlayStation, Switch, etc.).
2. Pick the **controller** from the list SDL sees (**USB cable or paired Bluetooth**).
3. Enter the **full product name** of the pad (box / manufacturer name), **or press Enter** to use the SDL name, **or** run with **`--use-sdl-name`**. The report records the source (`controller_full_name_source` in JSON).
4. For each on-screen prompt, press **only** that button or move that stick direction. After each capture, **release everything and re-center the sticks** — the script waits for a neutral pose before the next prompt so it doesn’t pick up the previous control again.
5. At the end, follow the **on-screen steps** to find **VID and PID** for your OS (Windows / macOS / Linux). On Linux, values may be pre-filled; you can confirm or override.
6. Save the generated **`.txt` file** (default suggestion is derived from the controller name) and upload it when requesting support.

On **Linux**, after SDL mapping the tool will ask whether to capture **raw HID input reports** from **`/dev/hidraw*`** (full report bytes in hex, per control, with an XOR diff vs idle). **Say yes** when asking for firmware support — that data matches what embedded code decodes (not SDL indices alone).

On **Linux**, after VID/PID is known the tool asks whether to record **raw HID** **alongside pygame**: after each SDL capture it reads the latest **hidraw** packet, so **pygame decides when** you pressed (no IMU/timer false triggers). Those fields live on each binding: `hidraw_baseline_hex`, `hidraw_active_hex`, `hidraw_xor_hex`. Use **`--no-hidraw-sidecar`** to skip that prompt.

The file includes both **human-readable** lines and a **JSON** block (`schema_version` **9**) with the same data for tooling. When the **Linux hidraw sidecar** is enabled, each binding includes **full** `hidraw_*_hex` strings plus lengths; the text report also has a **“Full hidraw packets”** section (16 bytes per line) so you do not need a separate dump script for that run. **`capture_mode`** is `full` (SDL, optional raw) or `raw_hid_only`. **`hidraw_gated_by_sdl`** summarizes the sidecar path. Optional **`raw_hid_reports`** is from the separate interactive hidraw walkthrough. **`controller_full_name_source`** is `sdl`, `user_entered`, or `hidraw` (raw-only). Use **`hidraw_full_report_dump.py`** only if you want a **live** stream unrelated to the SDL prompts.

Use **`python3 controller_capture.py --skip-raw-hid`** if you only want SDL/VID capture and no hidraw walkthrough.

## Finding which HID bytes a control uses (for firmware)

When SDL indices are not enough (common on **Switch 2 Pro USB**), you need the **`hidraw_*` hex** on each binding.

1. **Capture** with **Linux + SDL+hidraw sidecar** (or raw-hid flow) so every binding has `hidraw_baseline_hex` and `hidraw_active_hex` (same length, typically **64** bytes for full reports).
2. **XOR** baseline and active byte-for-byte. Any index where the result is **non‑zero** changed between “idle” and “pressed” for that sample.
3. **Ignore index 1** when reasoning about buttons: it is usually the **USB report timer** and toggles every frame.
4. **Start with indices 2–12** (report id + timer + the **10‑byte Nintendo “core”**: battery, three button bytes, six stick bytes). Many controls only touch those. If two different physical controls share the same XOR pattern, you need **more bytes** in the signature or a different test (see below).
5. **Sticks share bytes with some digitals** on Switch 2–style reports: the same byte may encode stick data *and* a button transition. Always compare your candidate XOR to **every axis binding** in the same file (`axis_lstick_*`, `axis_rstick_*`). If an axis **active** report matches the same **(byte21, byte22, …)** pattern as a button, a firmware decoder cannot use that short prefix alone.
6. **Indices 13+** often include **IMU / vendor fields** that move every frame even at rest. Treat them as **high risk** for ghost inputs unless you prove they are stable when idle on real hardware, or you use **relative** decoding (difference from a calibrated neutral) plus debouncing.
7. **Baselines differ per prompt**: each binding’s `hidraw_baseline_hex` was taken when you were “neutral” for *that* step, so absolute values at index 21+ are **not** comparable across rows—**XOR within the same row** is the reliable operation.

**Helper script** (prints XOR indices and `active[21:24]` for collision spotting):

```bash
python3 analyze_hidraw_xor.py Nintendo_Switch_2_Pro_Controller_YYYYMMDD_HHMMSS.txt --skip-timer
```

**Live full-report dump** (Linux): stream every hidraw read as one hex line so you can press **one** button at a time and see which indices change, separate from the interactive mapper:

```bash
cd Tools/controller_capture
python3 hidraw_full_report_dump.py --vid 057e --pid 2069
# or: python3 hidraw_full_report_dump.py --path /dev/hidraw4
# only print when something changes:
python3 hidraw_full_report_dump.py --vid 057e --pid 2069 --diff
# ignore pure IMU tail jitter; print when “core” bytes 2:12 (masked) change:
python3 hidraw_full_report_dump.py --vid 057e --pid 2069 --diff-core
```

Redirect to a file if you want a log: `… > dump.txt`. Compare **idle** vs **held** lines by eye or `diff`; bytes that **always** change every line even at rest are not good solo signatures.

After you identify a safe **mask + value** (or multi-byte signature), add it to the host driver and **verify on hardware** with sticks moved to extremes while tapping the control.

## What maintainers use this for

- **`capture_mode`**: `full` (SDL bindings + optional raw) vs `raw_hid_only` (hex only, no pygame indices).
- **`controller_full_name`**: human-readable name for changelog / Wired Controllers lists / issue tracking (retail name, SDL name, or hidraw `HID_NAME`).
- **`controller_full_name_source`**: `sdl`, `user_entered`, or `hidraw`.
- **`connection_guess`**: on Linux, best-effort `usb` / `bluetooth` / `unknown` from sysfs (optional context).
- **`HardwareIDs.h`**: `{ VID, PID }` entries for the right host driver (DInput, PS3, PS4, …).
- **SDL name/GUID**: cross-check with other databases and real device names.
- **Binding table**: map logical controls to **button / axis / hat indices** as the host stack reports them (cross-check vs other hosts).
- **Per-binding `hidraw_*` fields** (Linux, SDL+hidraw sidecar): raw bytes when pygame saw the control — preferred over the standalone walkthrough for Nintendo pads.
- **`raw_hid_reports`** (optional second phase): interactive hid-only capture — use **`xor_hex`** if you need it; often redundant if sidecar succeeded.

## Limitations

- **SDL / pygame mapping alone is not enough to write a host driver.** Drivers need **full HID report** byte/bit/axis layouts. Use **`hidraw_full_report_dump.py`**, Linux hidraw hex, USB capture, or on-device Debug UART dumps — see [Adding_Supported_Controllers.md](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md#step-2--capture-full-hid-reports-required-for-driver-mapping).
- Captures **what your OS + SDL expose** for the mapping table. If the device is in the wrong mode (e.g. XInput vs DInput), unplug/replug or toggle mode per the manual and run again.
- **Bluetooth** must be **paired and connected** in the OS before running; if capture is unstable, try **USB** for the same mapping run.
- **Raw hidraw** capture is **Linux-only** (needs a matching `/dev/hidraw` for your VID:PID and read access). On Windows/macOS, use Linux for the hex dump or a USB analyzer; the SDL mapping file is still useful alone for names/VID.

## Troubleshooting

The script prints tips if inputs don’t register (Steam Input, USB ports, drivers, Linux permissions, Bluetooth pairing, etc.). See also the main repo **[Wired Controllers](../../Firmware/RP2040/docs/Wired_Controllers.md)** doc and the full firmware guide **[Adding supported controllers](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md)**.
