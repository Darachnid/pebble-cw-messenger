package dev.darachnid.cwmessenger

import android.content.Context
import android.content.SharedPreferences
import io.rebble.pebblekit2.common.model.PebbleDictionary
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem

/** Saved locally on Android. The watch persists matching settings on delivery. */
internal object CwPreferences {
    fun prefs(context: Context): SharedPreferences =
        context.getSharedPreferences("cw_settings", Context.MODE_PRIVATE)

    fun bool(context: Context, name: String, default: Boolean = true): Boolean =
        prefs(context).getBoolean(name, default)

    fun int(context: Context, name: String, default: Int): Int =
        prefs(context).getInt(name, default)

    // Never overwrite the watch's existing calibrated preferences until the
    // user has actually edited something on their phone.
    fun packet(context: Context): PebbleDictionary {
        val p = prefs(context)
        if (!p.getBoolean("configured", false)) return emptyMap()
        fun number(name: String, default: Int): PebbleDictionaryItem =
            PebbleDictionaryItem.Int32(p.getInt(name, default))
        fun toggle(name: String, default: Boolean = true): PebbleDictionaryItem =
            PebbleDictionaryItem.Int32(if (p.getBoolean(name, default)) 1 else 0)
        return mapOf(
            CwTransport.KEY_WPM to number("wpm", 20),
            CwTransport.KEY_TONE to number("tone_hz", 600),
            CwTransport.KEY_VOLUME to number("volume", 60),
            CwTransport.KEY_RX_AUDIO to toggle("rx_audio"),
            CwTransport.KEY_RX_VIBE to toggle("rx_vibe"),
            CwTransport.KEY_TX_AUDIO to toggle("tx_audio"),
            CwTransport.KEY_TX_VIBE to toggle("tx_vibe"),
            CwTransport.KEY_REVERSE to toggle("reverse", false),
            CwTransport.KEY_THRESHOLD to number("threshold", 200),
            CwTransport.KEY_SYNC to number("sync_ms", 80),
            CwTransport.KEY_START_MODE to PebbleDictionaryItem.Int32(1),
            CwTransport.KEY_AUTOPLAY to PebbleDictionaryItem.Int32(1),
            CwTransport.KEY_DEMO_SENDER to PebbleDictionaryItem.Text(
                p.getString("demo_sender", "Ben") ?: "Ben"),
            CwTransport.KEY_DEMO_MESSAGE to PebbleDictionaryItem.Text(
                p.getString("demo_message", "good night bens best") ?: "good night bens best")
        )
    }

    fun markConfigured(context: Context) {
        prefs(context).edit().putBoolean("configured", true).apply()
    }
}
