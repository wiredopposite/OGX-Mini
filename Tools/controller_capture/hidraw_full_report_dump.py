#!/usr/bin/env python3
"""
Stream every full HID input report from Linux hidraw as hex (one line per read).

Use this to see **which byte indices** change when you hold one button vs idle, without
going through the interactive per-control capture. Helps separate:
  - stable button bits (good for firmware)
  - byte 1 (timer) and IMU tail bytes that move every frame (bad to match naïvely)

Examples:
  python3 hidraw_full_report_dump.py --vid 057e --pid 2069
  python3 hidraw_full_report_dump.py --path /dev/hidraw4 --diff
  python3 hidraw_full_report_dump.py --vid 057e --pid 2069 --diff-core --max 500

Requires: Linux, read access on /dev/hidraw* (plugdev/udev).
"""

from __future__ import annotations

import argparse
import os
import select
import sys
import time

from raw_hid_capture import _idle_compare_key, choose_hidraw_device, list_hidraw_nodes_for_vid_pid


def _open_nonblock(path: str) -> int:
    return os.open(path, os.O_RDONLY | os.O_NONBLOCK)


def _drain_one_report(fd: int, bufsize: int = 512) -> bytes | None:
    r, _, _ = select.select([fd], [], [], 0.25)
    if not r:
        return None
    try:
        return os.read(fd, bufsize)
    except BlockingIOError:
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description="Dump full hidraw input reports as hex (Linux).")
    ap.add_argument("--vid", help="USB VID hex, e.g. 057e")
    ap.add_argument("--pid", help="USB PID hex, e.g. 2069")
    ap.add_argument("--path", help="Explicit hidraw device (skips VID/PID lookup)")
    ap.add_argument(
        "--diff",
        action="store_true",
        help="Only print lines when the full report differs from the previous printed line.",
    )
    ap.add_argument(
        "--diff-core",
        action="store_true",
        help="Only print when Nintendo-style core key (bytes 2:12, stick LSBs masked) changes. "
        "Useful to ignore pure IMU tail jitter while still seeing button/stick-core changes.",
    )
    ap.add_argument("--max", type=int, default=0, metavar="N", help="Stop after N printed lines (0 = unlimited)")
    ap.add_argument("--count", action="store_true", help="Prefix each line with a monotonic line number")
    args = ap.parse_args()

    if sys.platform != "linux" and not sys.platform.startswith("linux"):
        print("This tool only runs on Linux (hidraw).", file=sys.stderr)
        return 1

    path: str | None = args.path
    if not path:
        if not args.vid or not args.pid:
            print("Provide --path or both --vid and --pid.", file=sys.stderr)
            return 1
        try:
            vid = int(args.vid, 16)
            pid = int(args.pid, 16)
        except ValueError:
            print("Invalid --vid or --pid (use hex, e.g. 057e).", file=sys.stderr)
            return 1
        candidates = list_hidraw_nodes_for_vid_pid(vid, pid)
        if not candidates:
            print(f"No hidraw node for VID {vid:04x} PID {pid:04x}.", file=sys.stderr)
            return 1
        chosen = candidates[0] if len(candidates) == 1 else choose_hidraw_device(candidates)
        if chosen is None:
            return 1
        path = chosen["path"]
        print(f"# {chosen.get('hid_name', '')} {path} {chosen.get('hid_id', '')}", flush=True)

    assert path is not None
    try:
        fd = _open_nonblock(path)
    except OSError as e:
        print(f"Open {path}: {e}", file=sys.stderr)
        return 1

    print(
        "# byte indices (hex):  " + " ".join(f"{i:02x}" for i in range(16)),
        "\n#                    " + " ".join(f"{i:02x}" for i in range(16, 32)),
        "\n# Ctrl+C to stop.\n",
        sep="",
        flush=True,
    )

    prev_full: bytes | None = None
    prev_key: bytes | None = None
    n_printed = 0
    line_no = 0
    try:
        while args.max <= 0 or n_printed < args.max:
            pkt = _drain_one_report(fd)
            if pkt is None:
                continue

            if args.diff_core:
                key = _idle_compare_key(pkt)
                if prev_key is not None and key == prev_key:
                    continue
                prev_key = key
            elif args.diff:
                if prev_full is not None and pkt == prev_full:
                    continue
                prev_full = pkt
            else:
                prev_full = pkt

            line_no += 1
            h = pkt.hex()
            prefix = f"{line_no:6d}  " if args.count else ""
            print(f"{prefix}len={len(pkt):3d}  {h}", flush=True)
            n_printed += 1
    except KeyboardInterrupt:
        print("\n# stopped", flush=True)
    finally:
        os.close(fd)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
