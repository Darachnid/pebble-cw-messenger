#!/usr/bin/env bash
# Bootstrap and deploy CW Messenger from the Manjaro desktop "manjo".
set -euo pipefail
REPO="https://github.com/Darachnid/pebble-cw-messenger.git"
PROJECT="$HOME/pebble-cw-messenger"

echo "==> Installing Manjaro build dependencies"
sudo pacman -S --needed git uv nodejs npm sdl2 glib2 pixman zlib

export PATH="$HOME/.local/bin:$PATH"
if ! command -v pebble >/dev/null 2>&1; then
  echo "==> Installing Pebble CLI"
  uv tool install --python 3.13 pebble-tool
fi
command -v pebble >/dev/null || { echo "pebble CLI not found" >&2; exit 1; }

echo "==> Installing the current Pebble SDK"
pebble sdk install latest

if [[ -d "$PROJECT/.git" ]]; then
  echo "==> Updating checkout on manjo"
  cd "$PROJECT"
  remote="$(git remote get-url origin)"
  case "$remote" in
    "$REPO"|"git@github.com:Darachnid/pebble-cw-messenger.git"|"https://github.com/Darachnid/pebble-cw-messenger") ;;
    *) echo "Unexpected git remote: $remote; refusing to overwrite." >&2; exit 1;;
  esac
  if [[ -n "$(git status --porcelain)" ]]; then
    echo "Local changes detected. Commit/stash them first." >&2
    exit 1
  fi
  git fetch origin main
  git switch main 2>/dev/null || git switch --track -c main origin/main
  git merge --ff-only origin/main
elif [[ -e "$PROJECT" ]]; then
  echo "$PROJECT already exists but is not a git checkout." >&2
  exit 1
else
  git clone "$REPO" "$PROJECT"
  cd "$PROJECT"
fi

echo "==> Compiling for Pebble Time 2"
pebble build
echo "Build output: $PROJECT/build/pebble-cw-messenger.pbw"

echo "Enable Dev Connection in the Pebble Android app, then log in here:"
pebble login
echo "==> Installing through the Pebble cloud proxy"
pebble install --cloudpebble
echo "==> Done."
