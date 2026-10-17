package dev.darachnid.cwmessenger

import java.util.UUID

internal object CwTransport {
    val APP_UUID: UUID = UUID.fromString("24ae7051-9f15-42a3-805e-e4b4a22bc64d")

    // Pebble SDK-generated keys for this package.json (starting at 10000).
    // Build script asserts these stay synchronized with message_keys.auto.h.
    const val KEY_WPM: UInt = 10000u
    const val KEY_TONE: UInt = 10001u
    const val KEY_VOLUME: UInt = 10002u
    const val KEY_RX_AUDIO: UInt = 10003u
    const val KEY_RX_VIBE: UInt = 10004u
    const val KEY_TX_AUDIO: UInt = 10005u
    const val KEY_TX_VIBE: UInt = 10006u
    const val KEY_REVERSE: UInt = 10007u
    const val KEY_THRESHOLD: UInt = 10008u
    const val KEY_SYNC: UInt = 10009u
    const val KEY_START_MODE: UInt = 10010u
    const val KEY_AUTOPLAY: UInt = 10011u
    const val KEY_DEMO_SENDER: UInt = 10012u
    const val KEY_DEMO_MESSAGE: UInt = 10013u
    const val KEY_SENDER: UInt = 10014u
    const val KEY_BODY: UInt = 10015u
    const val KEY_DRAFT: UInt = 10016u
    const val KEY_REVIEW: UInt = 10017u
    const val KEY_CONFIRM: UInt = 10018u
    const val KEY_STATUS: UInt = 10019u
}
