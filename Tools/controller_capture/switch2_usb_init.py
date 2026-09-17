"""
Nintendo Switch 2–family USB bulk handshake for PC capture.

The Pro Controller 2 (PID 0x2069), Joy-Con 2 L/R (0x2067/0x2066), and related
devices do not stream standard HID gamepad input over USB until the host sends
a vendor protocol on USB interface 1 (bulk OUT), matching the sequence used
by HandHeldLegend's browser tool:

  https://github.com/HandHeldLegend/handheldlegend.github.io
  (procon2tool/index.html — "ProCon 2 Enabler Tool")

After this runs, the OS may expose the pad to SDL/pygame for mapping capture.

Requires: PyUSB + system libusb (Linux: libusb-1.0; Windows often needs WinUSB via Zadig for the bulk interface).
"""

from __future__ import annotations

import time
from typing import Any

# Nintendo Co., Ltd.
VENDOR_ID = 0x057E

# Switch 2–era USB IDs (same as HHL dumper / enabler)
PRODUCT_IDS: dict[int, str] = {
    0x2066: "Joy-Con 2 (R)",
    0x2067: "Joy-Con 2 (L)",
    0x2069: "Pro Controller 2",
    0x2073: "NSO GameCube Controller",
}

USB_CONFIGURATION = 1
USB_INTERFACE = 1

# Packets copied from handheldlegend.github.io procon2tool/index.html (auto sequence on USB connect).
# Placeholder 0xFF bytes in MAC/LTK fields match the reference tool for PC use.

_INIT_COMMAND_0x03 = bytes(
    [
        0x03,
        0x91,
        0x00,
        0x0D,
        0x00,
        0x08,
        0x00,
        0x00,
        0x01,
        0x00,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    ]
)

_UNKNOWN_COMMAND_0x07 = bytes([0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00])

_UNKNOWN_COMMAND_0x16 = bytes([0x16, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00])

_REQUEST_CONTROLLER_MAC = bytes(
    [
        0x15,
        0x91,
        0x00,
        0x01,
        0x00,
        0x0E,
        0x00,
        0x00,
        0x00,
        0x02,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    ]
)

_LTK_REQUEST = bytes(
    [
        0x15,
        0x91,
        0x00,
        0x02,
        0x00,
        0x11,
        0x00,
        0x00,
        0x00,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    ]
)

_UNKNOWN_COMMAND_0x15_ARG_0x03 = bytes([0x15, 0x91, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00])

_UNKNOWN_COMMAND_0x09 = bytes(
    [
        0x09,
        0x91,
        0x00,
        0x07,
        0x00,
        0x08,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    ]
)

_IMU_COMMAND_0x02 = bytes([0x0C, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00])

_OUT_UNKNOWN_COMMAND_0x11 = bytes([0x11, 0x91, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00])

_UNKNOWN_COMMAND_0x0A = bytes(
    [
        0x0A,
        0x91,
        0x00,
        0x08,
        0x00,
        0x14,
        0x00,
        0x00,
        0x01,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x35,
        0x00,
        0x46,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    ]
)

_IMU_COMMAND_0x04 = bytes([0x0C, 0x91, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00])

_ENABLE_HAPTICS = bytes([0x03, 0x91, 0x00, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00])

_OUT_UNKNOWN_COMMAND_0x10 = bytes([0x10, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00])

_OUT_UNKNOWN_COMMAND_0x01 = bytes([0x01, 0x91, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00])

_OUT_UNKNOWN_COMMAND_0x03 = bytes([0x03, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00])

_OUT_UNKNOWN_COMMAND_0x0A_ALT = bytes([0x0A, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00])

_SET_PLAYER_LED = bytes(
    [
        0x09,
        0x91,
        0x00,
        0x07,
        0x00,
        0x08,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    ]
)

# Same order as HHL connectUsb() automatic sequence (SPI reads omitted there too).
SWITCH2_USB_INIT_PACKETS: list[bytes] = [
    _INIT_COMMAND_0x03,
    _UNKNOWN_COMMAND_0x07,
    _UNKNOWN_COMMAND_0x16,
    _REQUEST_CONTROLLER_MAC,
    _LTK_REQUEST,
    _UNKNOWN_COMMAND_0x15_ARG_0x03,
    _UNKNOWN_COMMAND_0x09,
    _IMU_COMMAND_0x02,
    _OUT_UNKNOWN_COMMAND_0x11,
    _UNKNOWN_COMMAND_0x0A,
    _IMU_COMMAND_0x04,
    _ENABLE_HAPTICS,
    _OUT_UNKNOWN_COMMAND_0x10,
    _OUT_UNKNOWN_COMMAND_0x01,
    _OUT_UNKNOWN_COMMAND_0x03,
    _OUT_UNKNOWN_COMMAND_0x0A_ALT,
    _SET_PLAYER_LED,
]


