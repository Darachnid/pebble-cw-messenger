# CW Messenger — Pebble Time 2

## v0.6 Preview: incoming SMS notifications without a separate Android APK

**This branch uses the existing Pebble Android app**, its experimental built-in
`notifications/notification` source, and PebbleKit JS to forward new permitted
notifications to your CW Messenger watchapp. It does **not** use the experimental
`android/` companion project from the older SMS branch.

The receive display shows the sender and decoded text while retaining the
working 20 WPM/600 Hz square-wave and synchronized vibration paths. Sender,
message, WPM and Hz appear as before. You can change timing and mode settings
using the existing Clay settings page in the Pebble Android app.

### Requirements on Android

- Pebble Android app **1.15 or newer** (experimental plugin permissions).
- Pebble app **Settings → Debug → Show debug options → Use experimental plugins**.
- Notification access must already be granted to the Pebble app.
- When asked for CW Messenger's **Notifications** permission, approve it.
- Keep CW Messenger **open on the watch**: PebbleKit JS and this subscription
  stop when the watchapp exits. This is NOT a background SMS listener.

### Configure the source in the Pebble app

1. Open the Pebble phone app, find **CW Messenger → Settings**.
2. In **Incoming SMS via Pebble (Preview)**, enable
   **Read new message notifications as CW**.
3. In **Allowed SMS app names**, enter the exact Android notification app
   name (default: **Messages**). More than one app name can be entered as a
   comma-separated list, e.g. `Messages, Google Messages`.
4. Save. Open CW Messenger on the watch.
5. Choose **Receive** as the initial screen in Settings, or press **Back**
   from Send. Send a new test SMS to your phone.

The first notification-shade snapshot is deliberately ignored, so CW
Messenger does not unexpectedly read old messages on startup. Subsequent
new/updated messages from a matching app name are displayed, capped at
120 ASCII Morse-compatible characters. This uses the notification title/body,
not the Android SMS database. A message hidden by Android's privacy controls
may not have readable content. The plugin can see the whole notification
shade, but CW Messenger only forwards matching app names to the watch.

### Build/install on manjo (no separate APK)

```bash
cd ~/pebble-cw-messenger
git stash push -m "Save local package edits" -- package.json
git fetch origin
git switch -c feature/no-apk-notification-cw --track origin/feature/no-apk-notification-cw
bash scripts/build-no-apk.sh && pebble install --cloudpebble
```

If you already have the branch locally, use
`git switch feature/no-apk-notification-cw` and
`git pull --ff-only origin feature/no-apk-notification-cw` rather than
creating it again.

The script installs Clay, cleans/rebuilds the PBW, and sets PBW
`manifestVersion=2` with the `Notifications` permission declared in
`package.json`. It creates `build/pebble-cw-messenger.pbw`. It does
not build or install anything on Android. The Pebble Android app
still needs to be paired with the watch to install the PBW.

To inspect the permission metadata:

```bash
python3 - <<'PY'
import json, zipfile
with zipfile.ZipFile("build/pebble-cw-messenger.pbw") as z:
    x = json.loads(z.read("appinfo.json"))
    print(x.get("manifestVersion"), x.get("usesPermissions"))
PY
```

Expected: `2 ['Notifications']`.

### What works in this preview

- Existing Send paddles, straight key and local decoding (unchanged).
- Existing Receive PCM square tone and precomputed vibration pattern (unchanged).
- Pebble app settings for speed, pitch, volume, mode, audio, haptics and
  demo message.
- **New:** opt-in forwarding of newly observed Android notification
  titles/bodies from user-approved app labels to the Receive screen.
- **New:** if a new message arrives while sending, defer Receive playback
  until the user returns from the CW keyer. Do not disturb an in-progress draft.

### What this does NOT do

- **No automatic receiving while CW Messenger is closed.** The PebbleKit JS
  subscription only exists during this watchapp session.
- **No outbound SMS sending.** The notifications source is read-only.
- **No access to SMS history or dismissed/hidden messages.**
- No dedicated Android APK; no server; no internet transfer of SMS content.

This depends on **Pebble mobile app experimental plugins** and needs a real
Time 2 test. An incompatible or disabled preview API leaves Send, Receive
and Clay Settings usable, but live notification reading unavailable.

References:
- https://developer.repebble.com/guides/plugins/preview/
- https://developer.repebble.com/docs/pebblekit-js/Pebble/
