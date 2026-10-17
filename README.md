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

## Optional watch-to-phone CW drafts

In Send, enter CW with Up/Down/Select. Double-press Back within 400ms, then
Select on the draft review screen to hand a decoded draft to Android. Android
creates a **local notification** you can tap to open the normal SMS composer.
Select a recipient and explicitly send there. No SMS is sent silently, and
this helper never requests `SEND_SMS`.

On Android 13+, grant the helper's **outgoing draft notification** permission
if you want that handoff.

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
