#!/usr/bin/env python3
"""Shack-side read-only K2 telemetry HTTP service."""

from __future__ import annotations

import argparse
import json
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

# matthew already runs bedroom Kinect gestures on 8765 (WebSocket).
DEFAULT_HTTP_PORT = 8772

from .config import load_serial_env_file, open_serial
from .dashboard_json import to_dashboard_payload
from .poll import poll_radio
from .serial_io import K2SerialError


class _State:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.payload: dict[str, Any] = {
            "connected": False,
            "connection": "OFFLINE",
            "errors": ["starting"],
        }
        self.last_ok_at: float = 0.0

    def set(self, poll: dict) -> None:
        with self.lock:
            dash = to_dashboard_payload(poll)
            if poll.get("ok"):
                self.last_ok_at = time.time()
            elif self.last_ok_at and (time.time() - self.last_ok_at) > 5.0:
                dash["connection"] = "STALE"
            self.payload = dash

    def get(self) -> dict[str, Any]:
        with self.lock:
            return dict(self.payload)


STATE = _State()


def _poller(port: str, interval_s: float) -> None:
    """Keep polling; reopen serial after errors (open/warmup used to kill the thread)."""
    backoff_s = interval_s
    while True:
        k2 = None
        try:
            k2 = open_serial(port)
            k2.warmup()
            backoff_s = interval_s
            while True:
                try:
                    poll = poll_radio(k2)
                    poll.setdefault("errors", [])
                    STATE.set(poll)
                except K2SerialError as exc:
                    STATE.set({"ok": False, "errors": [str(exc)]})
                    break
                time.sleep(interval_s)
        except K2SerialError as exc:
            STATE.set({"ok": False, "errors": [str(exc)]})
        except Exception as exc:  # noqa: BLE001 — keep poller alive; log for shack debugging
            import traceback

            traceback.print_exc()
            STATE.set({"ok": False, "errors": [f"poller: {exc!r}"]})
        finally:
            if k2 is not None:
                try:
                    k2.close()
                except Exception:
                    pass
        time.sleep(backoff_s)
        backoff_s = min(backoff_s * 1.5, 15.0)


class _Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt: str, *args: object) -> None:
        return

    def do_GET(self) -> None:
        if self.path in ("/api/v1/state", "/api/v1/state/"):
            body = json.dumps(STATE.get(), indent=2).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.path in ("/health", "/health/"):
            body = b'{"status":"ok"}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        self.send_response(404)
        self.end_headers()


def main(argv: list[str] | None = None) -> int:
    load_serial_env_file()
    p = argparse.ArgumentParser(description="K2 read-only telemetry daemon")
    p.add_argument("--port", default="/dev/ttyUSB0", help="Serial device")
    p.add_argument("--bind", default="0.0.0.0", help="HTTP bind address")
    p.add_argument(
        "--http-port",
        type=int,
        default=int(os.environ.get("K2_HTTP_PORT", str(DEFAULT_HTTP_PORT))),
        help="HTTP port (env K2_HTTP_PORT; avoid 8765 on matthew)",
    )
    p.add_argument("--interval", type=float, default=1.5, help="Poll interval seconds")
    args = p.parse_args(argv)

    t = threading.Thread(target=_poller, args=(args.port, args.interval), daemon=True)
    t.start()

    server = ThreadingHTTPServer((args.bind, args.http_port), _Handler)
    print(f"K2 telemetry on http://{args.bind}:{args.http_port}/api/v1/state", flush=True)
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
