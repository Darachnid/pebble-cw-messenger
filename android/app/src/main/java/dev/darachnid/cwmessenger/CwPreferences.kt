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
        val values = mutableMapOf<UInt, PebbleDictionaryItem>()
        fun number(key: UInt, name: String) {
            if (p.contains(name)) values[key] = PebbleDictionaryItem.Int32(p.getInt(name, 0))
        }
        fun toggle(key: UInt, name: String) {
            if (p.contains(name)) values[key] = PebbleDictionaryItem.Int32(
                if (p.getBoolean(name, false)) 1 else 0
            )
        }
        number(CwTransport.KEY_WPM, "wpm")
        number(CwTransport.KEY_TONE, "tone_hz")
        number(CwTransport.KEY_VOLUME, "volume")
        toggle(CwTransport.KEY_RX_AUDIO, "rx_audio")
        toggle(CwTransport.KEY_RX_VIBE, "rx_vibe")
        toggle(CwTransport.KEY_TX_AUDIO, "tx_audio")
        toggle(CwTransport.KEY_TX_VIBE, "tx_vibe")
        toggle(CwTransport.KEY_REVERSE, "reverse")
        number(CwTransport.KEY_THRESHOLD, "threshold")
        number(CwTransport.KEY_SYNC, "sync_ms")
        if (p.contains("demo_sender")) values[CwTransport.KEY_DEMO_SENDER] =
            PebbleDictionaryItem.Text(p.getString("demo_sender", "") ?: "")
        if (p.contains("demo_message")) values[CwTransport.KEY_DEMO_MESSAGE] =
            PebbleDictionaryItem.Text(p.getString("demo_message", "") ?: "")
        // Startup Receive and SMS auto-play are intentionally forced by
        // the watchapp background-message flow, not configuration fields.
        return values
    }

    fun markConfigured(context: Context) {
        prefs(context).edit().putBoolean("configured", true).apply()
    }
}
