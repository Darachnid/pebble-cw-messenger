"""Single-threaded KIO2 serial access (4800 8N2 per KIO2 AUX manual)."""

from __future__ import annotations

import re
import time
from typing import Callable, List, Optional

from .safety import validate_outbound

try:
    import serial
except ImportError:
    serial = None  # type: ignore

DEFAULT_BAUD = 4800
DEFAULT_STOPBITS = 2
IDLE_GAP_S = 0.05
TOTAL_TIMEOUT_S = 0.65
POST_WRITE_DELAY_S = 0.06

_PRINTABLE = re.compile(r"^[\x20-\x7e]+$")


class K2SerialError(Exception):
    pass


class K2Serial:
    def __init__(
        self,
        port: str,
        baud: int = DEFAULT_BAUD,
        stopbits: int = DEFAULT_STOPBITS,
        timeout_s: float = 0.08,
        dtr_rts: bool = True,
    ):
        if serial is None:
            raise K2SerialError("pyserial not installed (pip install pyserial)")
        try:
            sb = serial.STOPBITS_TWO if stopbits == 2 else serial.STOPBITS_ONE
            self._ser = serial.Serial(
                port=port,
                baudrate=baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=sb,
                timeout=timeout_s,
                rtscts=False,
                dsrdtr=False,
            )
        except serial.SerialException as exc:
            if "Permission denied" in str(exc) or getattr(exc, "errno", None) == 13:
                raise K2SerialError(
                    f"cannot open {port}: permission denied. "
                    "Add your user to group uucp (sudo usermod -aG uucp $USER), "
                    "log out/in, or use the stable by-id path under /dev/serial/by-id/."
                ) from exc
            raise K2SerialError(str(exc)) from exc
        self._ser.dtr = dtr_rts
        self._ser.rts = dtr_rts
        time.sleep(0.2)
        self._ser.reset_input_buffer()
        self._ser.reset_output_buffer()

    def close(self) -> None:
        self._ser.close()

    def warmup(self) -> None:
        """No RF impact — AI0 disables auto-info spam (Rev E meta-command)."""
        self.transact("AI0;")
        time.sleep(0.05)

    @staticmethod
    def _line_clean(line: str) -> bool:
        return bool(line) and _PRINTABLE.match(line) is not None

    def transact(self, command: str) -> List[str]:
        ok, reason = validate_outbound(command)
        if not ok:
            raise K2SerialError(f"blocked outbound command ({reason.name}): {command!r}")

        self._ser.reset_input_buffer()
        self._ser.write(command.encode("ascii"))
        self._ser.flush()
        time.sleep(POST_WRITE_DELAY_S)

        deadline = time.monotonic() + TOTAL_TIMEOUT_S
        buf = bytearray()
        lines: List[str] = []
        last_data = 0.0

        while time.monotonic() < deadline:
            chunk = self._ser.read(512)
            now = time.monotonic()
            if chunk:
                last_data = now
                buf.extend(chunk)
                while b";" in buf:
                    idx = buf.index(b";")
                    raw = bytes(buf[: idx + 1])
                    del buf[: idx + 1]
                    line = raw.decode("latin-1").strip()
                    if line:
                        lines.append(line)
            elif buf and last_data and (now - last_data) >= IDLE_GAP_S:
                break
            else:
                time.sleep(0.005)

        return lines

    def query_all(self, command: str) -> List[str]:
        return self.transact(command)

    def query_first(self, command: str, prefix: str) -> Optional[str]:
        cmd_norm = command.strip().upper()
        best: Optional[str] = None
        for line in self.transact(command):
            if not self._line_clean(line):
                continue
            up = line.upper()
            if up == cmd_norm:
                continue
            if not up.startswith(prefix.upper()):
                continue
            if len(line) > len(best or ""):
                best = line
        return best

    def query_retry(
        self,
        command: str,
        accept: Callable[[str], bool],
        attempts: int = 4,
    ) -> tuple[Optional[str], list[str]]:
        seen: list[str] = []
        for _ in range(attempts):
            for line in self.transact(command):
                seen.append(line)
                if not self._line_clean(line):
                    continue
                if accept(line):
                    return line, seen
            time.sleep(0.08)
        return None, seen
