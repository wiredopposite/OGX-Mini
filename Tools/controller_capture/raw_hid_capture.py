#!/usr/bin/env python3
"""
Read raw HID input reports from Linux hidraw nodes (by USB VID/PID).

Used by controller_capture.py so maintainers get **actual report bytes** (hex),
not only SDL button/axis indices — needed to match SwitchPro-style packed layouts
in firmware.
"""

from __future__ import annotations

import glob
import os
import select
import sys
import time
from typing import Any


def _parse_uevent(content: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in content.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def list_hidraw_nodes_for_vid_pid(vid: int, pid: int) -> list[dict[str, Any]]:
    """Return hidraw device paths whose HID_ID matches vid:pid (USB HID)."""
    found: list[dict[str, Any]] = []
    for hidraw_dir in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        uevent_path = os.path.join(hidraw_dir, "device", "uevent")
        if not os.path.isfile(uevent_path):
            continue
        try:
            with open(uevent_path, encoding="utf-8") as f:
                props = _parse_uevent(f.read())
        except OSError:
            continue
        hid_id = props.get("HID_ID")
        if not hid_id:
            continue
        parts = hid_id.split(":")
        if len(parts) != 3:
            continue
        try:
            _bus, v_s, p_s = parts
            v = int(v_s, 16)
            p = int(p_s, 16)
        except ValueError:
            continue
        if v != vid or p != pid:
            continue
        name = os.path.basename(hidraw_dir)
        found.append(
            {
                "path": f"/dev/{name}",
                "hid_name": props.get("HID_NAME", ""),
                "hid_id": hid_id,
                "sysfs": hidraw_dir,
            }
        )
    return found


def _open_hidraw_nonblock(path: str) -> int:
    return os.open(path, os.O_RDONLY | os.O_NONBLOCK)


def _read_latest_packet(fd: int, timeout_s: float, bufsize: int = 512) -> bytes | None:
    """Drain readable data within timeout; return last chunk read (one report per read typical)."""
    deadline = time.monotonic() + timeout_s
    last: bytes | None = None
    while time.monotonic() < deadline:
        r, _, _ = select.select([fd], [], [], min(0.08, max(0.0, deadline - time.monotonic())))
        if not r:
            continue
        try:
            while True:
                chunk = os.read(fd, bufsize)
                if not chunk:
                    break
                last = chunk
        except BlockingIOError:
            break
    return last


# Pro / Switch 2 style full input: byte0 report ID, byte1 timer/counter, then 10-byte
# SwitchPro::InReport (battery + 3 button bytes + 6 stick bytes). IMU follows in tail.
_NINTENDO_FULL_INPUT_IDS = frozenset({0x30, 0x09, 0x31})


def _nintendo_gamepad_core_key(pkt: bytes) -> bytes:
    """10 bytes at offset 2; relax stick packing bytes (indices 4..9) for idle match."""
    core = bytearray(pkt[2:12])
    if len(core) >= 10:
        for i in range(4, 10):
            core[i] &= 0xFC  # stronger than 0xFE — Switch 2 sticks often jitter 1–2 LSBs
    return bytes(core)


def _idle_compare_key(pkt: bytes) -> bytes:
    """
    Bytes that should stay fixed at neutral. Full 64-byte reports include a
    per-frame timer (byte 1) and IMU tail — never compare the whole packet.

    Switch 2 Pro often uses report ID **0x09** (not 0x30); layout matches the same core.
    """
    if len(pkt) >= 12 and pkt[0] in _NINTENDO_FULL_INPUT_IDS:
        return _nintendo_gamepad_core_key(pkt)
    # Long reports (other vendors): compare head only, drop IMU tail.
    if len(pkt) >= 48:
        head = bytearray(pkt[:24])
        for i in range(3, min(24, len(head))):
            if i >= 6:
                head[i] &= 0xFC
        return bytes(head)
    return pkt


def _wait_stable_report(
    fd: int,
    *,
    settle_s: float = 0.22,
    overall_timeout_s: float = 45.0,
) -> bytes | None:
    """Wait until consecutive samples match on idle-compare key (not full packet)."""
    last_pkt: bytes | None = None
    last_key: bytes | None = None
    stable_since: float | None = None
    start = time.monotonic()
    packet_count = 0
    last_progress = start
    saw_packet = False

    while time.monotonic() - start < overall_timeout_s:
        elapsed = time.monotonic() - start
        # After input, sticks may take a moment to settle; allow shorter "quiet" window.
        effective_settle = settle_s
        if elapsed > 8.0:
            effective_settle = min(settle_s, 0.14)
        if elapsed > 16.0:
            effective_settle = min(effective_settle, 0.09)
        if elapsed > 24.0:
            effective_settle = min(effective_settle, 0.06)

        pkt = _read_latest_packet(fd, timeout_s=0.12)
        now = time.monotonic()

        if pkt is None:
            if saw_packet and now - last_progress >= 0.6:
                print(
                    "\r    … waiting for hidraw data (move a stick slightly if nothing arrives)    ",
                    end="",
                    flush=True,
                )
                last_progress = now
            time.sleep(0.02)
            continue

        saw_packet = True
        packet_count += 1
        key = _idle_compare_key(pkt)

        if now - last_progress >= 0.45:
            print(
                f"\r    … {packet_count} reports, last len={len(pkt)} byte0=0x{pkt[0]:02x} (idle: core bytes 2:12)    ",
                end="",
                flush=True,
            )
            last_progress = now

        if last_key is not None and key == last_key:
            last_pkt = pkt  # keep latest full frame while idle-core is stable
            if stable_since is None:
                stable_since = now
            elif now - stable_since >= effective_settle:
                print()  # newline after progress
                return last_pkt
        else:
            stable_since = None
            last_pkt = pkt
            last_key = key
        time.sleep(0.008)

    print()
    return last_pkt


def _wait_changed_report(fd: int, baseline: bytes, timeout_s: float = 60.0) -> bytes | None:
    base_key = _idle_compare_key(baseline)
    start = time.monotonic()
    while time.monotonic() - start < timeout_s:
        pkt = _read_latest_packet(fd, timeout_s=0.12)
        if pkt is not None and _idle_compare_key(pkt) != base_key:
            return pkt
        time.sleep(0.008)
    return None


def _hex(b: bytes) -> str:
    return b.hex()


def peek_latest_hidraw(fd: int, timeout_s: float = 0.1) -> bytes | None:
    """Drain hidraw within timeout; return the last full report read (for SDL-gated sampling)."""
    return _read_latest_packet(fd, timeout_s)


def xor_hex_reports(a: bytes, b: bytes) -> str:
    """Hex string of a XOR b (padded to max length)."""
    return _xor_hex(a, b)


def _xor_hex(a: bytes, b: bytes) -> str:
    n = max(len(a), len(b))
    aa = a.ljust(n, b"\x00")
    bb = b.ljust(n, b"\x00")
    return bytes(x ^ y for x, y in zip(aa, bb)).hex()


def choose_hidraw_device(
    candidates: list[dict[str, Any]],
    *,
    quit_hint: str = "q to skip",
) -> dict[str, Any] | None:
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]
    print("\nMultiple hidraw nodes match this VID:PID. Pick the one that is the **gamepad** input:\n")
    for i, c in enumerate(candidates, 1):
        hn = c.get("hid_name") or "(no HID_NAME)"
        print(f"  {i}. {c['path']}\n       HID_NAME: {hn}\n       {c['hid_id']}")
    while True:
        raw = input(f"\nEnter number (or '{quit_hint}'): ").strip().lower()
        if raw == "q":
            return None
        try:
            idx = int(raw)
            if 1 <= idx <= len(candidates):
                return candidates[idx - 1]
        except ValueError:
            pass
        print("Invalid choice.")


