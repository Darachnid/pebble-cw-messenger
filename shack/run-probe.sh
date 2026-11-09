#!/usr/bin/env bash
# Read-only KIO2 probe. Uses project venv (system python3 has no pyserial).
set -euo pipefail
cd "$(dirname "$0")"
if [[ -f k2-serial.env ]]; then
  set -a
  # shellcheck disable=SC1091
  source k2-serial.env
  set +a
fi
PORT="${1:-/dev/ttyUSB0}"
if [[ ! -x .venv/bin/python ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -q -r requirements.txt
fi
exec .venv/bin/python -m k2_telemetry.probe "$PORT"
