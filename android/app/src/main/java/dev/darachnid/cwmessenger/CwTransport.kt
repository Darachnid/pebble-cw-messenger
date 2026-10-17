package dev.darachnid.cwmessenger

import java.util.UUID

internal object CwTransport {
    // Exact UUID from Pebble watchapp package.json.
    val APP_UUID: UUID = UUID.fromString("24ae7051-9f15-42a3-805e-e4b4a22bc64d")
    // Pebble SDK auto-allocates messageKeys starting at 10000.
    // Verify these IDs against build/include/message_keys.auto.h after pebble build.
    const val KEY_SENDER: UInt = 10014u
    const val KEY_BODY: UInt = 10015u
    const val KEY_DRAFT: UInt = 16u
}
