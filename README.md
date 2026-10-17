# Pebble CW Messenger

Morse-code message reader and keyer for **Pebble Time 2**. It has two
independent modes:

- **Receive:** hard-gated square-wave PCM at the selected pitch, with a
  precomputed vibration pattern that has been hardware-tested for alignment.
- **Send:** Up/Down Morse paddles and Select straight key, live decoded text,
  immediate square-wave sidetone, and optional vibration.

## Versions / branches

- `main`: stable watch-only Receive/Send with phone-app settings (v0.5).
- `feature/android-sms-bridge`: **v0.6 beta** incoming Android SMS notification
  bridge and explicitly confirmed CW-to-SMS composer handoff. Not yet tested
  on your watch/phone; please do not assume it is production-ready.

## Install beta watchapp on manjo

```sh
cd ~/pebble-cw-messenger
git fetch origin
git switch -c feature/android-sms-bridge --track origin/feature/android-sms-bridge
# If you already created the branch, use: git switch feature/android-sms-bridge
pebble package install @rebble/clay
pebble build
pebble install --cloudpebble
```

Phone settings remain inside the **Pebble app → CW Messenger → Settings**.
Speed 5–40 WPM, pitch 200–1500 Hz, volume, independent audio/haptic modes,
paddle reversal, straight-key dah threshold, startup mode, autoplay,
receive sync offset, and demo text are configurable.

## Android SMS companion

See **[android/README.md](android/README.md)** for build, installation,
permissions and tests. Install the companion APK with Android Studio.
The companion only watches notifications from the *default SMS app*, with
explicit opt-in and Android Notification Access.

The phone tests should first be conducted with the helper's **synthetic
test message button** (notification access off). Only turn on notification
access if the synthetic test and Clay settings both work.

### Incoming SMS

1. A new qualifying SMS notification appears on the phone.
2. The opt-in helper asks the Pebble to open the CW watchapp and transfers
   the sanitized sender and message over AppMessage.
3. If the Send screen is idle, the watch switches to Receive and plays Morse.
4. If the user is actively keying or has a draft, it preserves the draft;
   the new message is available in Receive and autoplays when shown.

The beta forwards up to 120 characters (with `...` for longer messages),
and converts unsupported symbols to `?`. It does not silently read the
remaining length of longer messages.

### Outgoing SMS draft

1. Compose text on the watch using Up/Down and/or Select.
2. **Double Back within 400 ms** to open a confirmation screen.
3. **Press Select** to request transfer of the CW text to the Android
   companion.
4. Tap **CW message ready** on the phone to open the system SMS composer.
5. Choose the recipient and **press Send on the phone**.

The application does not send SMS automatically, look up contacts, or
request SEND_SMS. The watch displays **HANDOFF REQUESTED**, not "SMS sent",
because native delivery is subject to Android/Core and PKJS compatibility.

## Validation checklist

- Verify `pebble build` on manjo, and Android Gradle `assembleDebug`.
- Confirm Clay Settings still open and change CW pitch/WPM.
- Confirm the synthetic test appears in Receive and plays in sync.
- Confirm a real test SMS arrives once and sender/body are correct.
- Verify a draft is never discarded when an incoming SMS arrives.
- Confirm double-Back opens the review window and that no SMS is sent
  without interacting with the phone composer.
- Only after all tests pass, merge SMS beta into `main`.

## Technical details

- Pebble C SDK: `src/c/`.
- PebbleKit JS / Clay settings: `src/pkjs/`.
- Android companion: `android/` (PebbleKit Android 2 1.3.2).
- Pebble AppMessage keys `SmsSender=14`, `SmsBody=15`, `TxDraft=16`.
- App UUID `24ae7051-9f15-42a3-805e-e4b4a22bc64d`.

**Known compatibility constraint:** Official Pebble documentation historically
disallows PebbleKit JS and PebbleKit Android in one watchapp. More recent
Core-app issue reports suggest phone-to-watch messaging works for multiple
senders, but watch-to-phone delivery is not uniformly supported in all
app versions. **Both directions need validation before this is a final release.**
