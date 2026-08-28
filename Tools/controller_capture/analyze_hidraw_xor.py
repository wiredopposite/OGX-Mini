#!/usr/bin/env python3
"""
Load a controller_capture .txt (schema_version 8+ JSON) and summarize hidraw XOR patterns.

Use this to find which report byte indices change for each control, and to check
collisions (same bytes changing for two different controls — often means you need
more bytes in the signature or a different neutral pose).

Usage:
  python3 analyze_hidraw_xor.py path/to/capture.txt
  python3 analyze_hidraw_xor.py path/to/capture.txt --max-index 31
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def load_json_block(path: Path) -> dict:
    text = path.read_text(encoding="utf-8")
    start = text.find('{\n  "schema_version"')
    if start < 0:
        start = text.find('{"schema_version"')
    if start < 0:
        raise ValueError("No JSON block with schema_version found")
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return json.loads(text[start : i + 1])
    raise ValueError("Unclosed JSON")


def hx(s: str) -> bytes:
    s = re.sub(r"\s+", "", s)
    return bytes.fromhex(s)


def nz(x: bytes, hi: int) -> list[tuple[int, int]]:
    return [(i, x[i]) for i in range(min(len(x), hi)) if x[i] != 0]


def main() -> int:
    ap = argparse.ArgumentParser(description="Summarize hidraw XOR per binding from capture JSON")
    ap.add_argument("capture_txt", type=Path)
    ap.add_argument("--max-index", type=int, default=24, help="Max byte index to list (exclusive)")
    ap.add_argument("--skip-timer", action="store_true", help="Omit index 1 from printed XOR list")
    args = ap.parse_args()

    data = load_json_block(args.capture_txt)
    bindings = data.get("maintainer_fields", {}).get("bindings", {})
    if not bindings:
        print("No bindings in JSON", file=sys.stderr)
        return 1

    hi = max(16, args.max_index)
    rows: list[tuple[str, bytes, bytes]] = []
    for name in sorted(bindings.keys()):
        b = bindings[name]
        if "hidraw_baseline_hex" not in b or "hidraw_active_hex" not in b:
            continue
        base = hx(b["hidraw_baseline_hex"])
        act = hx(b["hidraw_active_hex"])
        if len(base) != len(act):
            print(f"{name}: baseline/active length mismatch", file=sys.stderr)
            continue
        x = bytes(a ^ b for a, b in zip(base, act))
        rows.append((name, x, act))

    print(f"# Report length: {len(rows[0][1])} bytes (first binding)\n")
    for name, x, act in rows:
        pairs = nz(x, hi)
        if args.skip_timer:
            pairs = [(i, v) for i, v in pairs if i != 1]
        sig = act[21:24].hex() if len(act) >= 24 else ""
        print(f"{name}:")
        print(f"  xor[0..{hi}): " + " ".join(f"{i:02d}:{v:02x}" for i, v in pairs))
        if sig:
            print(f"  active[21:24]: {sig}")
        print()

    # Collision check on active[21:22] for controls that have hidraw
    d: dict[tuple[int, int], list[str]] = {}
    for name, _x, act in rows:
        if len(act) < 23:
            continue
        key = (act[21], act[22])
        d.setdefault(key, []).append(name)
    dup = {k: v for k, v in d.items() if len(v) > 1}
    if dup:
        print("# active[21:22] collisions (need longer signature or not discriminant):")
        for k, names in sorted(dup.items()):
            print(f"  {k[0]:02x} {k[1]:02x}: {', '.join(names)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
