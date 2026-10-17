# CW Messenger — Background SMS + Morse (v0.6.0)

This branch contains the **native Android background SMS** implementation for
Pebble Time 2. Two components are required: a `.pbw` watchapp and a small
Android companion. **No watchapp needs to be left open in the background.**

## Behavior

1. The official Pebble Android app stays connected to Pebble Time 2.
2. Android delivers a new notification from your *default SMS app* to the
   CW SMS Bridge's `NotificationListenerService`, if you have enabled it.
3. The helper first checks whether CW Messenger is already running. If it is,
   it delivers the message without restarting the app or discarding any CW
   draft. If it isn't, it uses PebbleKit Android 2 to open the watchapp.
4. The watchapp starts **in Receive**. The notification's sender and the
   first 120 Morse-compatible characters are displayed and played using the
   saved CW audio/vibration settings.
5. Playback finishes, then the watch returns to the previous screen
   approximately **6 seconds later**. Pressing any control keeps the app open.
6. Manually opening CW Messenger shows the Receive page. Hold Down for 700 ms
   to enter Send and use the established Up/Down/Select keying controls.

No demo text autoplays at launch. If you want the demo, open the app and press
Select in Receive. SMS reception also works when the watchapp is not running.

The CW Messenger entry stays available in the Pebble app launcher for manual
keying. It does **not** run invisibly on the watch or keep a JS process alive.

## Why the Settings location changed

Native PebbleKit Android and PebbleKit JS cannot route messages concurrently
for the same Pebble watchapp. Therefore this branch **removes `src/pkjs/`**
and keeps all adjustable CW controls **in the CW SMS Bridge Android helper**,
not Clay in Pebble's normal watchapp Settings gear. The watch still persists
its already calibrated settings. The phone helper only sends settings the
user has actually changed on the phone.

The helper has controls for WPM (5–40), tone (200–1500 Hz), volume (0–100),
receive audio, receive haptics, send sidetone, send haptics, reverse paddles,
straight-key dah threshold (125–300%), receiver sync offset (-300..400 ms),
and optional manual demo sender/message. All changes are saved in Android
preferences and accompany the next SMS/test transmission. **Incoming SMS
always launches Receive**, and it autoplays regardless of the old demo
autoplay preference.

## Install the watchapp from `manjo`

Save any local tracked changes before changing branches:

```bash
cd ~/pebble-cw-messenger
git stash push -m "Save local CW changes"
git fetch origin
git switch --track origin/feature/android-sms-bridge
# If already created: git switch feature/android-sms-bridge
git pull --ff-only origin feature/android-sms-bridge
pebble clean
pebble build && pebble install --cloudpebble
```

Use exactly **this branch** on your watch. It includes `SmsSender`,
`SmsBody`, and `TxDraft` AppMessage keys (indices 10014, 10015, 10016
with the current message key layout). Check the generated definitions after
building:

```bash
grep -E 'SmsSender|SmsBody|TxDraft' build/include/message_keys.auto.h
```

The branch uses **SDK numeric app version 0.6.0**, not `0.6.0-beta`,
because the SDK rejects prerelease strings.

## Install the Android helper on your phone

On `manjo`, install/open Android Studio and choose **Open** →
`~/pebble-cw-messenger/android`. Let Gradle sync, installing SDK Platform
35 if prompted. Use **Build → Build APK(s)**. The resulting file is:

`android/app/build/outputs/apk/debug/app-debug.apk`

Connect your Android phone with USB debugging enabled, then:

