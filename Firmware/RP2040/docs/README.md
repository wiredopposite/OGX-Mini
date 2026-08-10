# Documentation index

Project overview, support policy, platforms, and build instructions live in the [main README](../../README.md).

This folder holds **firmware user guides**, **references**, and **contributor** docs. Research / planning notes are under [`docs/`](../../../docs/README.md) at the repo root.

---

## Support & contributing

| Document | Description |
|----------|-------------|
| [Support_Issue_Requirements.md](Support_Issue_Requirements.md) | **Required details** for GitHub issues. Incomplete reports may be **closed or delayed**. |
| [Building_From_Source.md](Building_From_Source.md) | Clone, submodules, **required tools**, Pico SDK, and build/flash on Linux / macOS / Windows. |
| [Firmware_Architecture.md](Firmware_Architecture.md) | How the firmware is structured, runtime flow, modules, and **every file to touch** when adding host/device drivers. |
| [Adding_Supported_Controllers.md](Adding_Supported_Controllers.md) | Add a new input pad: VID/PID, **full HID reports**, host drivers, PadIn mapping, Debug UART. |
| [Tools/controller_capture](../../../Tools/controller_capture/README.md) | PC helpers: VID/PID checklist + **`hidraw_full_report_dump.py`** for full report streams. |
| [README — Support policy](../../README.md#support-policy) | Maintainer-supported boards and first-party controller policy. |

---

## Feature guides

How to use specific output modes and related setup.

| Document | Description |
|----------|-------------|
| [SteamOS_Bazzite_Output_Mode.md](SteamOS_Bazzite_Output_Mode.md) | **STEAM** mode: DualSense USB + touchpad → HID mouse (SteamOS / Bazzite / Linux desktop). |
| [PS3_PS4_Motion_Controls.md](PS3_PS4_Motion_Controls.md) | PS3 / PS4 Sixaxis / tilt passthrough, supported inputs, Brook adapter notes. |
| [Wii_Mode_Guide.md](Wii_Mode_Guide.md) | Wii (Wiimote) output — build-option only: extensions, sync, mapping. |
| [PICO2W_WII_USB_SETUP.md](PICO2W_WII_USB_SETUP.md) | Pico W / Pico 2 W USB host wiring (PIO USB) for Wii mode. |
| [GPIO_Output_Pinout_and_Mappings.md](GPIO_Output_Pinout_and_Mappings.md) | GPIO pin-outs and mappings: PS1/PS2, Dreamcast, GameCube, N64. |

---

## Reference (lists & mappings)

| Document | Description |
|----------|-------------|
| [Wired_Controllers.md](Wired_Controllers.md) | Supported **wired USB** input pads by host driver (VID/PID lists). |
| [Controller_Mappings.md](Controller_Mappings.md) | **PadIn** master reference: input → PadIn → every output mode. |

---

## Technical notes & changelog

| Document | Description |
|----------|-------------|
| [Firmware_Architecture.md](Firmware_Architecture.md) | Folder structure, Core0/Core1 flow, PadIn bridge, host/device checklists, external modules. |
| [Planned_Additions.md](Planned_Additions.md) | Roadmap / future work (original creator + this fork + researched projects such as Xbox wireless dongle). |
| [IMPROVEMENTS.md](IMPROVEMENTS.md) | Deep technical notes: BT stability, Switch 2 / Triton, latency, rumble, unplug detection. |
| [Dreamcast_Port.md](Dreamcast_Port.md) | Dreamcast (Maple) port status / completing the port from DreamPicoPort. |
| [CHANGELOG.md](../../../CHANGELOG.md) | Version history and release notes (repo root). |

---

## Research & planning

Longer-form surveys (not day-to-day user manuals):

| Document | Description |
|----------|-------------|
| [docs/README.md](../../../docs/README.md) | Index of research docs |
| [Input_Controllers_Research.md](../../../docs/Input_Controllers_Research.md) | Current vs possible USB/BT input controllers |
| [Wired_Retro_Controllers.md](../../../docs/Wired_Retro_Controllers.md) | Wired retro GPIO/USB input possibilities |
| [Other_Projects_Output_Modes.md](../../../docs/Other_Projects_Output_Modes.md) | Output modes in other projects not yet in OGX-Mini |
