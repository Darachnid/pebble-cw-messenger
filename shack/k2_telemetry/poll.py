"""Read-only KIO2 poll cycle (Rev E)."""

from __future__ import annotations

from . import protocol
from .config import open_serial
from .serial_io import K2Serial


def _extended_fw_get(k2: K2Serial) -> tuple[str | None, list[str]]:
    raw: list[str] = []
    for cmd in ("K22;", "FW;", "K20;"):
        _, lines = k2.query_retry(cmd, lambda ln: protocol.parse_fw(ln) is not None, attempts=2)
        raw.extend(lines)
    for line in raw:
        if protocol.parse_fw(line):
            return line, raw
    return None, raw


def _apply_if(out: dict, info: protocol.IfInfo) -> None:
    out["frequency_hz"] = info.freq_hz
    out["frequency_text"] = protocol.format_k2_frequency(info.freq_hz)
    out["mode"] = protocol.MODE_NAMES.get(info.mode_code, f"M{info.mode_code}")
    out["band"] = protocol.derive_band_label(info.freq_hz)
    out["rit_on"] = info.rit_on
    out["rit_offset_hz"] = info.rit_offset_hz
    out["transmitting"] = info.tx


def poll_radio(k2: K2Serial, *, verbose: bool = False) -> dict:
    out: dict = {"ok": False, "errors": []}
    raw: dict = {} if verbose else {}
    if_seen: list[str] = []

    tq_line, tq_seen = k2.query_retry("TQ;", lambda ln: protocol.parse_tq(ln) is not None)
    if not tq_line:
        out["errors"].append("no TQ response")
        return out
    out["transmitting"] = protocol.parse_tq(tq_line)

    if_line, if_seen = k2.query_retry("IF;", lambda ln: protocol.parse_if(ln) is not None)
    if verbose:
        raw["if"] = if_seen
    if if_line:
        info = protocol.parse_if(if_line)
        if info:
            _apply_if(out, info)

    if "frequency_hz" not in out:
        for line in if_seen:
            hz = protocol.parse_if_frequency_loose(line)
            if hz is not None:
                out["frequency_hz"] = hz
                out["frequency_text"] = protocol.format_k2_frequency(hz)
                out["band"] = protocol.derive_band_label(hz)
                break

    fa_line, fa_seen = k2.query_retry("FA;", lambda ln: protocol.parse_fa(ln) is not None)
    if verbose:
        raw["fa"] = fa_seen
    if fa_line:
        fa_hz = protocol.parse_fa(fa_line)
        if fa_hz is not None:
            if "frequency_hz" not in out:
                out["frequency_hz"] = fa_hz
                out["frequency_text"] = protocol.format_k2_frequency(fa_hz)
                out["band"] = protocol.derive_band_label(fa_hz)
            else:
                out["fa_confirm_hz"] = fa_hz

    if "mode" not in out:
        md_line, _ = k2.query_retry("MD;", lambda ln: protocol.parse_md(ln) is not None)
        if md_line:
            md = protocol.parse_md(md_line)
            if md is not None:
                out["mode"] = protocol.MODE_NAMES.get(md, f"M{md}")

    bg_line, _ = k2.query_retry("BG;", lambda ln: protocol.parse_bg(ln) is not None)
    if bg_line:
        bg = protocol.parse_bg(bg_line)
        if bg is not None:
            out["bargraph"] = bg
            out["s_meter_segments"] = protocol.bg_to_segments(bg)
            if out.get("transmitting"):
                out["power_segments"] = protocol.bg_to_segments(bg)

    fw_line, fw_raw = _extended_fw_get(k2)
    if verbose:
        raw["fw"] = fw_raw
    if fw_line:
        fw = protocol.parse_fw(fw_line)
        if fw:
            out["crystal_bw_hz"] = fw.bandwidth_hz
            out["audio_filter"] = protocol.audio_filter_from_fw(fw)

    gt_line, _ = k2.query_retry("GT;", lambda ln: protocol.parse_gt(ln) is not None)
    if gt_line:
        gt = protocol.parse_gt(gt_line)
        if gt:
            out["agc"] = protocol.gt_label(gt)

    nb_line, _ = k2.query_retry("NB;", lambda ln: protocol.parse_nb(ln) is not None)
    if nb_line:
        nb = protocol.parse_nb(nb_line)
        if nb:
            out["noise_blanker"] = protocol.nb_label(nb)

    rt_line, _ = k2.query_retry("RT;", lambda ln: protocol.parse_rt(ln) is not None)
    if rt_line:
        rit = protocol.parse_rt(rt_line)
        if rit is not None:
            out["rit_enabled_cat"] = rit
            out["rit_on"] = rit

    if verbose:
        out["raw"] = raw

    out["ok"] = "frequency_hz" in out
    return out


def poll_once(port: str, *, verbose: bool = False) -> dict:
    k2 = open_serial(port)
    try:
        k2.warmup()
        result = poll_radio(k2, verbose=verbose)
        result["port"] = port
        return result
    finally:
        k2.close()