```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

Or use Android Studio's **Run** action to install directly onto the connected
phone. A separate Gradle CLI version 8.11.1 can also build it:

```bash
cd ~/pebble-cw-messenger/android
gradle :app:assembleDebug --no-daemon
```

A GitHub Actions workflow also specifies PBW/APK builds, but its workflow
jobs have not started on a runner in this repository; **do not rely on there
being a prebuilt APK artifact**.

### Configure on Android (one-time)

1. Open **CW SMS Bridge** on the phone.
2. Leave **Enable background incoming SMS** OFF.
3. Tap **Test background launch and Morse**. The watch should open Receive,
   display **CW Test / good night bens best**, play CW, then close 6 seconds
   after audio/vibration finishes. Do this with the watchapp initially closed.
4. Test the audio and haptic settings you wish to change. Changes apply at
   the next incoming message.
5. Enable **Enable background incoming SMS**.
6. Tap **Grant notification access**, then allow **CW SMS Bridge** in Android
   settings. Notification access is sensitive; it allows reading notifications.
   The app filters the phone's default SMS package only.
7. Send a new SMS to your phone from another number while the Pebble app and
   Pebble Time 2 are connected. You no longer need CW Messenger open.

On some Android phones you must allow the helper unrestricted background
operation in Battery settings, so Android does not suspend its notification
listener. Do not grant unrelated permissions.

## Triple-Back Morse reply and local spelling review

**Requires rebuilding BOTH** the watch PBW and the Pixel Android APK after
updating to this branch; older installed components do not know the new keys.

1. Receive a real SMS from the sender you want to reply to. The phone's
   default SMS app must post a notification with an Android **Reply** action.
   The bridge stores only the most recently notified conversation's Reply
   action in memory; it does not infer recipient phone numbers from a name.
2. On the watch, hold **Down** in Receive to open **Send**. Key your reply
   with the Up/Down paddles or Select straight key as usual.
3. **Triple-press Back** (three taps within 400 ms) to prepare the decoded
   Morse text. The watch shows **SPELLCHECK** while the phone processes it.
4. If a local model is configured, the phone makes one Ollama-compatible
   `POST /api/generate` request asking for conservative spelling correction.
   Configure **CW SMS Bridge → Local Ollama URL** and **Ollama model** on
   Android. For a model hosted on `matthew`, use its reachable LAN or
   Tailscale address and the Ollama port, e.g. `http://100.x.y.z:11434`;
   this is an example address, not a known accessible endpoint. The user
   must operate the Ollama server; the helper does not install or run a
   model on the Pixel itself. **No URL/model = no model call**. If the
   model times out, the original text is returned for review.
5. The watch shows **REVIEW REPLY** with the proposed exact text. Press
   **Select** to confirm that text, or **Back** to cancel. There is no
   automatic send or implicit approval of edits.
6. Android uses the *original notification's* available RemoteInput
   `Reply` action to submit that message. It reports **SMS REPLY SENT**
   if the Android action accepts the request, or a failure status if no
   action is available. The transport reports handoff to the Android app,
   not carrier delivery confirmation. The reply target is never guessed.
   Google Messages' original notification must still have a valid Reply
   action. No `SEND_SMS` permission is required.

**Caution:** The optional Ollama server must be one you control and trust.
The HTTP option sends draft text over your own configured network endpoint.
Use HTTPS or an encrypted network such as Tailscale. A host reachable only
from the desktop but not the Pixel will not work. The phone shows the
uncorrected draft instead of inventing a correction when the model is
unavailable. Review the suggestion before choosing Select. This flow has
not yet been tested on real Pebble hardware after these changes.

### Audio diagnostic

If an incoming message vibrates but is silent:

1. Open **CW SMS Bridge on the Pixel**, toggle **Receive sound** OFF and
   ON to explicitly send the preference on the next SMS/test. Set the phone
   **Speaker volume** slider to at least 60. Saved Clay settings on the
   watch otherwise remain as they were previously.
2. Install this branch's PBW and repeat **Test background launch and
   Morse** from the phone.
3. Read the footer on the watch. **RX AUDIO DISABLED** indicates the
   stored receive-audio toggle is off; **VOLUME = 0** means the stored
   volume is muted; **WATCH SOUND MUTED** means the Pebble's Sounds & Haptics / Quiet Time\n   has muted speaker output; **SPEAKER OPEN FAILED** means the PCM stream could
   not be opened. If it still sounds silent with the normal `WPM | Hz`
   footer, the device/PCM path needs hardware debugging.
4. Receive and send PCM generation were otherwise intentionally left
   unchanged from the earlier working timing implementation.


## Privacy, reliability, and limitations

- The helper requests Notification Access, **not** READ_SMS, RECEIVE_SMS or
  SEND_SMS. It only processes a new notification from the current default SMS
  application when its in-app switch is enabled.
- The Android SMS app must actually post a notification with sender/body
  visible. Sending an SMS to yourself often does not produce an inbound
  notification. Test from another number.
- Only the first ~120 supported CW characters are included; longer messages
  are visibly ended with `...`. Emojis and unsupported glyphs are normalized.
- CW audio is a real square wave; timing of the already-working receive
  vibration pattern and send paddles has not been redesigned here.
- Multiple incoming notifications are serialized. Duplicate notification
  updates are filtered; failed deliveries are not marked permanently delivered.
- The watchapp returns after playback if the user did not interact. This
  behavior has **not yet been verified on Pebble Time 2 hardware**.
- A new SMS may interrupt whichever watchapp or watchface is active, as
  requested. Whether the phone OS shows or suppresses message contents and
  whether PebbleKit returns a successful ACK are device-dependent.
- Build scripts and source have been checked, but a complete Android Gradle
  build and live end-to-end watch/phone SMS test are still required.

## References

- https://developer.repebble.com/guides/communication/
- https://github.com/pebble-dev/PebbleKitAndroid2
- https://developer.android.com/reference/android/service/notification/NotificationListenerService
