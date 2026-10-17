# Pebble CW Messenger

Native Morse-code watchapp for the **Pebble Time 2 (emery)**.

## Working prototype (v0.1)

- Select starts/restarts a demo message: **SOS AC1RV**
- International Morse code encoding for A-Z, 0-9, and common punctuation
- PARIS timing ratios: dit=1, dah=3, intra-character gap=1, letter gap=3, word gap=7
- 20 WPM, 600 Hz sine-wave audio and matching haptic patterns
- Up/Down change speaker volume in 10% increments during playback
- Back single click disables audio first, then vibration on a later single click
- Back double click within 400 ms stops playback
- Back exits the app while idle
- Speaker system mute/Quiet Time still take precedence

**MVP limitations:** Demo text, speed, and pitch are constants in the C source, not yet phone-app settings. No notification integration, keying, or AI yet. SDK app timers and vibration hardware may introduce playback jitter; physical CW accuracy and speaker/haptic synchronization still require testing on the Pebble Time 2. Back single-click is delayed by the double-click detection window.

## Build and install

From your Pebble SDK computer:

    git clone https://github.com/Darachnid/pebble-cw-messenger.git
    cd pebble-cw-messenger
    pebble build
    pebble install --cloudpebble

If you already cloned the project:

    cd ~/pebble-cw-messenger
    git pull origin main
    pebble build

The Pebble emulator needs a graphical display (SDL). On a headless server, build normally and install on a paired watch instead.

## Next milestones

1. Measure real-world CW timing and address speaker/vibration scheduling drift.
2. Add Pebble phone-app settings for WPM, tone frequency, audio, and haptics.
3. Integrate Android notifications using an Android companion and evaluate phone settings compatibility.
4. Implement configurable dit/dah paddle and middle-button straight key.
5. Add explicit, meaning-preserving CW abbreviation rules and optional AI conversion.

## SDK references

- [Buttons and clicks](https://developer.repebble.com/docs/c/User_Interface/Clicks/)
- [Speaker API](https://developer.repebble.com/docs/c/User_Interface/Speaker/)
- [Vibration API](https://developer.repebble.com/docs/c/User_Interface/Vibes/)
