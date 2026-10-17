# CW Messenger — Pebble Time 2

## v0.5 — Pebble phone-app settings (hardware validation pending)

The Pebble phone app's **CW Messenger → Settings** gear is implemented with
[Clay](https://developer.repebble.com/guides/user-interfaces/app-configuration/).
Clay renders the form offline inside the existing Pebble app (no separate website or
Android settings app). Preferences are saved on the watch and applied at the next
receive playback or Send-mode session.

Configurable in the phone app:
- WPM: 5–40 (default 20)
- Square-wave CW pitch: 200–1500 Hz (default 600)
- Speaker volume: 0–100% (default 60%)
- Receive speaker and receive vibration independently
- Send sidetone and send vibration independently
- Swap Up/Down paddle assignments
- Straight-key dah threshold: 125–300% of one dit (default 200%)
- Receive haptic offset: -300 through +400 ms (default +80, retained from v0.4)
- Startup screen: Send (default) or Receive
- Autoplay on opening Receive
- Simulated sender and simulated message for hardware testing

The already-tested Receive square-wave PCM engine and haptic pattern are retained.
Send uses direct keyed square-wave tones, so it does not build an audio queue.
The existing Receive **hold Select** sync-calibration gesture is retained.

**The SMS companion is deliberately not installed in v0.5.** PebbleKit JS
configuration and PebbleKit Android 2 simultaneous routing require hardware
validation; there is no default SMS access or outbound messaging yet.
The work to implement the companion belongs behind a tested integration gate.

## Build/install on `manjo`

```bash
cd ~/pebble-cw-messenger
git pull --ff-only origin main
pebble package install @rebble/clay
pebble build
pebble install --cloudpebble
```

If the SDK reports missing JS dependency, delete **only** stale build artifacts
using `rm -rf build`, then rerun `pebble package install @rebble/clay` and
`pebble build`. Do not delete the source repository.

Settings should appear next to the installed app in the Pebble phone app.
After saving, reopen the watchapp and replay the message; the bottom bar should
reflect your chosen WPM and Hz. In Send mode, hold a paddle to verify its speed
and use Select as the straight key. Test the reverse-paddle switch separately.

## Current mode controls

| Mode | Button | Action |
| --- | --- | --- |
| Send | Up / Down | Dit / Dah (reversible in Settings) |
| Send | Hold Up / Down | Repeated dits/dahs |
| Send | Select | Straight key on button down/up |
| Send | Back | Return to Receive |
| Receive | Select | Replay demo text |
| Receive | Up / Down | Volume (or sync +/-10 ms in calibration) |
| Receive | Hold Select | Toggle sync calibration |
| Receive | Hold Down | Open Send mode |
| Receive | Back | Mute or stop playback |

## SMS architecture and approval gate

The eventual Android companion should use **NotificationListenerService**
with explicit user permission and a narrow allowlist of SMS/messaging apps.
It will parse permitted sender/body notifications and send them to the Pebble
watch via **PebbleKit Android 2**. Direct `READ_SMS` is unnecessary for
incoming notifications. Raw message contents should not be logged.

PebbleKit JS hosts the Clay settings; the Pebble platform's mixed native/JS
AppMessage routing is version-sensitive. We should validate these separately:

1. Phone Settings opens and values persist to the watch.
2. A companion installed on Android can send a synthetic message to the
   active watchapp while Clay/PKJS is present.
3. Notification listener permission works and selected SMS notifications
   reach the watch without accidental duplication or private-content leakage.
4. Incoming audio begins only under the user's configured privacy policy.
5. Sending text from the watch opens a **user-confirmed SMS intent** on the
   phone; never silently send or request SEND_SMS just for convenience.

We should not claim the whole SMS system works until those tests have passed
on the specific Pebble app and Android firmware in use.

## Experimental Android SMS bridge (feature branch only)

**Status:** Code prepared, but not yet SDK/Android-built or watch-tested. This is NOT a final release.

- Android project: android/ (import into Android Studio).
- PebbleKit Android 2: io.rebble.pebblekit2:client:1.3.2.
- NotificationListenerService listens ONLY to notifications from the default SMS app.
- Nothing is forwarded until both Android Notification Access and the helper's explicit opt-in switch are enabled.
- Android does not request READ_SMS, RECEIVE_SMS or SEND_SMS.
- Watch receives SmsSender (key 14) and SmsBody (key 15). Both match package.json.
- Incoming SMS replaces the Receive sender/body and autoplays only when the Receive screen is visible and autoplay is enabled.
- No SMS is transmitted outward, no server is contacted, and message text is not intentionally logged.
- Sender and message are truncated to 31 and 120 chars for Pebble AppMessage.

### Required tests before enabling real SMS

1. On main (v0.5), build and verify the Pebble Settings page saves WPM/Hz and Send/Receive preferences.
2. Build this branch and install the watchapp. In Settings set Startup screen = Receive.
3. Import the android/ project into Android Studio and install the APK.
4. BEFORE granting notification access, press Send a harmless test message in the Android helper.
5. Confirm the watch shows CW Test / test message AND the Clay Settings screen still opens.
6. Only after that succeeds, enable the helper's SMS toggle, grant Notification Access and test an actual incoming SMS.
7. If mixed native + PKJS transport is supported, add a user-confirmed Android SMS composer for outgoing drafts.

**Compatibility warning:** The Pebble docs traditionally say PKJS and native PebbleKit cannot be combined. Some newer builds allow native phone-to-watch messages alongside PKJS, but this route must be tested on your Pebble Android version. Watch-to-phone messages may still be routed exclusively to PKJS.
