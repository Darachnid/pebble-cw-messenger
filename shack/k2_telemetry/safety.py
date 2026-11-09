"""Receive-only KIO2 gate — all bytes to the radio pass through here (Rev E)."""

from __future__ import annotations

import re
from enum import Enum, auto
from typing import Optional

# GET-only two-letter commands allowed for Milestone 2 telemetry.
_ALLOWED_GET = frozenset(
    {
        "AI",
        "AN",
        "BG",
        "DS",
        "FA",
        "FB",
        "FR",
        "FT",
        "FW",
        "GT",
        "ID",
        "IF",
        "K2",
        "KS",
        "KY",
        "LK",
        "MD",
        "NB",
        "PA",
        "PC",
        "PS",
        "RA",
        "RT",
        "SM",
        "SQ",
        "TQ",
        "XT",
    }
)

# SET meta: K2n and AIn do not change RF state (Rev E Meta-commands).
_ALLOWED_SET_META = re.compile(r"^(?:K2[0-3]|AI[0-3]);$")

# Never transmit / key / emulate switches / terminate TX / clear RIT via CAT.
_BLOCKED_PREFIXES = (
    "TX",
    "RX",
    "KY",
    "SW",
    "RC",
    "RD",
    "RU",
    "DN",
    "UP",
)


class DenyReason(Enum):
    NONE = auto()
    EMPTY = auto()
    NOT_ALLOWLISTED = auto()
    TRANSMIT_OR_KEYING = auto()
    SET_NOT_META = auto()


def validate_outbound(command: str) -> tuple[bool, DenyReason]:
    """Return (ok, reason). command must include trailing semicolon."""
    cmd = command.strip()
    if not cmd or not cmd.endswith(";"):
        return False, DenyReason.EMPTY

    upper = cmd.upper()
    for prefix in _BLOCKED_PREFIXES:
        if upper.startswith(prefix):
            return False, DenyReason.TRANSMIT_OR_KEYING

    if _ALLOWED_SET_META.match(upper):
        return True, DenyReason.NONE

    body = upper[:-1]
    if len(body) == 2 and body in _ALLOWED_GET:
        return True, DenyReason.NONE

    return False, DenyReason.NOT_ALLOWLISTED


def build_get(two_letter: str) -> Optional[str]:
    two = two_letter.upper()
    if two not in _ALLOWED_GET:
        return None
    cmd = f"{two};"
    ok, _ = validate_outbound(cmd)
    return cmd if ok else None
