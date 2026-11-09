#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
PORT="${1:-/dev/ttyUSB0}"
if [[ ! -x .venv/bin/python ]]; then
  python3 -m venv .venv
  .venv/bin/pip install -q -r requirements.txt
fi
.venv/bin/python -m k2_telemetry.link_scan "$PORT" | tee /tmp/k2-scan.json
if .venv/bin/python -c "import json,sys; d=json.load(open('/tmp/k2-scan.json')); sys.exit(0 if d.get('ok') else 1)"; then
  .venv/bin/python -c "
import json
d=json.load(open('/tmp/k2-scan.json'))
r=d['recommended']
open('k2-serial.env','w').write(
  '# Written by scan-link.sh\n'
  f\"K2_SERIAL_BAUD={r['baud']}\n\"
  f\"K2_SERIAL_STOPBITS={r['stopbits']}\n\"
  f\"K2_SERIAL_DTR={1 if r['dtr_rts'] else 0}\n\"
)
print('Saved k2-serial.env')
"
fi
