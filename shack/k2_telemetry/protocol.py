"""Parse KIO2 Rev E response lines (uppercase, semicolon-terminated)."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Optional

# IF00014060000     +0123r1x0t0m3v0s0p0b0; (Rev E; firmware may add trailing fields)
_IF_RE = re.compile(
    r"^IF(\d{11})\s*([+-])(\d{4})r([0-9])x([0-9]).*?t([0-9])m([0-9])v([0-9])s([0-9])p([0-9])b([0-9]+)",
    re.IGNORECASE,
)

_FA_RE = re.compile(r"^FA(\d{11});?$", re.IGNORECASE)
_MD_RE = re.compile(r"^MD(\d);?$", re.IGNORECASE)

_BG_RE = re.compile(r"^BG(\d{2});?$", re.IGNORECASE)
_FW_EXT_RE = re.compile(r"^FW(\d{4})(\d)([0-2]);?$", re.IGNORECASE)
_FW_BASIC_RE = re.compile(r"^FW(\d{4});?$", re.IGNORECASE)
_GT_EXT_RE = re.compile(r"^GT(\d{3})([01]);?$", re.IGNORECASE)
_GT_BASIC_RE = re.compile(r"^GT(\d{3});?$", re.IGNORECASE)
_NB_EXT_RE = re.compile(r"^NB(\d)([01]);?$", re.IGNORECASE)
_NB_BASIC_RE = re.compile(r"^NB(\d);?$", re.IGNORECASE)
_RT_RE = re.compile(r"^RT([01]);?$", re.IGNORECASE)
_TQ_RE = re.compile(r"^TQ([01]);?$", re.IGNORECASE)
_SM_RE = re.compile(r"^SM(\d{4});?$", re.IGNORECASE)

MODE_NAMES = {
    1: "LSB",
    2: "USB",
    3: "CW",
    6: "RTTY",
    7: "CW-R",
    9: "RTTY-R",
}


@dataclass
class IfInfo:
    freq_hz: int
    rit_sign: int
    rit_offset_hz: int
    rit_on: bool
    xit_on: bool
    tx: bool
    mode_code: int
    rx_vfo_b: bool


def _plausible_vfo_hz(hz: int) -> bool:
    mhz = hz / 1_000_000.0
    return 1.799 <= mhz <= 54.5


def _vfo_hz_from_noisy_digits(digits: str) -> Optional[int]:
    """Find VFO Hz in a corrupted digit stream (K2 sends frequency first in IF/FA)."""
    if len(digits) < 7:
        return None
    for length in range(min(11, len(digits)), 6, -1):
        hz = int(digits[:length])
        if _plausible_vfo_hz(hz):
            return hz
    for length in range(min(11, len(digits)), 6, -1):
        for i in range(1, len(digits) - length + 1):
            hz = int(digits[i : i + length])
            if _plausible_vfo_hz(hz):
                return hz
    return None


def parse_if_frequency_loose(line: str) -> Optional[int]:
    pos = line.upper().find("IF")
    if pos < 0:
        return None
    digits = "".join(c for c in line[pos + 2 :] if c.isdigit())
    return _vfo_hz_from_noisy_digits(digits)


def parse_fa_frequency_loose(line: str) -> Optional[int]:
    pos = line.upper().find("FA")
    if pos < 0:
        return None
    digits = "".join(c for c in line[pos + 2 :] if c.isdigit())
    hz = _vfo_hz_from_noisy_digits(digits)
    if hz is not None:
        return hz
    digits11 = _cat_digits(line.strip(), "FA", 11)
    return int(digits11) if digits11 else None


def _cat_digits(line: str, prefix: str, count: int) -> Optional[str]:
    """Pull digit run after prefix when framing glitches hide a strict match."""
    upper = line.upper()
    if not upper.startswith(prefix):
        return None
    tail = upper[len(prefix) :]
    digits = "".join(c for c in tail if c.isdigit())
    if len(digits) >= count:
        return digits[:count]
    return None


def parse_fa(line: str) -> Optional[int]:
    m = _FA_RE.match(line.strip())
    if m:
        return int(m.group(1))
    return parse_fa_frequency_loose(line)


def parse_md(line: str) -> Optional[int]:
    m = _MD_RE.match(line.strip())
    return int(m.group(1)) if m else None


def parse_if(line: str) -> Optional[IfInfo]:
    stripped = line.strip()
    m = _IF_RE.match(stripped)
    if not m:
        clean = "".join(c for c in stripped.upper() if c.isprintable() and ord(c) < 128)
        m = _IF_RE.match(clean) or re.search(
            r"IF(\d{11})\s*([+-])(\d{4})R(\d)X(\d).*?T(\d)M(\d)V(\d)S(\d)P(\d)B(\d+)",
            clean,
        )
    if not m:
        return None
    freq_hz = int(m.group(1))
    sign = 1 if m.group(2) == "+" else -1
    rit_hz = sign * int(m.group(3))
    return IfInfo(
        freq_hz=freq_hz,
        rit_sign=sign,
        rit_offset_hz=rit_hz,
        rit_on=m.group(4) == "1",
        xit_on=m.group(5) == "1",
        tx=m.group(6) == "1",
        mode_code=int(m.group(7)),
        rx_vfo_b=m.group(8) == "1",
    )


def format_k2_frequency(freq_hz: int) -> str:
    """K2-style display: 14.060.00 (10 Hz steps under CAT per Rev E)."""
    mhz = freq_hz // 1_000_000
    khz_milli = freq_hz % 1_000_000
    khz = khz_milli // 1000
    centi_khz = (khz_milli % 1000) // 10
    return f"{mhz}.{khz:03d}.{centi_khz:02d}"


def derive_band_label(freq_hz: int) -> str:
    """Amateur band label from center frequency (approximate)."""
    mhz = freq_hz / 1_000_000.0
    bands = (
        (1.8, "160 m"),
        (3.5, "80 m"),
        (7.0, "40 m"),
        (10.1, "30 m"),
        (14.0, "20 m"),
        (18.1, "17 m"),
        (21.0, "15 m"),
        (24.9, "12 m"),
        (28.0, "10 m"),
        (50.0, "6 m"),
    )
    best = "—"
    best_delta = 1e9
    for edge, name in bands:
        delta = abs(mhz - edge)
        if delta < best_delta:
            best_delta = delta
            best = name
    if best_delta > 2.0:
        return "—"
    return best


def bg_to_segments(bg: int) -> int:
    """Map BG response to 0–10 dashboard segments (Rev E BAR mode 12–22)."""
    if bg >= 12:
        return max(0, min(10, bg - 12))
    return max(0, min(10, bg))


def sm_to_segments(sm: int) -> int:
    """SM 0–15 → rough 0–10 bars (legacy; prefer BG)."""
    return max(0, min(10, round(sm * 10 / 15)))


def parse_bg(line: str) -> Optional[int]:
    m = _BG_RE.match(line.strip())
    if m:
        return int(m.group(1))
    digits = _cat_digits(line.strip(), "BG", 2)
    return int(digits) if digits else None


def parse_sm(line: str) -> Optional[int]:
    m = _SM_RE.match(line.strip())
    return int(m.group(1)) if m else None


@dataclass
class FwInfo:
    bandwidth_hz: int
    filter_number: Optional[int]
    audio_mode: Optional[int]


def parse_fw(line: str) -> Optional[FwInfo]:
    line = line.strip()
    m = _FW_EXT_RE.match(line)
    if m:
        return FwInfo(int(m.group(1)), int(m.group(2)), int(m.group(3)))
    m = _FW_BASIC_RE.match(line)
    if m:
        return FwInfo(int(m.group(1)), None, None)
    digits = _cat_digits(line, "FW", 6)
    if digits and len(digits) >= 5:
        bw = int(digits[:4])
        flt = int(digits[4]) if len(digits) > 4 else None
        af = int(digits[5]) if len(digits) > 5 else None
        return FwInfo(bw, flt, af)
    return None


@dataclass
class GtInfo:
    code: int
    agc_on: Optional[bool]


def parse_gt(line: str) -> Optional[GtInfo]:
    line = line.strip()
    m = _GT_EXT_RE.match(line)
    if m:
        return GtInfo(int(m.group(1)), m.group(2) == "1")
    m = _GT_BASIC_RE.match(line)
    if m:
        return GtInfo(int(m.group(1)), None)
    return None


def gt_label(gt: GtInfo) -> str:
    if gt.agc_on is False:
        return "OFF"
    speed = "FAST" if gt.code == 2 else "SLOW"
    return speed


@dataclass
class NbInfo:
    mode: int
    threshold_hi: Optional[bool]


def parse_nb(line: str) -> Optional[NbInfo]:
    line = line.strip()
    m = _NB_EXT_RE.match(line)
    if m:
        return NbInfo(int(m.group(1)), m.group(2) == "1")
    m = _NB_BASIC_RE.match(line)
    if m:
        mode = 0 if m.group(1) == "0" else 1
        return NbInfo(mode, None)
    return None


def nb_label(nb: NbInfo) -> str:
    if nb.mode == 0:
        return "OFF"
    if nb.mode == 1:
        return "NB1"
    return "NB2"


def parse_rt(line: str) -> Optional[bool]:
    m = _RT_RE.match(line.strip())
    return m.group(1) == "1" if m else None


def parse_tq(line: str) -> Optional[bool]:
    stripped = line.strip()
    m = _TQ_RE.match(stripped)
    if m:
        return m.group(1) == "1"
    if stripped.upper().startswith("TQ") and len(stripped) >= 3:
        d = stripped[2]
        if d in ("0", "1"):
            return d == "1"
    return None


def audio_filter_from_fw(fw: FwInfo) -> str:
    if fw.audio_mode is None:
        return "—"
    return ("OFF", "AF1", "AF2")[fw.audio_mode]
