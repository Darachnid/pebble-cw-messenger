"""Map KIO2 poll dict to Pebble dashboard-shaped JSON."""

from __future__ import annotations

import time
from typing import Any


def _audio_filter_code(label: str | None) -> str:
    if not label or label == "—":
        return "OFF"
    return label


def to_dashboard_payload(poll: dict, *, updated_at: float | None = None) -> dict[str, Any]:
    ts = updated_at if updated_at is not None else time.time()
    connected = bool(poll.get("ok"))
    transmitting = bool(poll.get("transmitting"))

    power_label = None
    if transmitting and poll.get("power_segments") is not None:
        power_label = f"{poll.get('power_segments', 0)} seg"

    return {
        "connected": connected,
        "updated_at": ts,
        "connection": "LIVE" if connected else "OFFLINE",
        "frequency_text": poll.get("frequency_text"),
        "mode_text": poll.get("mode"),
        "band_text": poll.get("band"),
        "crystal_bw_hz": poll.get("crystal_bw_hz"),
        "audio_filter": _audio_filter_code(poll.get("audio_filter")),
        "s_meter_segments": poll.get("s_meter_segments"),
        "s_meter_label": f"S{poll['s_meter_segments']}"
        if poll.get("s_meter_segments") is not None
        else None,
        "power_segments": poll.get("power_segments") if transmitting else None,
        "power_label": power_label,
        "rit_enabled": poll.get("rit_on", poll.get("rit_enabled_cat")),
        "rit_offset_hz": poll.get("rit_offset_hz", 0),
        "agc": poll.get("agc"),
        "noise_blanker": poll.get("noise_blanker"),
        "transmitting": transmitting,
        "errors": poll.get("errors", []),
    }