def try_open_hidraw_for_sidecar(vid: int, pid: int) -> tuple[int | None, str | None]:
    """
    Open one hidraw fd for SDL-gated sampling: pygame decides *when* input happened;
    caller peeks this fd right after.
    """
    candidates = list_hidraw_nodes_for_vid_pid(vid, pid)
    if not candidates:
        print(
            f"(No /dev/hidraw* for VID {vid:04x} PID {pid:04x} — raw bytes will not be recorded alongside SDL.)"
        )
        return None, None
    if len(candidates) == 1:
        chosen = candidates[0]
    else:
        print("\nSeveral hidraw nodes match — pick the **gamepad** for raw HID alongside pygame:")
        chosen = choose_hidraw_device(candidates, quit_hint="q to skip sidecar")
    if chosen is None:
        return None, None
    path = chosen["path"]
    try:
        fd = _open_hidraw_nonblock(path)
        print(
            f"(Opened {path} for raw HID. **Pygame** still decides when you pressed; we only read bytes then.)\n"
        )
        return fd, path
    except OSError as e:
        print(f"(Could not open hidraw for sidecar: {e})")
        return None, None


def close_hidraw_fd(fd: int | None) -> None:
    if fd is not None:
        try:
            os.close(fd)
        except OSError:
            pass


