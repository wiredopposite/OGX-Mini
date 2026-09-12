#!/usr/bin/env python3
"""
OGX-Mini controller input capture tool.

Walks the user through selecting a target platform, choosing a connected
gamepad (via SDL/pygame), then mapping each control. Works with **USB** and
**Bluetooth** controllers as long as the OS exposes them to SDL. Produces a
text/JSON report for maintainers to add HardwareIDs / HID layout support.

Usage:
  python3 controller_capture.py
  python3 controller_capture.py -o my_pad.txt
  python3 controller_capture.py --skip-raw-hid   # SDL/VID only; no hidraw hex phase
  python3 controller_capture.py --use-sdl-name --auto-output-name   # SDL name + auto .txt filename
  python3 controller_capture.py --raw-hid-only --auto-output-name  # Linux: hidraw hex only, no pygame
  python3 controller_capture.py --no-hidraw-sidecar                 # SDL only; no hidraw alongside pygame
"""

from __future__ import annotations

import argparse
import json
import os
import re
import platform
import sys
import textwrap
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any

from switch2_usb_init import platform_requires_switch2_usb_init, run_switch2_usb_handshake

# ---------------------------------------------------------------------------
# Platform → ordered list of logical controls to capture
# Labels are what we show the user; ids are stable keys in the output.
# ---------------------------------------------------------------------------

_SWITCH_PRO_LAYOUT: list[tuple[str, str]] = [
    ("face_b", "B (bottom)"),
    ("face_a", "A (right)"),
    ("face_y", "Y (left)"),
    ("face_x", "X (top)"),
    ("shoulder_l", "L"),
    ("shoulder_r", "R"),
    ("trigger_zl", "ZL — press fully"),
    ("trigger_zr", "ZR — press fully"),
    ("center_minus", "Minus (−)"),
    ("center_plus", "Plus (+)"),
    ("center_home", "Home"),
    ("center_capture", "Capture (or skip)"),
    ("stick_l3", "Left stick click"),
    ("stick_r3", "Right stick click"),
    ("dpad_up", "D-pad UP"),
    ("dpad_down", "D-pad DOWN"),
    ("dpad_left", "D-pad LEFT"),
    ("dpad_right", "D-pad RIGHT"),
    ("axis_lstick_left", "Left stick — fully LEFT"),
    ("axis_lstick_right", "Left stick — fully RIGHT"),
    ("axis_lstick_up", "Left stick — fully UP"),
    ("axis_lstick_down", "Left stick — fully DOWN"),
    ("axis_rstick_left", "Right stick — fully LEFT"),
    ("axis_rstick_right", "Right stick — fully RIGHT"),
    ("axis_rstick_up", "Right stick — fully UP"),
    ("axis_rstick_down", "Right stick — fully DOWN"),
]

