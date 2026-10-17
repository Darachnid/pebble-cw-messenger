package dev.darachnid.cwmessenger

import java.util.UUID

internal object CwTransport {
    // Exact UUID from Pebble watchapp package.json.
    val APP_UUID: UUID = UUID.fromString("24ae7051-9f15-42a3-805e-e4b4a22bc64d")
    // Pebble messageKeys are assigned sequentially, starting at zero.
    const val KEY_SENDER: UInt = 14u
    const val KEY_BODY: UInt = 15u
}