def try_capture_raw_hid_reports(
    vid: int,
    pid: int,
    controls: list[tuple[str, str]],
    *,
    hidraw_path: str | None = None,
) -> dict[str, Any]:
    """
    Interactive session: for each logical control, capture baseline vs active raw report.

    Returns a dict suitable for JSON under maintainer_fields.raw_hid_reports.
    """
    if sys.platform != "linux" and not sys.platform.startswith("linux"):
        return {
            "ok": False,
            "platform": "non-linux",
            "message": "Raw hidraw capture is only implemented on Linux. Use Linux for full byte dumps, "
            "or attach a USB capture / Wireshark for other OSes.",
        }

    candidates = list_hidraw_nodes_for_vid_pid(vid, pid)
    if not candidates:
        return {
            "ok": False,
            "platform": "linux_hidraw",
            "message": f"No /dev/hidraw* found for VID {vid:04x} PID {pid:04x}. "
            "Check lsusb, permissions (plugdev/udev), and that the controller exposes HID.",
            "vid_hex": f"{vid:04x}",
            "pid_hex": f"{pid:04x}",
        }

    chosen = None
    if hidraw_path:
        for c in candidates:
            if c["path"] == hidraw_path:
                chosen = c
                break
        if chosen is None:
            return {
                "ok": False,
                "platform": "linux_hidraw",
                "message": f"Given hidraw path {hidraw_path!r} does not match enumerated nodes for this VID:PID.",
                "candidates": candidates,
            }
    else:
        chosen = choose_hidraw_device(candidates, quit_hint="q to skip raw capture")
        if chosen is None:
            return {
                "ok": False,
                "platform": "linux_hidraw",
                "skipped": True,
                "message": "User skipped hidraw selection.",
            }

    path = chosen["path"]
    fd: int | None = None
    try:
        fd = _open_hidraw_nonblock(path)
    except OSError as e:
        return {
            "ok": False,
            "platform": "linux_hidraw",
            "path": path,
            "message": f"Could not open {path}: {e}. Try udev rules or `sudo` for a one-off test.",
            "candidates": candidates,
        }

    samples: list[dict[str, Any]] = []
    max_len = 0

    print(
        "\n"
        + "=" * 72
        + "\nRaw HID reports (linux hidraw)\n"
        + "=" * 72
        + f"\nReading from: {path}\n"
        f"HID_NAME: {chosen.get('hid_name', '')}\n\n"
        "For each step: return to **neutral** (release buttons, center sticks), then press/move **only**\n"
        "the named control. The tool records full report bytes (hex) for firmware maintainers.\n\n"
        "Note: Nintendo full reports change every frame (timer + IMU). Idle detection uses bytes **2:12**\n"
        "(gamepad core; works for report IDs **0x30** and **Switch 2’s 0x09**). If neutral takes long after a\n"
        "button, release fully and center sticks — settle time shortens automatically after ~8s.\n"
        "Commands: Enter=continue  s=skip control  q=quit raw phase\n"
    )

    try:
        i = 0
        while i < len(controls):
            cid, label = controls[i]
            print(f"\n>>> [{i + 1}/{len(controls)}]  RAW  {label}")
            print("    Release everything; center sticks — waiting for stable idle report...")
            baseline = _wait_stable_report(fd)
            if baseline is None:
                print("    No HID data (timeout). Wiggle a stick slightly or press a button once, then release.")
                retry = input("    Retry this step? [Y/n/q quit]: ").strip().lower()
                if retry == "q":
                    break
                if retry == "n":
                    samples.append(
                        {
                            "control_id": cid,
                            "label": label,
                            "error": "timeout waiting for stable baseline",
                        }
                    )
                    i += 1
                continue

            max_len = max(max_len, len(baseline))
            print(f"    Baseline length: {len(baseline)} bytes")
            print("    Now press or move ONLY the control above. Waiting for report change...")
            active = _wait_changed_report(fd, baseline)
            if active is None:
                print("    No change seen.")
                ans = input("    Retry [Y/n/s skip]: ").strip().lower()
                if ans == "q":
                    break
                if ans == "s":
                    samples.append(
                        {
                            "control_id": cid,
                            "label": label,
                            "baseline_hex": _hex(baseline),
                            "skipped": True,
                        }
                    )
                    i += 1
                continue

            max_len = max(max_len, len(active))
            samples.append(
                {
                    "control_id": cid,
                    "label": label,
                    "baseline_hex": _hex(baseline),
                    "active_hex": _hex(active),
                    "xor_hex": _xor_hex(baseline, active),
                    "baseline_len": len(baseline),
                    "active_len": len(active),
                }
            )
            print(f"    Captured: +{len(active) - len(baseline)} len delta" if len(active) != len(baseline) else "    Captured.")
            i += 1
            cmd = input("Enter=continue | s=skip next | q=quit raw: ").strip().lower()
            if cmd == "q":
                break
            if cmd == "s" and i < len(controls):
                cid2, lab2 = controls[i]
                samples.append({"control_id": cid2, "label": lab2, "skipped": True})
                i += 1
    finally:
        if fd is not None:
            os.close(fd)

    session_ok = bool(samples)
    return {
        "ok": session_ok,
        "platform": "linux_hidraw",
        "path": path,
        "hid_name": chosen.get("hid_name", ""),
        "hid_id": chosen.get("hid_id", ""),
        "vid_hex": f"{vid:04x}",
        "pid_hex": f"{pid:04x}",
        "max_report_len_seen": max_len,
        "sample_count": len(samples),
        "samples": samples,
        "note": "Bytes are as returned by read(2) on hidraw (typically includes report ID as byte 0). "
        "Compare xor_hex to map bits/bytes for firmware decode tables. "
        "Idle detection uses bytes 2:12 gamepad core (report IDs 0x30 / 0x09 / 0x31), not timer/IMU; "
        "stick bytes masked with 0xFC for neutral matching.",
    }