PLATFORM_CONTROLS: dict[str, list[tuple[str, str]]] = {
    "Xbox / XInput (360, One, Series, Elite)": [
        ("face_a", "A (bottom face)"),
        ("face_b", "B (right face)"),
        ("face_x", "X (left face)"),
        ("face_y", "Y (top face)"),
        ("shoulder_lb", "Left bumper (LB)"),
        ("shoulder_rb", "Right bumper (RB)"),
        ("trigger_lt", "Left trigger (LT) — press fully"),
        ("trigger_rt", "Right trigger (RT) — press fully"),
        ("center_view", "View / Back / Select"),
        ("center_menu", "Menu / Start"),
        ("center_guide", "Guide / Xbox / Nexus button"),
        ("stick_l3", "Left stick click (L3)"),
        ("stick_r3", "Right stick click (R3)"),
        ("dpad_up", "D-pad UP"),
        ("dpad_down", "D-pad DOWN"),
        ("dpad_left", "D-pad LEFT"),
        ("dpad_right", "D-pad RIGHT"),
        ("axis_lstick_left", "Left stick — push fully LEFT"),
        ("axis_lstick_right", "Left stick — push fully RIGHT"),
        ("axis_lstick_up", "Left stick — push fully UP"),
        ("axis_lstick_down", "Left stick — push fully DOWN"),
        ("axis_rstick_left", "Right stick — push fully LEFT"),
        ("axis_rstick_right", "Right stick — push fully RIGHT"),
        ("axis_rstick_up", "Right stick — push fully UP"),
        ("axis_rstick_down", "Right stick — push fully DOWN"),
    ],
    "PlayStation (DS3 / DS4 / DualSense / generic)": [
        ("face_cross", "Cross (×)"),
        ("face_circle", "Circle (○)"),
        ("face_square", "Square (□)"),
        ("face_triangle", "Triangle (△)"),
        ("shoulder_l1", "L1"),
        ("shoulder_r1", "R1"),
        ("trigger_l2", "L2 — press fully"),
        ("trigger_r2", "R2 — press fully"),
        ("center_share", "Share / Select"),
        ("center_options", "Options / Start"),
        ("center_ps", "PlayStation / PS button"),
        ("touchpad_click", "Touchpad click (or skip if none)"),
        ("stick_l3", "L3 (left stick click)"),
        ("stick_r3", "R3 (right stick click)"),
        ("dpad_up", "D-pad UP"),
        ("dpad_down", "D-pad DOWN"),
        ("dpad_left", "D-pad LEFT"),
        ("dpad_right", "D-pad RIGHT"),
        ("axis_lstick_left", "Left stick — fully LEFT"),
        ("axis_lstick_right", "Left stick — fully RIGHT"),
        ("axis_lstick_up", "Left stick — fully UP"),
        ("axis_lstick_down", "Left stick — fully DOWN"),
        ("axis_rstick_left", "Right stick — fully LEFT"),
        ("axis_rstick_right", "Right stick — fully RIGHT"),
        ("axis_rstick_up", "Right stick — fully UP"),
        ("axis_rstick_down", "Right stick — fully DOWN"),
    ],
    "Nintendo Switch Pro / wired Switch layout": list(_SWITCH_PRO_LAYOUT),
    # Switch 2 / Joy-Con 2: PC does not expose SDL input until USB bulk handshake (see switch2_usb_init.py).
    "Nintendo Switch 2 Pro / Joy-Con 2 (USB — libusb handshake, then capture)": list(_SWITCH_PRO_LAYOUT),
    "Generic DInput / PC gamepad": [
        ("face_1", "Primary action (often '1' or South)"),
        ("face_2", "Secondary (often '2' or East)"),
        ("face_3", "Tertiary"),
        ("face_4", "Quaternary"),
        ("shoulder_l", "Left shoulder"),
        ("shoulder_r", "Right shoulder"),
        ("trigger_l", "Left trigger — fully"),
        ("trigger_r", "Right trigger — fully"),
        ("select", "Select / Back"),
        ("start", "Start"),
        ("mode", "Mode / Home / Guide (if present)"),
        ("stick_l3", "Left stick click"),
        ("stick_r3", "Right stick click"),
        ("dpad_up", "D-pad UP"),
        ("dpad_down", "D-pad DOWN"),
        ("dpad_left", "D-pad LEFT"),
        ("dpad_right", "D-pad RIGHT"),
        ("axis_lstick_left", "Left stick — fully LEFT"),
        ("axis_lstick_right", "Left stick — fully RIGHT"),
        ("axis_lstick_up", "Left stick — fully UP"),
        ("axis_lstick_down", "Left stick — fully DOWN"),
        ("axis_rstick_left", "Right stick — fully LEFT"),
        ("axis_rstick_right", "Right stick — fully RIGHT"),
        ("axis_rstick_up", "Right stick — fully UP"),
        ("axis_rstick_down", "Right stick — fully DOWN"),
    ],
    "Wii / Wii U style (Classic / adapter — what PC sees)": [
        ("face_a", "A"),
        ("face_b", "B"),
        ("face_x", "X"),
        ("face_y", "Y"),
        ("shoulder_l", "L"),
        ("shoulder_r", "R"),
        ("trigger_zl", "ZL"),
        ("trigger_zr", "ZR"),
        ("minus", "Minus"),
        ("plus", "Plus"),
        ("home", "Home"),
        ("stick_l3", "Left stick click (if any)"),
        ("stick_r3", "Right stick click (if any)"),
        ("dpad_up", "D-pad UP"),
        ("dpad_down", "D-pad DOWN"),
        ("dpad_left", "D-pad LEFT"),
        ("dpad_right", "D-pad RIGHT"),
        ("axis_lstick_left", "Left stick — LEFT"),
        ("axis_lstick_right", "Left stick — RIGHT"),
        ("axis_lstick_up", "Left stick — UP"),
        ("axis_lstick_down", "Left stick — DOWN"),
        ("axis_rstick_left", "Right stick — LEFT"),
        ("axis_rstick_right", "Right stick — RIGHT"),
        ("axis_rstick_up", "Right stick — UP"),
        ("axis_rstick_down", "Right stick — DOWN"),
    ],
    "Original Xbox (Duke / S) — if exposed as HID on PC": [
        ("face_a", "A"),
        ("face_b", "B"),
        ("face_x", "X"),
        ("face_y", "Y"),
        ("black", "Black"),
        ("white", "White"),
        ("trigger_l", "Left trigger"),
        ("trigger_r", "Right trigger"),
        ("back", "Back"),
        ("start", "Start"),
        ("stick_l3", "Left stick click"),
        ("stick_r3", "Right stick click"),
        ("dpad_up", "D-pad UP"),
        ("dpad_down", "D-pad DOWN"),
        ("dpad_left", "D-pad LEFT"),
        ("dpad_right", "D-pad RIGHT"),
        ("axis_lstick_left", "Left stick — LEFT"),
        ("axis_lstick_right", "Left stick — RIGHT"),
        ("axis_lstick_up", "Left stick — UP"),
        ("axis_lstick_down", "Left stick — DOWN"),
        ("axis_rstick_left", "Right stick — LEFT"),
        ("axis_rstick_right", "Right stick — RIGHT"),
        ("axis_rstick_up", "Right stick — UP"),
        ("axis_rstick_down", "Right stick — DOWN"),
    ],
    "Other / not listed (custom list)": [
        ("ctrl_01", "Control 1 — press or move as instructed next"),
        ("ctrl_02", "Control 2"),
        ("ctrl_03", "Control 3"),
        ("ctrl_04", "Control 4"),
        ("ctrl_05", "Control 5"),
        ("ctrl_06", "Control 6"),
        ("ctrl_07", "Control 7"),
        ("ctrl_08", "Control 8"),
        ("ctrl_09", "Control 9"),
        ("ctrl_10", "Control 10"),
        ("ctrl_11", "Control 11"),
        ("ctrl_12", "Control 12"),
    ],
}

TROUBLESHOOTING = """
If this program does not see your controller or inputs do not register:

  • USB: unplug and replug; try a different port (USB 2.0 often best) and a
    data-capable cable (many cables are charge-only).
  • Bluetooth: pair and connect the controller in **Windows Settings / macOS
    System Settings / Linux BlueZ** first, then run this script. Put the pad in
    pairing mode if it won’t connect. Stay within range; low battery can drop input.
  • If Bluetooth is flaky, try **USB** for this capture (same mapping is still useful).
  • Close Steam (Steam Input can remap or hide raw gamepad input). In Steam:
    Settings → Controller → disable Steam Input for the gamepad, or exit Steam.
  • On Windows: check Device Manager — controller should appear under
    "Human Interface Devices", "Bluetooth", or "Xbox peripherals" without a yellow
    warning. Update or reinstall the vendor driver if needed.
  • On Linux: your user may need access to /dev/input/js* and event nodes.
    Install `steam-devices` or add a udev rule; try: `sudo chmod a+r /dev/input/*`
    temporarily to test (not a permanent fix).
  • Multi-mode pads (8BitDo, etc.): switch to DInput, XInput, or Switch mode
    per the manual — use the mode that matches how OGX-Mini will see the pad.
  • Run this script from a normal terminal with the gamepad connected before start.
  • If nothing helps: note your OS version, controller model, USB vs Bluetooth,
    and how it appears in Device Manager / System Information / `lsusb`, and send
    that with your mapping file.
"""


def print_troubleshooting() -> None:
    print(TROUBLESHOOTING)


# ---------------------------------------------------------------------------
# USB VID:PID (Linux sysfs)
# ---------------------------------------------------------------------------


