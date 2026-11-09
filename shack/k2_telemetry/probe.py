#!/usr/bin/env python3
"""Read-only KIO2 smoke test (Rev E). Usage: probe.py /dev/ttyUSB0"""

from __future__ import annotations

import argparse
import json
import sys
import time

from .config import open_serial
from .poll import poll_once, poll_radio
from .serial_io import K2SerialError


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Read-only KIO2 probe (Rev E)")
    p.add_argument("port", help="Serial device e.g. /dev/ttyUSB0")
    p.add_argument("-v", "--verbose", action="store_true", help="Include raw serial lines")
    p.add_argument(
        "--listen",
        action="store_true",
        help="Open port, wait, send PS;, print all lines (link debug)",
    )
    args = p.parse_args(argv)

    if args.listen:
        k2 = open_serial(args.port)
        try:
            k2.warmup()
            time.sleep(0.5)
            _, lines = k2.query_retry("PS;", lambda ln: ln.upper().startswith("PS1"))
            print(json.dumps({"port": args.port, "lines": lines}, indent=2))
        finally:
            k2.close()
        return 0

    try:
        result = poll_once(args.port, verbose=args.verbose)
    except K2SerialError as exc:
        print(json.dumps({"ok": False, "error": str(exc)}), file=sys.stderr)
        return 1

    print(json.dumps(result, indent=2))
    return 0 if result.get("ok") else 2


if __name__ == "__main__":
    raise SystemExit(main())
