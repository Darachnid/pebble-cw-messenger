#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [[ -f k2-serial.env ]]; then
  set -a
  # shellcheck disable=SC1091
  source k2-serial.env
  set +a
fi
SERIAL="${1:-/dev/ttyUSB0}"
if [[ ! -x .venv/bin/python ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -q -r requirements.txt
fi
HTTP_PORT="${K2_HTTP_PORT:-8772}"
exec .venv/bin/python -m k2_telemetry.daemon \
  --port "$SERIAL" \
  --bind 0.0.0.0 \
  --http-port "$HTTP_PORT"