def try_linux_connection_guess(js_index: int) -> str:
    """Best-effort: whether the js device path looks USB- or Bluetooth-attached."""
    try:
        path = os.path.realpath(f"/sys/class/input/js{js_index}/device")
    except OSError:
        return "unknown"
    segments = [s.lower() for s in path.split(os.sep)]
    if "bluetooth" in segments:
        return "bluetooth"
    for seg in segments:
        if len(seg) >= 4 and seg.startswith("usb") and seg[3:].isdigit():
            return "usb"
    if "/usb" in path.replace("\\", "/").lower():
        return "usb"
    return "unknown"


def try_linux_vid_pid_for_js(js_index: int) -> tuple[str | None, str | None, str | None]:
    """Walk sysfs from /sys/class/input/jsN to find idVendor/idProduct."""
    base = f"/sys/class/input/js{js_index}/device"
    if not os.path.isdir(base):
        return None, None, None
    path = os.path.realpath(base)
    for _ in range(12):
        vid_p = os.path.join(path, "idVendor")
        pid_p = os.path.join(path, "idProduct")
        if os.path.isfile(vid_p) and os.path.isfile(pid_p):
            try:
                with open(vid_p, encoding="utf-8") as f:
                    vid = f.read().strip().lower()
                with open(pid_p, encoding="utf-8") as f:
                    pid = f.read().strip().lower()
                return vid, pid, path
            except OSError:
                break
        parent = os.path.dirname(path)
        if parent == path:
            break
        path = parent
    return None, None, None


def print_vid_pid_instructions() -> None:
    """Step-by-step VID/PID lookup for the current OS (for HardwareIDs.h)."""
    sysname = platform.system()
    print(
        textwrap.dedent(
            """
            Why we need VID and PID
            -----------------------
            Maintainers add a line like { VID, PID } in HardwareIDs.h so the
            firmware recognizes your controller. The values are **four hex digits
            each** (Vendor ID and Product ID), e.g. VID 045e and PID 0b12.

            """
        ).rstrip()
    )

    if sysname == "Windows":
        print(
            textwrap.dedent(
                """
                Windows — find VID & PID
                ------------------------
                1. Right-click **Start** → **Device Manager**.
                2. Find your controller. Try expanding:
                     • **Human Interface Devices** (many gamepads)
                     • **Xbox peripherals** (some Microsoft / Xbox pads)
                     • **Bluetooth** (if you use the controller wirelessly)
                3. Right-click the device → **Properties**.
                4. Open the **Details** tab.
                5. In **Property**, choose **Hardware Ids** (or **Device instance path**).
                6. In the list, look for a line containing **VID_xxxx** and **PID_yyyy**
                   (example: `USB\\VID_045E&PID_0B12` or `HID\\VID_045E&PID_0B12`).
                7. Copy **xxxx** as the VID and **yyyy** as the PID (hex, no 0x prefix
                   needed when you type them below).

                Tip: If you see multiple lines, prefer the one that matches **USB** or
                **HID** for the gamepad you are holding.

                """
            ).rstrip()
        )
    elif sysname == "Darwin":
        print(
            textwrap.dedent(
                """
                macOS — find VID & PID
                ----------------------
                **Option A — System Information**
                1. Hold **Option** → click the Apple menu → **System Information**.
                2. Under **Hardware**, click **USB** (wired) or **Bluetooth** (wireless).
                3. Select your controller in the list.
                4. Look for **Vendor ID** and **Product ID** (often shown in hex).

                **Option B — Terminal (works for many devices)**
                • USB: open **Terminal** and run:
                    system_profiler SPUSBDataType
                  Scroll to your controller and note **Vendor ID** and **Product ID**.
                • Bluetooth: run:
                    system_profiler SPBluetoothDataType
                  Find your device; note vendor/product if listed.

                Use the hex values (you may see 0x054C — type **054c** for VID below).

                """
            ).rstrip()
        )
    else:
        # Linux and other Unix-like
        print(
            textwrap.dedent(
                """
                Linux — find VID & PID
                ----------------------
                **USB controller**
                1. Plug the controller in, then in a terminal run:
                     lsusb
                2. Find the line for your device (name on the right). The form is:
                     Bus 001 Device 012: ID **1234:5678** Vendor Name
                   Here **1234** is the VID and **5678** is the PID (hex).

                **Bluetooth controller**
                1. Pair and connect the controller first.
                2. Try:
                     lsusb
                   Some adapters show the pad here. If not, try:
                     bluetoothctl devices
                   (VID/PID may still appear in **udev** for the input device.)
                3. With the controller connected, replace **0** with your joystick index
                   if needed (often **js0**):
                     udevadm info -q property -n /dev/input/js0 | grep -E 'ID_VENDOR_ID|ID_MODEL_ID'
                   You should see **ID_VENDOR_ID**= and **ID_MODEL_ID**= in hex (0x prefix OK).

                If this script auto-detected VID/PID on Linux, you can still verify with
                the commands above.

                """
            ).rstrip()
        )


# ---------------------------------------------------------------------------
# Pygame capture
# ---------------------------------------------------------------------------


@dataclass
class CaptureState:
    """User-provided retail name plus SDL-reported identity for maintainer tracking."""

    platform: str
    controller_full_name: str
    pygame_name: str
    pygame_guid: str
    num_axes: int
    num_buttons: int
    num_hats: int
    os_name: str
    vid: str | None = None
    pid: str | None = None
    vid_pid_source: str = "unknown"
    connection_guess: str | None = None  # "usb" | "bluetooth" | "unknown" (Linux); None if N/A
    bindings: dict[str, Any] = field(default_factory=dict)
    baseline_axes: list[float] = field(default_factory=list)
    notes: str = ""
    skipped: list[str] = field(default_factory=list)
    # Result of Switch 2–family USB bulk init (PyUSB), if that platform was selected.
    switch2_usb_handshake: dict[str, Any] | None = None
    # Linux hidraw: per-control raw input report hex (baseline vs active, xor) for firmware decode.
    raw_hid_reports: dict[str, Any] | None = None
    # Linux: raw hidraw bytes sampled right after pygame saw each control (no idle guessing).
    hidraw_gated_by_sdl: dict[str, Any] | None = None
    # Whether controller_full_name was typed by the user, SDL, or Linux hidraw HID_NAME.
    controller_full_name_source: str = "user_entered"  # "user_entered" | "sdl" | "hidraw"
    # "full" = SDL capture (optional raw phase); "raw_hid_only" = --raw-hid-only (Linux).
    capture_mode: str = "full"


