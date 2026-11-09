#!/usr/bin/env bash
# Emulator screenshots for README / store (rectangular + round chalk).
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

mkdir -p docs

echo "Building CW Messenger (native, no PebbleKit JS)…"
pebble clean
pebble build

capture_receive() {
  local platform="$1"
  local outfile="$2"
  echo "→ $platform → $outfile"
  export PEBBLE_EMULATOR="$platform"
  pebble install --emulator "$platform"
  # Launcher: SELECT, then SELECT again on CW Messenger (first sideloaded app).
  pebble emu-button --emulator "$platform" click select
  sleep 0.6
  pebble emu-button --emulator "$platform" click select
  sleep 0.8
  pebble screenshot --emulator "$platform" --no-open "$outfile"
}

capture_receive emery  "docs/cw-receive-emery.png"
capture_receive basalt "docs/cw-receive-basalt.png"
capture_receive chalk  "docs/cw-receive-chalk.png"
capture_receive gabbro "docs/cw-receive-gabbro.png"

echo "Send screen (emery): hold DOWN in the emulator after install, then re-run:"
echo "  pebble screenshot --emulator emery --no-open docs/cw-send-emery.png"
