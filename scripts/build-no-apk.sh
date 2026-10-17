#!/usr/bin/env bash
# Build the watchapp and declare preview "Notifications" permission in the PBW.
# No separate Android APK, external helper, or remote build service required.
set -euo pipefail
cd "$(dirname "$0")/.."
pebble package install @rebble/clay
# Pebble SDK version parser requires N.N.N with no prerelease suffixes.
python3 - <<'PY'
import json, re
from pathlib import Path
version = json.loads(Path("package.json").read_text())["version"]
if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
    raise SystemExit(f"Invalid Pebble app version {version!r}: expected N.N.N")
print("Pebble app version:", version)
PY
pebble clean
pebble build
python3 - <<'PY'
import json, os, tempfile, zipfile
from pathlib import Path
root = Path.cwd()
package = json.loads((root / 'package.json').read_text())
file = root / 'build' / (package['name'] + '.pbw')
with zipfile.ZipFile(file, 'r') as src:
    entries = [(item, src.read(item.filename)) for item in src.infolist()]
contents = dict((item.filename, data) for item, data in entries)
info = json.loads(contents['appinfo.json'])
info['manifestVersion'] = 2
info['usesPermissions'] = package['pebble'].get('usesPermissions', [])
assert 'Notifications' in info['usesPermissions'], 'Notifications permission missing'
fd, temp = tempfile.mkstemp(prefix='cw-permissions-', suffix='.pbw', dir=file.parent)
os.close(fd)
try:
    with zipfile.ZipFile(temp, 'w') as dest:
        for item, data in entries:
            if item.filename == 'appinfo.json':
                data = json.dumps(info, indent=2).encode('utf-8')
            dest.writestr(item, data)
    with zipfile.ZipFile(temp) as verified:
        confirm = json.loads(verified.read('appinfo.json'))
        assert confirm['manifestVersion'] == 2
        assert 'Notifications' in confirm['usesPermissions']
    os.replace(temp, file)
finally:
    if os.path.exists(temp): os.unlink(temp)
print('Ready:', file)
print('Permission:', info['usesPermissions'], 'manifest v', info['manifestVersion'])
PY