def sanitize_filename_stem(name: str, max_len: int = 80) -> str:
    """Make a single path component from a controller name (no dirs)."""
    s = name.strip()
    for c in '<>:"/\\|?\x00-\x1f':
        s = s.replace(c, "_")
    s = re.sub(r"\s+", "_", s)
    s = re.sub(r"_+", "_", s).strip("._ ")
    return (s or "controller")[:max_len]


def default_report_filename(st: CaptureState) -> str:
    """Suggested report path under cwd: <sanitized_name>_<timestamp>.txt"""
    stem = sanitize_filename_stem(st.controller_full_name)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"{stem}_{ts}.txt"


def init_pygame() -> None:
    os.environ.setdefault("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1")
    import pygame

    pygame.init()
    pygame.joystick.init()


def poll_joystick_state(joy) -> tuple[list[float], list[int], list[tuple[int, int]]]:
    import pygame

    axes = [float(joy.get_axis(i)) for i in range(joy.get_numaxes())]
    buttons = [joy.get_button(i) for i in range(joy.get_numbuttons())]
    hats = []
    for i in range(joy.get_numhats()):
        hats.append(joy.get_hat(i))
    return axes, buttons, hats


def wait_for_quiet(joy, settle_s: float = 0.35) -> tuple[list[float], list[int], list[tuple[int, int]]]:
    """Read stable resting state."""
    import pygame

    last: tuple | None = None
    stable_since: float | None = None
    while True:
        pygame.event.pump()
        state = poll_joystick_state(joy)
        if last is not None and state == last:
            if stable_since is None:
                stable_since = time.monotonic()
            elif time.monotonic() - stable_since >= settle_s:
                return state
        else:
            stable_since = None
        last = state
        time.sleep(0.02)


def rebaseline_for_capture(joy) -> tuple[list[float], list[int], list[tuple[int, int]]]:
    """
    Clear pygame's queue and sample a new neutral pose before listening for the
    *next* control. Without this, a held button or deflected stick from the
    previous step immediately matches the next prompt (false 'auto' capture).
    """
    import pygame

    pygame.event.clear()
    return wait_for_quiet(joy)


def _joy_event_matches(event, joy) -> bool:
    """Pygame 2 uses instance_id on events; older code used joy index."""
    if hasattr(event, "instance_id"):
        try:
            return int(event.instance_id) == int(joy.get_instance_id())
        except (AttributeError, TypeError):
            pass
    return getattr(event, "joy", None) == joy.get_id()


def capture_next_input(
    joy,
    baseline_axes: list[float],
    baseline_buttons: list[int],
    baseline_hats: list[tuple[int, int]],
    timeout_s: float = 45.0,
    axis_threshold: float = 0.45,
) -> tuple[str, dict[str, Any]] | tuple[None, None]:
    """
    Block until a new button, hat, or significant axis change vs baseline.
    Returns (kind, detail_dict).
    """
    import pygame

    from pygame import JOYBUTTONDOWN, JOYHATMOTION, JOYAXISMOTION, QUIT

    pygame.event.clear()

    start = time.monotonic()
    while time.monotonic() - start < timeout_s:
        for event in pygame.event.get():
            if event.type == QUIT:
                return None, None
            if event.type == JOYBUTTONDOWN and _joy_event_matches(event, joy):
                bi = event.button
                if bi < len(baseline_buttons) and baseline_buttons[bi]:
                    continue
                return "button", {"button": bi}
            if event.type == JOYHATMOTION and _joy_event_matches(event, joy):
                if event.value != (0, 0):
                    return "hat", {"hat": event.hat, "value": list(event.value)}
            if event.type == JOYAXISMOTION and _joy_event_matches(event, joy):
                ax = event.axis
                val = event.value
                if ax < len(baseline_axes) and abs(val - baseline_axes[ax]) >= axis_threshold:
                    return "axis", {"axis": ax, "value": val, "delta_vs_baseline": val - baseline_axes[ax]}

        pygame.event.pump()
        axes, buttons, hats = poll_joystick_state(joy)
        # Polling fallback for buttons not seen as events on some drivers
        for bi, b in enumerate(buttons):
            if bi < len(baseline_buttons) and b and not baseline_buttons[bi]:
                return "button", {"button": bi}
        for hi, h in enumerate(hats):
            if hi < len(baseline_hats) and h != baseline_hats[hi] and h != (0, 0):
                return "hat", {"hat": hi, "value": list(h)}
        for ai, val in enumerate(axes):
            if ai < len(baseline_axes) and abs(val - baseline_axes[ai]) >= axis_threshold:
                return "axis", {"axis": ai, "value": val, "delta_vs_baseline": val - baseline_axes[ai]}
        time.sleep(0.01)

    return None, None


def choose_platform() -> str:
    names = list(PLATFORM_CONTROLS.keys())
    print("\nSelect the platform / layout this physical controller is meant for:\n")
    for i, n in enumerate(names, 1):
        print(f"  {i}. {n}")
    while True:
        raw = input("\nEnter number (or 'q' to quit): ").strip().lower()
        if raw == "q":
            sys.exit(0)
        try:
            idx = int(raw)
            if 1 <= idx <= len(names):
                return names[idx - 1]
        except ValueError:
            pass
        print("Invalid choice.")


def choose_joystick():
    import pygame

    n = pygame.joystick.get_count()
    if n == 0:
        print("\nNo joysticks detected. Connect a controller and try again.\n")
        print_troubleshooting()
        sys.exit(1)
    print(
        "\nConnected controllers (USB **or** Bluetooth — pair BT devices in the OS first):\n"
    )
    for i in range(n):
        j = pygame.joystick.Joystick(i)
        j.init()
        print(f"  {i}. {j.get_name()}")
        j.quit()
    while True:
        raw = input("\nEnter index of the controller to capture (or 'q'): ").strip().lower()
        if raw == "q":
            sys.exit(0)
        try:
            idx = int(raw)
            if 0 <= idx < n:
                joy = pygame.joystick.Joystick(idx)
                joy.init()
                return joy, idx
        except ValueError:
            pass
        print("Invalid index.")


