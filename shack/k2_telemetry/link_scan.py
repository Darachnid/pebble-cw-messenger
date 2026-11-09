#!/usr/bin/env python3
"""Find KIO2 serial settings by probing for TQ0/TQ1 or PS1 responses."""

from __future__ import annotations

import argparse
import json
import re
import sys
import time

try:
    import serial
except ImportError:
    serial = None  # type: ignore

GOOD = re.compile(r"^(TQ[01]|PS1|IF\d{11}|ID017);", re.IGNORECASE)


def _try_once(port: str, baud: int, stopbits: int, dtr: bool) -> dict:
    assert serial is not None
    sb = serial.STOPBITS_TWO if stopbits == 2 else serial.STOPBITS_ONE
    ser = serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=sb,
        timeout=0.4,
        rtscts=False,
        dsrdtr=False,
    )
    try:
        ser.dtr = dtr
        ser.rts = dtr
        time.sleep(0.2)
        ser.reset_input_buffer()

        for cmd in (b"AI0;", b"TQ;", b"PS;"):
            ser.reset_input_buffer()
            ser.write(cmd)
            time.sleep(0.05)
            deadline = time.monotonic() + 0.5
            buf = bytearray()
            lines: list[str] = []
            while time.monotonic() < deadline:
                chunk = ser.read(256)
                if chunk:
                    buf.extend(chunk)
                    while b";" in buf:
                        i = buf.index(b";")
                        raw = bytes(buf[: i + 1])
                        del buf[: i + 1]
                        text = raw.decode("ascii", errors="replace").strip()
                        if text:
                            lines.append(text)
                else:
                    time.sleep(0.01)

            for line in lines:
                if GOOD.match(line.upper()):
                    return {
                        "ok": True,
                        "baud": baud,
                        "stopbits": stopbits,
                        "dtr_rts": dtr,
                        "command": cmd.decode(),
                        "line": line,
                        "all_lines": lines,
                    }

            if lines:
                return {
                    "ok": False,
                    "baud": baud,
                    "stopbits": stopbits,
                    "dtr_rts": dtr,
                    "command": cmd.decode(),
                    "all_lines": lines,
                    "hint": "got data but not valid K2 CAT",
                }
    finally:
        ser.close()

    return {
        "ok": False,
        "baud": baud,
        "stopbits": stopbits,
        "dtr_rts": dtr,
        "hint": "no response",
    }


def scan(port: str) -> dict:
    if serial is None:
        return {"ok": False, "error": "pip install pyserial in .venv"}

    attempts = []
    for baud in (4800, 9600, 38400, 57600):
        for stop in (2, 1):
            for dtr in (True, False):
                result = _try_once(port, baud, stop, dtr)
                attempts.append(result)
                if result.get("ok"):
                    rec = {
                        "baud": baud,
                        "stopbits": stop,
                        "dtr_rts": dtr,
                    }
                    out = {
                        "ok": True,
                        "port": port,
                        "recommended": rec,
                        "match": result,
                        "attempts": len(attempts),
                        "env_exports": (
                            f"export K2_SERIAL_BAUD={baud}; "
                            f"export K2_SERIAL_STOPBITS={stop}; "
                            f"export K2_SERIAL_DTR={1 if dtr else 0}"
                        ),
                    }
                    return out

    return {
        "ok": False,
        "port": port,
        "message": "No valid K2 response on any tried setting",
        "last_attempts": attempts[-6:],
    }


def main() -> int:
    p = argparse.ArgumentParser(description="Scan serial settings for KIO2")
    p.add_argument("port", default="/dev/ttyUSB0", nargs="?")
    args = p.parse_args()
    print(json.dumps(scan(args.port), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
