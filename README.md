# CW Messenger for Pebble Time 2

Read incoming messages as Morse code via watch speaker and haptics. Current revision is a **self-contained simulated message**; it does not yet capture phone notifications.

## v0.2 – message layout and square-wave keying

- Sender top strip: **Ben**
- Message body: **good night bens best**
- Bottom strip: **20 WPM | 600 Hz**
- No app title, debug lines, or on-screen button instructions
- The demo begins automatically when opened; **Select** replays it
- One continuous, **16 kHz signed 8-bit PCM stream**. A constant-amplitude **600 Hz 50%-duty square wave** is gated directly on/off; every Morse space is encoded as actual zero-valued PCM samples. No software attack, decay, fade, reverb, or release tail is applied.
- Timing: 20 WPM, one dit = 60 ms, one dah = 180 ms, inter-element silence = 60 ms, inter-letter silence = 180 ms, inter-word silence = 420 ms. Stream timing is determined by **PCM sample counts**, not repeated one-shot tone restarts or app timer accuracy.
- Up/Down adjust volume; Back once stops speaker audio, again disables haptics; double Back within 400 ms stops playback. The single click is delayed by the multi-click detection window. Back exits when not playing.

### Known limitations

- **Physical speaker ringing** and Pebble's audio pipeline may still cause audible acoustic tails even though the emitted PCM signal is hard-gated. Real-watch listening is necessary to assess this.
- Vibration is driven by Pebble app timers and hardware, so haptic edges may not precisely align with sample-accurate audio edges.
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

## Roadmap

1. Measure real hardware audio gating, onset and offset; characterize haptic/audio latency.
2. Configure tone frequency, WPM and playback mode using Pebble's phone app settings.
3. Receive phone notifications and populate sender/message fields.
4. Add sending paddles and straight-key decoder.
5. Add opt-in text compression using standard CW abbreviations and optional AI summarization.