def prompt_controller_full_name(
    sdl_reported_name: str,
    *,
    use_sdl_only: bool = False,
) -> tuple[str, str]:
    """
    Ask for the full retail / marketing name, or use SDL's name.

    Returns (controller_full_name, source) where source is 'user_entered' or 'sdl'.
    """
    header = "\n" + "=" * 72 + "\nController identity\n" + "=" * 72 + "\n\n"

    if use_sdl_only:
        print(
            header
            + "Using **SDL-reported name** as the controller full name (`--use-sdl-name`):\n  "
            f"{sdl_reported_name!r}\n"
        )
        return sdl_reported_name, "sdl"

    print(
        header
        + "Enter the **full product name** of this controller "
        "(as printed on the box, manual, or the manufacturer’s website).\n\n"
        f"Your PC / SDL currently reports this device as:\n  {sdl_reported_name!r}\n\n"
        "**Press Enter** to use that SDL name as-is (optional shortcut).\n\n"
        "Examples if you prefer to type a retail name: “Sony DualSense Wireless Controller”, "
        "“PowerA Enhanced Wired Controller for Xbox Series X|S”, "
        "“8BitDo Pro 2”.\n"
    )
    name = input("Full controller name (Enter = use SDL name): ").strip()
    if name:
        return name, "user_entered"
    return sdl_reported_name, "sdl"


def prompt_vid_pid_with_instructions(st: CaptureState) -> None:
    """
    Show OS-specific VID/PID instructions, then optional manual entry or override.
    Updates st.vid, st.pid, st.vid_pid_source in place.
    """
    had_auto = bool(st.vid and st.pid)

    print("\n" + "=" * 72)
    print("USB Vendor ID (VID) and Product ID (PID)")
    print("=" * 72 + "\n")

    print_vid_pid_instructions()

    if had_auto:
        print(
            f"\n>>> This session already detected:  VID: {st.vid}    PID: {st.pid}\n"
            f"    Source: {st.vid_pid_source}\n"
            "\n    Leave the next two lines **empty** to keep these values in the report,\n"
            "    or type new hex values if you verified different ones using the steps above.\n"
        )
    else:
        print(
            "\nEnter the **VID** and **PID** you found (hex digits only, no 0x prefix required).\n"
            "Leave both **empty** to skip — the report will still help, but adding VID/PID\n"
            "makes it much easier to add your controller to the firmware list.\n"
        )

    v = input("VID (hex, or Enter to " + ("keep auto-detected" if had_auto else "skip") + "): ").strip().lower().replace("0x", "")
    p = input("PID (hex, or Enter to " + ("keep auto-detected" if had_auto else "skip") + "): ").strip().lower().replace("0x", "")

    if v and p:
        st.vid, st.pid = v, p
        st.vid_pid_source = "user entered"
    elif v or p:
        print("\nNeed **both** VID and PID, or leave **both** blank. Keeping previous values.\n")
    elif not had_auto:
        # leave st.vid/st.pid as None
        pass
    # else: had_auto and blank input — keep sysfs values already in st


