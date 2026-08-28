# `Tools/controller_capture` (legacy PC helpers)

> **Not accepted for mapping support at this time.**  
> Scripts in this folder — including **`controller_capture.py`** and **`hidraw_full_report_dump.py`** — **do not produce information that can be used** to map a controller into OGX-Mini firmware. **Do not submit** their output for new-pad or wrong-button issues or PRs.  
> **Accepted capture:** on-device **Debug UART** full report hex from the adapter’s USB host port. See [Adding supported controllers](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md#step-2--capture-reports-on-the-adapter-required-for-driver-mapping).

These tools remain in the repo for **local experimentation** or historical reference. They are **not** part of the supported workflow for opening mapping issues or requesting maintainer support.

---

## Scripts in this folder

| Script | Purpose | Submissions |
|--------|---------|-------------|
| **`controller_capture.py`** | Interactive SDL/pygame mapping + optional Linux hidraw sidecar | **Not accepted** |
| **`hidraw_full_report_dump.py`** | Live Linux hidraw hex stream on a PC | **Not accepted** |
| **`analyze_hidraw_xor.py`** | XOR helper for PC capture files | **Not accepted** (same source data) |
| **`switch2_usb_init.py`** | Switch 2 USB bulk bring-up (used internally / dev) | N/A |

---

## `controller_capture.py` (interactive — not for submissions)

Records how your **PC** sees the controller through **SDL / pygame** (button indices, axis indices, hat values, SDL GUID) and **USB VID:PID**, then writes a **single text file**. May help you find **VID/PID** locally — **not** for firmware mapping submissions.

**USB and Bluetooth** are both supported: if the OS exposes the pad to SDL (pair Bluetooth in system settings first), it will appear in the list.

### Nintendo Switch 2 Pro / Joy-Con 2 (USB)

**Switch 2–family** pads often **do not show up in SDL** on a PC until a host sends Nintendo’s **USB bulk** bring-up sequence on **interface 1** (same approach as [HandHeldLegend’s ProCon 2 Enabler](https://github.com/HandHeldLegend/handheldlegend.github.io) — `procon2tool`).

Pick platform **“Nintendo Switch 2 Pro / Joy-Con 2 (USB — libusb handshake, then capture)”**. The script runs **`switch2_usb_init.py`** (PyUSB + **libusb**) first, then pygame. This does **not** flash firmware; it only runs on your computer.

### Requirements

- **Python 3.10+**
- `pygame` 2.x  
- **Windows, macOS, or Linux**

```bash
cd Tools/controller_capture
python3 -m pip install -r requirements.txt
python3 controller_capture.py
```

Optional CLI flags: `-o`, `--use-sdl-name`, `--auto-output-name`, `--raw-hid-only`, `--skip-raw-hid`, `--no-hidraw-sidecar` — see script `--help`.

**Do not upload** the generated `.txt` / JSON for mapping support.

---

## `hidraw_full_report_dump.py` (not for submissions)

Linux-only live stream of **`/dev/hidraw*`** packets on a **PC**. PC hidraw is **not** equivalent to report buffers on the OGX-Mini USB host stack (init, interfaces, and timing differ). **Logs from this script cannot be used** for mapping issues or PRs at this time.

The script may still be useful on your own machine for rough exploration; for firmware work, capture on the **adapter** instead (Debug UART — [Adding supported controllers](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md)).

---

## What to use instead

1. Build **Debug** firmware for your board (`./scripts/build.sh` → Debug).
2. Plug the pad into the **OGX-Mini USB host port**.
3. Log **full report hex** from `process_report` on the adapter ([Step 4](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md#step-4--add-temporary-full-report-logging-on-the-adapter)).
4. Attach that UART log to your issue or PR.

For init / multi-interface pads, a **USB analyzer** on the **adapter host port** may supplement UART capture.

---

## Limitations (all PC scripts)

- **Output from this folder is not accepted** for mapping or new-controller support submissions.
- **SDL / PC hidraw ≠ adapter-side reports** — OGX host drivers parse what TinyUSB delivers on the Pico/RP2350 host port.
- Wrong pad **mode** (XInput vs DInput vs Switch) on PC may not match adapter behavior.
- **Bluetooth** on PC requires OS pairing; adapter BT path may differ.

## Troubleshooting

The interactive script prints tips if inputs don’t register (Steam Input, USB ports, drivers, Linux permissions, Bluetooth pairing, etc.). For firmware support policy, see **[Wired Controllers](../../Firmware/RP2040/docs/Wired_Controllers.md)** and **[Adding supported controllers](../../Firmware/RP2040/docs/Adding_Supported_Controllers.md)**.
