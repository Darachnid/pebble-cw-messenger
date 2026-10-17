# CW Messenger for Pebble Time 2

Read incoming messages as Morse code via watch speaker and haptics. Current revision is a **self-contained simulated message**; it does not yet capture phone notifications.

## v0.4 – Send mode opens by default

**When you launch the watchapp it now opens straight into SEND CW**, not the Receive demo. No hidden long press required.

- Top bar: SEND CW
- Body: decoded local CW draft and straight-key/paddle instructions
- Bottom: WPM and Hz
- UP = dit, DOWN = dah, SELECT = straight key, BACK = return to Receive
- The previous receive test stays one Back press away; tap Select there to replay it

Hold Down in Receive to return to Send. You must install the new PBW after git pull and pebble build to see this change.

## v0.3 – synchronized Morse playback

- Sender top strip: **Ben**
- Message body: **good night bens best**
- Bottom strip: **20 WPM | 600 Hz**
- No app title, debug lines, or on-screen button instructions
- The demo begins automatically when opened; **Select** replays it
- One continuous, **16 kHz signed 8-bit PCM stream**. A constant-amplitude **600 Hz 50%-duty square wave** is gated directly on/off; every Morse space is encoded as actual zero-valued PCM samples. No software attack, decay, fade, reverb, or release tail is applied.
- Timing: 20 WPM, one dit = 60 ms, one dah = 180 ms, inter-element silence = 60 ms, inter-letter silence = 180 ms, inter-word silence = 420 ms. Stream timing is determined by **PCM sample counts**, not repeated one-shot tone restarts or app timer accuracy.
- **Haptics now use one complete `VibePattern` containing the same mark/space durations** as audio instead of per-element `AppTimer`s. This eliminates cumulative callback drift. PCM still has initial playback-buffer latency, so the watch has a calibration adjustment.
- Up/Down adjust volume; Back once stops speaker audio, again disables haptics; double Back within 400 ms stops playback. The single click is delayed by the multi-click detection window. Back exits when not playing.

### Sync calibration on the watch

1. Hold **Select** for 700 ms to show `SYNC +80 ms` (initial default).
2. While in calibration, **Up / Down** adjust by 10 ms. Positive delays vibration relative to PCM; negative adds leading silence to PCM.
3. Tap **Select** to replay the test message using the new offset.
4. Hold **Select** again to return the bottom strip to `20 WPM | 600 Hz`. Up/Down once again change speaker volume.
5. Calibration is stored with Pebble persistent storage and survives app restarts.

If you hear the sound **after** the vibration, increase the sync value (+10 ms per press). If you hear the sound **before** the vibration, decrease the value (-10 ms per press).

### Known limitations

- **Physical speaker ringing** and Pebble's audio pipeline may still cause audible acoustic tails even though the emitted PCM signal is hard-gated. Real-watch listening is necessary to assess this.
- Vibration is driven by the Pebble OS custom-pattern engine. It avoids timer drift, but PCM startup buffering, actuator response, and firmware timing can still introduce a fixed mismatch. Use sync calibration while listening on hardware.
- WPM, Hz, and text are still C constants until configuration and notification transport are implemented.
- No incoming Android notification integration in this demo.

## Build and deploy from `manjo`

```bash
cd ~/pebble-cw-messenger
git pull origin main
pebble build
pebble install --cloudpebble
```

Direct Pebble SDK references: [Speaker and PCM](https://developer.repebble.com/docs/c/User_Interface/Speaker/) · [Clicks](https://developer.repebble.com/docs/c/User_Interface/Window/).

## Send / Practice (new local prototype)

**Hold Down for 700 ms on the Receive screen** to open TX mode. Note: this slightly delays the volume-down single click on the Receive screen.

| Button | Send behavior |
| --- | --- |
| Up | Dit (60 ms at 20 WPM) |
| Down | Dah (180 ms at 20 WPM) |
| Hold Up / Down | Repeat elements at one-unit spacing |
| Hold both | Alternate elements (basic squeeze, not iambic A/B yet) |
| Select | Straight key: press for sound, release to end; duration >= 2 units is a dah |
| Back | Return to Receive |

The TX screen shows a live decoded local draft and the current partial dot/dash pattern. Letters finalize after 3 units of silence; spaces after 7 units. Invalid Morse decodes to ?.

A continuous 600 Hz hard-gated square-wave PCM sidetone plays while keying. The first send build requires on-device testing for button recognition, stream-buffer latency, speaker/haptic synchronization and power consumption. It currently has no messaging transport. TX drafting is local only and resets when you reopen it.

Paddle reversal and settings will be implemented using the Pebble Android app later. The top and bottom buttons are mapped to dit and dah by default.

## Roadmap

1. Measure real hardware audio gating, onset and offset; characterize haptic/audio latency.
2. Configure tone frequency, WPM and playback mode using Pebble's phone app settings.
3. Receive phone notifications and populate sender/message fields.
4. Add sending paddles and straight-key decoder.
5. Add opt-in text compression using standard CW abbreviations and optional AI summarization.