def run_capture_loop(
    joy,
    platform_name: str,
    js_index: int,
    *,
    switch2_usb_handshake: dict[str, Any] | None = None,
    use_sdl_name: bool = False,
    enable_hidraw_sidecar: bool = True,
) -> CaptureState:
    import pygame

    name = joy.get_name()
    controller_full_name, name_src = prompt_controller_full_name(name, use_sdl_only=use_sdl_name)
    guid = joy.get_guid()
    na = joy.get_numaxes()
    nb = joy.get_numbuttons()
    nh = joy.get_numhats()

    st = CaptureState(
        platform=platform_name,
        controller_full_name=controller_full_name,
        controller_full_name_source=name_src,
        pygame_name=name,
        pygame_guid=guid,
        num_axes=na,
        num_buttons=nb,
        num_hats=nh,
        os_name=f"{platform.system()} {platform.release()}",
        switch2_usb_handshake=switch2_usb_handshake,
    )

    if platform.system() == "Linux":
        st.connection_guess = try_linux_connection_guess(js_index)
        vid, pid, sysfs = try_linux_vid_pid_for_js(js_index)
        if vid and pid:
            st.vid, st.pid = vid, pid
            st.vid_pid_source = f"linux sysfs ({sysfs})"
        else:
            st.vid_pid_source = "not found in sysfs — use instructions at end"
    elif platform.system() == "Windows":
        st.vid_pid_source = "not auto-detected — use instructions at end"
    else:
        st.vid_pid_source = "not auto-detected — use instructions at end"

    controls = PLATFORM_CONTROLS[platform_name]

    hidraw_fd: int | None = None
    hidraw_path: str | None = None
    if (
        enable_hidraw_sidecar
        and platform.system() == "Linux"
        and st.vid
        and st.pid
    ):
        try:
            vid_i, pid_i = int(st.vid, 16), int(st.pid, 16)
        except ValueError:
            vid_i = pid_i = -1
        if vid_i >= 0:
            from raw_hid_capture import try_open_hidraw_for_sidecar

            side_ans = input(
                "\nRecord **raw HID bytes** from hidraw when **pygame** sees each press "
                "(recommended on Linux — avoids false captures from IMU/timer noise)? [Y/n]: "
            ).strip().lower()
            if side_ans in ("", "y", "yes"):
                hidraw_fd, hidraw_path = try_open_hidraw_for_sidecar(vid_i, pid_i)

    print(
        "\n"
        + "=" * 72
        + "\nFor each prompt, press or move ONLY that control.\n"
        "After each capture, return sticks to center and release all buttons — the\n"
        "script waits for neutral before the next step (avoids false inputs).\n"
        "Type 's' + Enter to skip a control, 'r' + Enter to re-capture the previous,\n"
        "'t' + Enter to print troubleshooting tips, 'q' + Enter to quit.\n"
        + "=" * 72
        + "\n"
    )

    print("Release all buttons and sticks to neutral. Calibrating rest position...")
    pygame.event.clear()
    axes, buttons, hats = wait_for_quiet(joy)
    st.baseline_axes = axes
    baseline_buttons = buttons
    baseline_hats = hats

    print(f"Baseline axes (approx): {[round(x, 3) for x in axes]}")
    print(f"Buttons: {nb}, Axes: {na}, Hats: {nh}\n")

    prev_binding: tuple[str, Any] | None = None
    i = 0
    try:
        while i < len(controls):
            cid, label = controls[i]
            print(f"\n>>> [{i + 1}/{len(controls)}]  {label}")
            print("    Release all buttons and center the sticks — sampling neutral...")
            axes, baseline_buttons, baseline_hats = rebaseline_for_capture(joy)
            st.baseline_axes = axes

            baseline_hid: bytes | None = None
            if hidraw_fd is not None:
                from raw_hid_capture import peek_latest_hidraw

                baseline_hid = peek_latest_hidraw(hidraw_fd, 0.08)

            print("    Press or move ONLY the control named above.")
            print("    Waiting for input... ", end="", flush=True)
            kind, detail = capture_next_input(joy, st.baseline_axes, baseline_buttons, baseline_hats)
            if kind is None:
                print("\n\nNo input detected for a long time.\n")
                print_troubleshooting()
                ans = input("\nTry again this step? [Y/n/s=skip]: ").strip().lower()
                if ans == "s":
                    st.skipped.append(cid)
                    i += 1
                    continue
                if ans == "n":
                    break
                print("Release all controls to neutral (re-calibrating)...")
                pygame.event.clear()
                axes, buttons, hats = wait_for_quiet(joy)
                st.baseline_axes = axes
                baseline_buttons = buttons
                baseline_hats = hats
                continue

            print(f"captured: {kind} {detail}")
            binding: dict[str, Any] = {"type": kind, **detail}
            if hidraw_fd is not None:
                from raw_hid_capture import peek_latest_hidraw, xor_hex_reports

                active_hid = peek_latest_hidraw(hidraw_fd, 0.12)
                if baseline_hid and active_hid and baseline_hid == active_hid:
                    time.sleep(0.03)
                    active_hid = peek_latest_hidraw(hidraw_fd, 0.18)
                if baseline_hid is not None and active_hid is not None:
                    binding["hidraw_baseline_hex"] = baseline_hid.hex()
                    binding["hidraw_active_hex"] = active_hid.hex()
                    xor_s = xor_hex_reports(baseline_hid, active_hid)
                    binding["hidraw_xor_hex"] = xor_s
                    binding["hidraw_baseline_len"] = len(baseline_hid)
                    binding["hidraw_active_len"] = len(active_hid)
                    print(
                        "\n    [hidraw] Full baseline packet "
                        f"(len={len(baseline_hid)}):\n      "
                        + _wrap_hex_lines(baseline_hid.hex()).replace("\n", "\n      ")
                        + "\n    [hidraw] Full active packet "
                        f"(len={len(active_hid)}):\n      "
                        + _wrap_hex_lines(active_hid.hex()).replace("\n", "\n      ")
                        + "\n    [hidraw] XOR (baseline ⊕ active):\n      "
                        + _wrap_hex_lines(xor_s).replace("\n", "\n      ")
                    )
                else:
                    binding["hidraw_note"] = "partial (could not read baseline or active packet)"

            st.bindings[cid] = binding
            prev_binding = (cid, st.bindings[cid])
            i += 1

            # Optional command from user after each capture (stdin)
            hint = input("Enter=s continue | s=skip | r=redo last | t=tips | q=quit : ").strip().lower()
            if hint == "q":
                break
            if hint == "t":
                print_troubleshooting()
            elif hint == "s" and i < len(controls):
                cid2, _ = controls[i]
                st.skipped.append(cid2)
                i += 1
            elif hint == "r" and prev_binding is not None:
                i -= 1
                del st.bindings[prev_binding[0]]
    finally:
        if hidraw_fd is not None:
            from raw_hid_capture import close_hidraw_fd

            close_hidraw_fd(hidraw_fd)

    if hidraw_path and any("hidraw_active_hex" in v for v in st.bindings.values() if isinstance(v, dict)):
        st.hidraw_gated_by_sdl = {
            "hidraw_path": hidraw_path,
            "method": "Pygame detects input; latest hidraw report read immediately after (baseline after neutral).",
        }
    elif hidraw_path:
        st.hidraw_gated_by_sdl = {
            "hidraw_path": hidraw_path,
            "method": "hidraw opened but no complete per-control samples (check hidraw_note on bindings).",
        }

    extra = input("\nAny extra notes for the developer (optional): ").strip()
    if extra:
        st.notes = extra

    prompt_vid_pid_with_instructions(st)

    return st


def resolve_vid_pid_for_raw_capture(st: CaptureState) -> tuple[int, int] | None:
    """VID/PID as integers for hidraw matching (Linux sysfs or Switch 2 handshake)."""
    if st.vid and st.pid:
        try:
            return int(st.vid, 16), int(st.pid, 16)
        except ValueError:
            pass
    h = st.switch2_usb_handshake
    if h and h.get("product_id"):
        try:
            return 0x057E, int(str(h["product_id"]), 16)
        except ValueError:
            pass
    return None


def run_raw_hid_only_capture(
    platform_name: str,
    switch2_handshake: dict[str, Any] | None,
) -> CaptureState:
    """
    Linux only: skip pygame; open hidraw by VID/PID and run per-control raw hex capture.
    """
    if platform.system() != "Linux":
        print("Error: --raw-hid-only requires Linux (/dev/hidraw*).")
        sys.exit(1)

    vid: str | None = None
    pid: str | None = None
    vid_pid_src = "unknown"

    if switch2_handshake and switch2_handshake.get("ok") and switch2_handshake.get("product_id"):
        vid = "057e"
        pid = str(switch2_handshake["product_id"]).lower().replace("0x", "")
        vid_pid_src = "switch2_usb_handshake"

    if not vid or not pid:
        print("\nUSB **VID** and **PID** (hex) are required to find the hidraw node.\n")
        v = input("VID [Enter = 057e Nintendo]: ").strip().lower().replace("0x", "")
        p = input("PID: ").strip().lower().replace("0x", "")
        if not p:
            print("PID is required for raw-hid-only.")
            sys.exit(1)
        vid = v or "057e"
        pid = p
        vid_pid_src = "user entered (raw-hid-only)"

    from raw_hid_capture import try_capture_raw_hid_reports

    raw = try_capture_raw_hid_reports(int(vid, 16), int(pid, 16), PLATFORM_CONTROLS[platform_name])
    label = (raw.get("hid_name") or "").strip() or f"VID_{vid}_PID_{pid}"

    return CaptureState(
        platform=platform_name,
        controller_full_name=label,
        controller_full_name_source="hidraw",
        pygame_name="(SDL/pygame not used — raw-hid-only)",
        pygame_guid="",
        num_axes=0,
        num_buttons=0,
        num_hats=0,
        os_name=f"{platform.system()} {platform.release()}",
        vid=vid,
        pid=pid,
        vid_pid_source=vid_pid_src,
        connection_guess="usb",
        switch2_usb_handshake=switch2_handshake,
        raw_hid_reports=raw,
        capture_mode="raw_hid_only",
    )