def _find_bulk_endpoints(interface) -> tuple[Any, Any]:
    import usb.util

    ep_out = None
    ep_in = None
    for ep in interface:
        t = usb.util.endpoint_type(ep.bEndpointAddress)
        d = usb.util.endpoint_direction(ep.bEndpointAddress)
        if t != usb.util.ENDPOINT_TYPE_BULK:
            continue
        if d == usb.util.ENDPOINT_OUT:
            ep_out = ep
        elif d == usb.util.ENDPOINT_IN:
            ep_in = ep
    return ep_out, ep_in


def run_switch2_usb_handshake(
    *,
    read_timeout_ms: int = 80,
    pause_s: float = 0.012,
    read_size: int = 64,
) -> dict[str, Any]:
    """
    Open the first matching Switch 2–family device, claim interface 1, send init packets.

    Returns a dict with keys: ok (bool), product_id, product_name, message, error (optional).
    """
    try:
        import usb.core
        import usb.util
    except ImportError as e:
        return {
            "ok": False,
            "product_id": None,
            "product_name": None,
            "message": "PyUSB is not installed. Run: pip install pyusb",
            "error": str(e),
        }

    dev = None
    found_pid = None
    for pid in PRODUCT_IDS:
        dev = usb.core.find(idVendor=VENDOR_ID, idProduct=pid)
        if dev is not None:
            found_pid = pid
            break

    if dev is None:
        return {
            "ok": False,
            "product_id": None,
            "product_name": None,
            "message": (
                f"No USB device found for VID {VENDOR_ID:04X} and PIDs "
                + ", ".join(f"{p:04X}" for p in PRODUCT_IDS)
                + ". Plug the controller in with a data cable and try again."
            ),
            "error": None,
        }

    name = PRODUCT_IDS.get(found_pid or 0, "unknown")
    out: dict[str, Any] = {
        "ok": False,
        "product_id": f"{found_pid:04x}" if found_pid is not None else None,
        "product_name": name,
        "message": "",
        "error": None,
    }

    detached_kernel = False
    try:
        try:
            if dev.is_kernel_driver_active(USB_INTERFACE):
                dev.detach_kernel_driver(USB_INTERFACE)
                detached_kernel = True
                out["detached_kernel_driver"] = True
            else:
                out["detached_kernel_driver"] = False
        except (NotImplementedError, ValueError, usb.core.USBError):
            out["detached_kernel_driver"] = None

        try:
            dev.set_configuration()
        except usb.core.USBError:
            pass  # already configured

        cfg = dev.get_active_configuration()
        intf = cfg[(USB_INTERFACE, 0)]
        ep_out, ep_in = _find_bulk_endpoints(intf)

        if ep_out is None:
            out["message"] = "No bulk OUT endpoint on interface 1 — wrong device or driver?"
            out["error"] = "missing_bulk_out"
            return out

        usb.util.claim_interface(dev, USB_INTERFACE)
        try:
            for pkt in SWITCH2_USB_INIT_PACKETS:
                ep_out.write(pkt)
                time.sleep(pause_s)
                if ep_in is not None:
                    try:
                        ep_in.read(read_size, timeout=read_timeout_ms)
                    except usb.core.USBError:
                        pass
            out["ok"] = True
            out["packets_sent"] = len(SWITCH2_USB_INIT_PACKETS)
            out["message"] = (
                f"Handshake OK ({name}, PID {found_pid:04X}). "
                "Wait a moment, then SDL should list the controller."
            )
        finally:
            usb.util.release_interface(dev, USB_INTERFACE)

    except Exception as e:
        out["message"] = f"USB handshake failed: {e}"
        out["error"] = repr(e)
        try:
            usb.util.dispose_resources(dev)
        except Exception:
            pass
        return out
    finally:
        if detached_kernel:
            try:
                dev.attach_kernel_driver(USB_INTERFACE)
            except (usb.core.USBError, NotImplementedError, ValueError):
                pass

    return out


def platform_requires_switch2_usb_init(platform_label: str) -> bool:
    """True for the capture platform that runs the HHL-compatible bulk handshake first."""
    return "libusb handshake" in platform_label
