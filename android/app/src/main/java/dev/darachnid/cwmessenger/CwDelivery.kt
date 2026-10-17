package dev.darachnid.cwmessenger

import android.content.Context
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.TransmissionResult
import kotlinx.coroutines.delay

/** Background-friendly phone -> watch delivery. Never logs message contents. */
internal object CwDelivery {
    private fun Map<*, TransmissionResult>?.ok(): Boolean =
        this != null && values.any { it is TransmissionResult.Success }

    private fun Map<*, TransmissionResult>?.disconnected(): Boolean =
        this != null && values.isNotEmpty() &&
            values.all { it is TransmissionResult.FailedWatchNotConnected }

    suspend fun showMessage(
        context: Context,
        title: String,
        body: String
    ): Boolean {
        val sender = DefaultPebbleSender(context.applicationContext)
        try {
            val payload = CwPreferences.packet(context).toMutableMap()
            payload[CwTransport.KEY_SENDER] = PebbleDictionaryItem.Text(title)
            payload[CwTransport.KEY_BODY] = PebbleDictionaryItem.Text(body)

            // If CW Messenger is already running, do not restart it: that could
            // destroy a partially keyed reply. Its AppMessage handler preserves
            // the draft and queues this SMS until the user leaves Send mode.
            val existing = sender.sendDataToPebble(CwTransport.APP_UUID, payload)
            if (existing.ok()) return true
            if (existing.disconnected()) return false

            // Another app / watchface is running. Open CW Messenger into the
            // Receive screen, then deliver the real SMS (no demo autoplay).
            val launched = sender.startAppOnTheWatch(CwTransport.APP_UUID)
            if (!launched.ok()) return false
            delay(700)
            repeat(4) { attempt ->
                val result = sender.sendDataToPebble(CwTransport.APP_UUID, payload)
                if (result.ok()) return true
                if (result.disconnected()) return false
                if (attempt < 3) delay(450)
            }
            return false
        } catch (_: Exception) {
            return false
        } finally {
            sender.close()
        }
    }
}