def _wrap_hex_lines(hexstr: str, step: int = 32) -> str:
    """Split hex string into lines (default step=32 nibbles = 16 bytes per line)."""
    if not hexstr:
        return ""
    return "\n".join(hexstr[i : i + step] for i in range(0, len(hexstr), step))


def dump_report(st: CaptureState) -> dict[str, Any]:
    return {
        "schema_version": 9,
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "purpose": "OGX-Mini controller support — maintainer mapping capture",
        "maintainer_fields": {
            "capture_mode": st.capture_mode,
            "controller_full_name": st.controller_full_name,
            "controller_full_name_source": st.controller_full_name_source,
            "connection_guess": st.connection_guess,
            "hardware_id": {
                "vid_hex": st.vid,
                "pid_hex": st.pid,
                "vid_pid_source": st.vid_pid_source,
            },
            "sdl": {
                "name": st.pygame_name,
                "guid": st.pygame_guid,
                "axes": st.num_axes,
                "buttons": st.num_buttons,
                "hats": st.num_hats,
            },
            "target_platform_label": st.platform,
            "bindings": st.bindings,
            "baseline_axes": st.baseline_axes,
            "skipped_controls": st.skipped,
            "user_notes": st.notes,
            "host_os": st.os_name,
            "switch2_usb_handshake": st.switch2_usb_handshake,
            "raw_hid_reports": st.raw_hid_reports,
            "hidraw_gated_by_sdl": st.hidraw_gated_by_sdl,
        },
    }


