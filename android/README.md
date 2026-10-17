# CW SMS Bridge (Android companion)

Beta companion for the **Pebble Time 2** CW Messenger watchapp.

## What it does

- Watches **notifications posted by the phone's default SMS app** only.
- If you explicitly enabled forwarding, sends the sender and first 120 CW-supported
  characters to the paired Pebble using **PebbleKit Android 2**.
- Supports a button to transmit a harmless sample to the watch before requesting
  broad Android Notification Access.
- Receives decoded CW drafts from the watch and posts an Android notification that
  opens the system's SMS composer when tapped.
- It NEVER directly sends an SMS, and NEVER guesses a recipient.

## Build/install

Use Android Studio on `manjo` or another workstation with the Android SDK.

1. Open the `android/` subfolder as the project in Android Studio.
2. Allow Gradle to sync and install Android SDK Platform 35 if requested.
3. Build the app: **Build → Build APK(s)**, or run
   `gradle -p android :app:assembleDebug` if Gradle 8.11.1 and Android SDK 35
   are already installed.
4. Install `android/app/build/outputs/apk/debug/app-debug.apk` on your Android
   phone via Android Studio, or `adb install -r <file>`.
5. Enable Developer Connection in the Pebble Android app and install the watch
   `.pbw` from the **same feature branch**.

The SDK source of truth is [PebbleKitAndroid2](https://github.com/pebble-dev/PebbleKitAndroid2).
This beta uses `io.rebble.pebblekit2:client:1.3.2`.

## First-run validation

1. Open **CW SMS Bridge** on the phone; keep forwarding **off**.
2. Tap **Send a harmless test message to Pebble**. The Pebble should display
   sender `CW Test`, body `test message` and play Morse if autoplay is enabled.
3. Open CW Messenger's **Settings** in the regular Pebble phone app to verify
   Clay still works (e.g. change WPM 20→15).
4. Enable **Forward incoming SMS to Pebble** and grant Android Notification
   Access to CW SMS Bridge. This is a sensitive permission: the Android OS makes
   other notifications technically accessible, but our code filters the default
   SMS app package and notification category.
5. Send a test SMS to yourself from another phone. Verify sender/body and
   duplicates, and that an active Send draft is not interrupted.
6. In Send mode, key a CW word. **Double Back** within 400 ms, review draft and
   press **Select**. Grant **Allow draft handoff notifications** in the helper.
   Tap the received phone notification to open the SMS composer. Select a
   recipient and press Send there if you really want to send it.

## Limitations

- **Not yet device-tested:** The Core app's native companion + PebbleKit JS
  coexistence is version-sensitive. The documented PKJS restriction and Core
  discussions mean this beta must pass its hardware tests before merging.
- Android's default SMS app notification must contain visible text. Hidden,
  silent or privacy-redacted message bodies are unavailable.
- Only the first 120 characters are forwarded; longer messages end in `...`.
  Accented Latin letters are normalized and unsupported symbols become `?`.
- The current decoder handles International Morse characters, but does not
  compose or transmit the full original SMS thread.
- Outgoing draft delivery relies on watch-to-companion AppMessage routing.
  If only PKJS receives that message, no Android handoff notification appears.
  Treat the watch's "HANDOFF REQUESTED" as a request, not confirmation of delivery.
- No SMS database, contact lookup, recipient matching or SEND_SMS permission.
- No automatic outgoing messages and no SMS contents uploaded to an external
  server by our implementation.
