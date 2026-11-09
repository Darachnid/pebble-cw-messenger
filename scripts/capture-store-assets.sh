#!/usr/bin/env bash
# Pebble emulator screenshots + static store icons (regenerate icons first).
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

python3 scripts/generate-brand-assets.py
pebble build

mkdir -p docs/store

capture() {
  local platform="$1"
  local name="$2"
  export PEBBLE_EMULATOR="$platform"
  pebble install --emulator "$platform" >/dev/null
  pebble emu-button --emulator "$platform" click select
  sleep 0.6
  # Skip welcome on repeat installs; open app if on watchface
  pebble emu-button --emulator "$platform" click select
  sleep 0.8
  pebble screenshot --emulator "$platform" --no-open "docs/store/${name}-${platform}.png"
  echo "→ docs/store/${name}-${platform}.png"
}

capture emery cw-receive
capture chalk cw-receive-round

# Send screen: from receive, hold is not easy in script — document manual:
echo "Optional: open Send (HOLD DOWN in emulator), then:"
echo "  pebble screenshot --emulator emery --no-open docs/store/cw-send-emery.png"

echo "Store icons: docs/store/app-icon-512.png, docs/store/pebble-menu-icon-80.png"