def render_human_text(st: CaptureState, data: dict[str, Any]) -> str:
    lines = [
        "OGX-Mini Controller Mapping Capture",
        "===================================",
        f"Generated (UTC): {data['generated_utc']}",
        f"Host OS: {st.os_name}",
        "",
        "Controller full name (for tracking supported devices):",
        f"  {st.controller_full_name}",
        "  Source: "
        + (
            "SDL / OS reported name"
            if st.controller_full_name_source == "sdl"
            else "Linux hidraw HID_NAME"
            if st.controller_full_name_source == "hidraw"
            else "user-entered retail name"
        ),
        "",
        "Connection (best guess; Linux sysfs only — USB and Bluetooth both supported):",
        f"  {st.connection_guess if st.connection_guess is not None else 'not determined (use USB or Bluetooth; both work with this tool)'}",
        "",
        f"Capture mode: {st.capture_mode}",
        f"Target platform (user selected): {st.platform}",
        f"SDL joystick name (OS report): {st.pygame_name}",
        f"SDL GUID: {st.pygame_guid}",
        f"Axes: {st.num_axes}  Buttons: {st.num_buttons}  Hats: {st.num_hats}",
        "",
        "USB identifiers (for HardwareIDs.h / driver matching):",
        f"  VID: {st.vid or '(unknown)'}",
        f"  PID: {st.pid or '(unknown)'}",
        f"  Source: {st.vid_pid_source}",
        "",
    ]
    if st.switch2_usb_handshake:
        lines.append("Switch 2–family USB bulk handshake (PyUSB, before SDL capture):")
        lines.extend(textwrap.indent(json.dumps(st.switch2_usb_handshake, indent=2), "  ").splitlines())
        lines.append("")
    if st.raw_hid_reports:
        lines.append("Raw HID input reports (Linux hidraw — for firmware bit/byte mapping):")
        lines.extend(textwrap.indent(json.dumps(st.raw_hid_reports, indent=2), "  ").splitlines())
        lines.append("")
    if st.hidraw_gated_by_sdl:
        lines.append("Raw HID alongside SDL (pygame detected input; hidraw bytes in each binding below):")
        lines.extend(textwrap.indent(json.dumps(st.hidraw_gated_by_sdl, indent=2), "  ").splitlines())
        lines.append("")
    lines.append("Per-control mapping (SDL/pygame indices as seen on this PC):")
    if st.capture_mode == "raw_hid_only" and not st.bindings:
        lines.append("  (none — raw-hid-only run; see raw JSON above for hex per control)")
    else:
        for cid, val in sorted(st.bindings.items()):
            lines.append(f"  {cid}: {val}")

    hidraw_blocks: list[str] = []
    for cid, val in sorted(st.bindings.items()):
        if not isinstance(val, dict) or "hidraw_active_hex" not in val:
            continue
        hidraw_blocks.append(f"  [{cid}]")
        if "hidraw_baseline_len" in val:
            hidraw_blocks.append(f"    baseline_len: {val['hidraw_baseline_len']}")
        if "hidraw_active_len" in val:
            hidraw_blocks.append(f"    active_len:   {val['hidraw_active_len']}")
        bh = val.get("hidraw_baseline_hex", "")
        ah = val.get("hidraw_active_hex", "")
        xh = val.get("hidraw_xor_hex", "")
        hidraw_blocks.append("    baseline_hex:")
        for ln in _wrap_hex_lines(bh).splitlines() or [""]:
            hidraw_blocks.append(f"      {ln}")
        hidraw_blocks.append("    active_hex:")
        for ln in _wrap_hex_lines(ah).splitlines() or [""]:
            hidraw_blocks.append(f"      {ln}")
        hidraw_blocks.append("    xor_hex:")
        for ln in _wrap_hex_lines(xh).splitlines() or [""]:
            hidraw_blocks.append(f"      {ln}")
        hidraw_blocks.append("")
    if hidraw_blocks:
        lines.append("")
        lines.append("Full hidraw packets (per control — same data as JSON bindings; 16 bytes per line):")
        lines.extend(hidraw_blocks)
    if st.skipped:
        lines.append("")
        lines.append("Skipped:")
        for s in st.skipped:
            lines.append(f"  - {s}")
    if st.notes:
        lines.append("")
        lines.append("User notes:")
        lines.append(textwrap.indent(st.notes, "  "))
    lines.append("")
    lines.append("--- JSON (same data, machine-readable) ---")
    lines.append(json.dumps(data, indent=2))
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Capture gamepad mappings for OGX-Mini support.")
    parser.add_argument(
        "-o",
        "--output",
        help="Write report to this file (UTF-8). If omitted, prompted at end.",
    )
    parser.add_argument(
        "--skip-raw-hid",
        action="store_true",
        help="Do not offer the Linux hidraw raw report capture phase (SDL/VID capture only).",
    )
    parser.add_argument(
        "--use-sdl-name",
        action="store_true",
        help="Use the SDL joystick name as controller_full_name (skip typing a retail name).",
    )
    parser.add_argument(
        "--auto-output-name",
        action="store_true",
        help="Save the report as <sanitized_controller_name>_<timestamp>.txt without asking (ignored if -o is set).",
    )
    parser.add_argument(
        "--raw-hid-only",
        action="store_true",
        help="Linux only: skip SDL/pygame mapping; only run hidraw raw hex capture (use with Switch 2 USB handshake platform as needed).",
    )
    parser.add_argument(
        "--no-hidraw-sidecar",
        action="store_true",
        help="Do not offer hidraw sampling alongside pygame (Linux).",
    )
    args = parser.parse_args()

    if args.raw_hid_only and args.skip_raw_hid:
        parser.error("--raw-hid-only and --skip-raw-hid cannot be used together.")

    if args.raw_hid_only:
        print(
            textwrap.dedent(
                """
                OGX-Mini — Raw HID capture only (Linux hidraw)
                -----------------------------------------------
                Skips SDL/pygame. You map each control against **raw report bytes** only.
                Pick the same **platform layout** as your controller so prompts match.
                For Switch 2 USB, choose the Switch 2 platform so the bulk handshake runs first.
                """
            ).strip()
        )
    else:
        print(
            textwrap.dedent(
                """
                OGX-Mini — Controller mapping capture
                -------------------------------------
                This tool records how your PC sees each stick, button, and trigger
                (via SDL/pygame). Use a **USB** cable **or** **Bluetooth** — if the
                controller shows up in the list below, you can capture it.
                Upload the saved file when requesting support for a new controller
                in OGX-Mini.
                """
            ).strip()
        )

    platform_name = choose_platform()
    switch2_handshake: dict[str, Any] | None = None
    if platform_requires_switch2_usb_init(platform_name):
        print(
            textwrap.dedent(
                """

                ------------------------------------------------------------------
                Switch 2 Pro / Joy-Con 2 (USB) — USB bulk handshake
                ------------------------------------------------------------------
                These controllers often do **not** appear in SDL until a host sends
                Nintendo's USB vendor sequence on **interface 1** (same idea as
                HandHeldLegend's ProCon 2 Enabler in Chrome).

                • Install:  pip install pyusb
                • Linux:    needs **libusb**; you may need **udev** rules or run
                            briefly with sudo if access to the device is denied.
                • Windows:  interface 1 may need **WinUSB** (e.g. Zadig) — YMMV.
                • Use a **data** USB cable; plug the controller in **before** Enter.

                """
            ).rstrip()
        )
        input("Press Enter when the controller is connected over USB… ")
        switch2_handshake = run_switch2_usb_handshake()
        print(switch2_handshake.get("message", ""))
        if not switch2_handshake.get("ok"):
            if args.raw_hid_only:
                print(
                    "\nHandshake did not complete. You can still enter VID/PID manually for hidraw "
                    "(e.g. PID 2069 for Pro Controller 2).\n"
                )
            else:
                cont = input("Handshake failed. Try SDL capture anyway? [y/N]: ").strip().lower()
                if cont not in ("y", "yes"):
                    print("Exiting.")
                    return
        time.sleep(0.8)

    if args.raw_hid_only:
        st = run_raw_hid_only_capture(platform_name, switch2_handshake)
    else:
        init_pygame()
        joy, js_index = choose_joystick()

        try:
            st = run_capture_loop(
                joy,
                platform_name,
                js_index,
                switch2_usb_handshake=switch2_handshake,
                use_sdl_name=args.use_sdl_name,
                enable_hidraw_sidecar=not args.no_hidraw_sidecar,
            )
        finally:
            joy.quit()
            import pygame

            pygame.quit()

    # Optional second phase: full raw HID report bytes (Linux hidraw) for maintainers.
    if (
        not args.raw_hid_only
        and not args.skip_raw_hid
        and platform.system() == "Linux"
        and st.raw_hid_reports is None
    ):
        pair = resolve_vid_pid_for_raw_capture(st)
        if pair is not None:
            vid_i, pid_i = pair
            has_sdl_hidraw = any(
                isinstance(v, dict) and "hidraw_active_hex" in v for v in st.bindings.values()
            )
            if has_sdl_hidraw:
                ans = (
                    input(
                        "\nThis report already has **raw hidraw hex** on each binding (SDL detected the press).\n"
                        "Run the **extra** interactive hidraw walkthrough too (idle-based)? Usually **not** needed. [y/N]: "
                    )
                    .strip()
                    .lower()
                )
                do_walkthrough = ans in ("y", "yes")
            else:
                ans = (
                    input(
                        "\nAlso capture **raw HID** in the **interactive hidraw-only** walkthrough?\n"
                        "(Enable **SDL+hidraw** sidecar next time to avoid this — pygame gates each sample.) [Y/n]: "
                    )
                    .strip()
                    .lower()
                )
                do_walkthrough = ans in ("", "y", "yes")
            if do_walkthrough:
                from raw_hid_capture import try_capture_raw_hid_reports

                st.raw_hid_reports = try_capture_raw_hid_reports(
                    vid_i, pid_i, PLATFORM_CONTROLS[platform_name]
                )
                if not st.raw_hid_reports.get("ok"):
                    print(st.raw_hid_reports.get("message", "Raw hidraw capture did not complete."))
        else:
            print(
                "\n(Skip raw hidraw: no VID/PID in report — fill VID/PID at end of capture next time, "
                "or use Switch 2 USB handshake so product_id is known.)"
            )

    data = dump_report(st)
    text = render_human_text(st, data)

    out_path = args.output
    if not out_path:
        suggested = default_report_filename(st)
        if args.auto_output_name:
            out_path = suggested
        else:
            ans = input("\nSave report to file? [Y/n]: ").strip().lower()
            if ans in ("", "y", "yes"):
                path = input(f"Filename [{suggested}]: ").strip() or suggested
                out_path = path

    if out_path:
        with open(out_path, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"\nWrote: {out_path}\n")
    else:
        print("\n--- Report ---\n")
        print(text)

    print("Thank you. Attach the file to your issue or send it to the maintainer.")


if __name__ == "__main__":
    main()
