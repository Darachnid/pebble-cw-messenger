# pebble-cw-messenger

Receive SMS on the watch as Morse (audio or vibes); compose replies with **paddle keys** or the **straight key**, via the Android companion.

## Building & running

```sh
pebble build
pebble install --emulator emery       # rectangular Pebble Time 2
pebble install --emulator chalk       # round Pebble Time Round
pebble install --phone <ip>           # paired phone
./scripts/capture-cw-screenshots.sh   # watch screenshots (emery, chalk, …)
./scripts/generate-brand-assets.py   # Pebble menu icon (25px) + Android launcher
./scripts/capture-store-assets.sh    # docs/store marketing PNGs
```

Round platforms (`chalk`, `gabbro`) use extra side insets and slightly smaller type so header, body, and footer stay inside the circle. Those models have **no speaker** — receive/send CW uses **vibration only**.

**First launch** shows a guided welcome tour (SELECT to advance, BACK to go back or skip). The footer shows hints such as `HLD DN: SEND` or `SEL: REPLAY` on live SMS.

## Android companion

See `android/README.md`. Keys in `package.json` must stay aligned with `android/.../CwTransport.kt` (checked by `scripts/validate-native-sms.py`).

## K2 dashboard (experimental)

Sources under `src/c/dashboard/` and `src/c/main.c` are a separate K2 radio face; they are not linked by the default `wscript` (this project builds **CW Messenger**).

## Project layout

```
src/c/           Watchapp (pebble-cw-messenger.c, send.c, config.c, cw_ui.c)
android/         Native SMS / PebbleKit2 companion
scripts/         Screenshot and validation helpers
package.json     UUID, platforms, message keys, companion app
wscript          Build rules — links CW app sources only
```

Docs: <https://developer.repebble.com>
