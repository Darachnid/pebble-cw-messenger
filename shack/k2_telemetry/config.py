"""Load K2 serial settings from environment and k2-serial.env."""

from __future__ import annotations

import os
from pathlib import Path

from .serial_io import K2Serial


def load_serial_env_file() -> None:
    env_path = Path(__file__).resolve().parents[1] / "k2-serial.env"
    if not env_path.is_file():
        return
    for line in env_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, val = line.partition("=")
        os.environ.setdefault(key.strip(), val.strip())


def open_serial(port: str) -> K2Serial:
    load_serial_env_file()
    baud = int(os.environ.get("K2_SERIAL_BAUD", "4800"))
    stopbits = int(os.environ.get("K2_SERIAL_STOPBITS", "2"))
    dtr = os.environ.get("K2_SERIAL_DTR", "1") not in ("0", "false", "False")
    return K2Serial(port, baud=baud, stopbits=stopbits, dtr_rts=dtr)
