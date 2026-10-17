#!/usr/bin/env python3
"""Preflight: native CW Messenger, Android key contracts and PBW packaging."""
import json
import re
import sys
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parent.parent
pkg = json.loads((root / "package.json").read_text())
pebble = pkg["pebble"]
keys = pebble["messageKeys"]
android = (root / "android/app/src/main/java/dev/darachnid/cwmessenger/CwTransport.kt").read_text()
expected = {
    "KEY_SENDER": "SmsSender",
    "KEY_BODY": "SmsBody",
    "KEY_DRAFT": "TxDraft",
    "KEY_REVIEW": "DraftReview",
    "KEY_CONFIRM": "DraftConfirm",
    "KEY_STATUS": "DraftStatus",
    "KEY_WPM": "Wpm",
    "KEY_TONE": "ToneHz",
    "KEY_VOLUME": "Volume",
    "KEY_RX_AUDIO": "RxAudio",
    "KEY_RX_VIBE": "RxVibe",
    "KEY_TX_AUDIO": "TxAudio",
    "KEY_TX_VIBE": "TxVibe",
    "KEY_REVERSE": "ReversePaddles",
    "KEY_THRESHOLD": "StraightThreshold",
    "KEY_SYNC": "SyncMs",
    "KEY_START_MODE": "StartMode",
    "KEY_AUTOPLAY": "AutoPlay",
    "KEY_DEMO_SENDER": "DemoSender",
    "KEY_DEMO_MESSAGE": "DemoMessage",
}
errors = []
if not re.fullmatch(r"\d+\.\d+\.\d+", pkg["version"]):
    errors.append("Pebble version must be strictly N.N.N")
if (root / "src/pkjs/index.js").exists():
    errors.append("PebbleKit JS is present and would intercept native companion messages")
for android_name, pebble_name in expected.items():
    if pebble_name not in keys:
        errors.append("Missing Pebble key " + pebble_name)
        continue
    numeric = 10000 + keys.index(pebble_name)
    found = re.search(r"const val " + re.escape(android_name) + r": UInt = (\d+)u", android)
    if not found or int(found.group(1)) != numeric:
        errors.append("Key mismatch " + android_name + " should be " + str(numeric))
app = pebble.get("companionApp", {}).get("android", {})
packages = [x.get("package") for x in app.get("apps", [])]
if "dev.darachnid.cwmessenger" not in packages:
    errors.append("Native companion package is not registered in package.json")

pbw = root / "build/pebble-cw-messenger.pbw"
if pbw.exists():
    with zipfile.ZipFile(pbw) as z:
        names = z.namelist()
        if any(x.endswith("pebble-js-app.js") for x in names):
            errors.append("PBW contains PebbleKit JS: clean and rebuild before installing")
        info = json.loads(z.read("appinfo.json"))
        if info.get("uuid", "").lower() != pebble["uuid"].lower():
            errors.append("Built PBW uses unexpected watchapp UUID")
        print("PBW files:", ", ".join(names))
else:
    print("No PBW yet; run pebble clean && pebble build to validate packaging")

if errors:
    for error in errors:
        print("ERROR:", error, file=sys.stderr)
    sys.exit(1)
print("PASS: native Android bridge, watch UUID, and", len(expected), "message keys match")
