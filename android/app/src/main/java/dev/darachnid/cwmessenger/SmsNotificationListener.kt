package dev.darachnid.cwmessenger

import android.app.Notification
import android.provider.Telephony
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * Notification-driven (not SMS database reading), privacy-first prototype.
 * Some SMS apps hide sender/content in notifications or update summaries.
 * This sends nothing unless the user explicitly toggles forwarding ON.
 */
class SmsNotificationListener : NotificationListenerService() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val sentRecently = LinkedHashMap<String, Int>()

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        if (sbn == null) return
        if (!getSharedPreferences("privacy", MODE_PRIVATE)
                .getBoolean("enabled", false)) return

        // Strictly restrict access to the system's current default SMS app.
        // Never forward unrelated chats or general device notifications.
        val defaultSmsApp = Telephony.Sms.getDefaultSmsPackage(this) ?: return
        if (sbn.packageName != defaultSmsApp) return
        val notification = sbn.notification ?: return
        if ((notification.flags and Notification.FLAG_GROUP_SUMMARY) != 0) return
        if (notification.category != Notification.CATEGORY_MESSAGE) return

        val title = notification.extras.getCharSequence(Notification.EXTRA_TITLE)
            ?.toString()?.trim().orEmpty()
        val body = (notification.extras.getCharSequence(Notification.EXTRA_BIG_TEXT)
            ?: notification.extras.getCharSequence(Notification.EXTRA_TEXT))
            ?.toString()?.trim().orEmpty()
        if (title.isBlank() || body.isBlank()) return

        val dedupKey = sbn.key
        val signature = (title + "\u0000" + body).hashCode()
        if (sentRecently[dedupKey] == signature) return
        sentRecently[dedupKey] = signature
        if (sentRecently.size > 64) {
            sentRecently.remove(sentRecently.keys.first())
        }

        scope.launch {
            val sender = DefaultPebbleSender(this@SmsNotificationListener)
            try {
                // Opening first permits the watch to register AppMessage.
                // Compatibility with Clay/PKJS needs confirmation on device.
                sender.startAppOnTheWatch(CwTransport.APP_UUID)
                delay(600)
                sender.sendDataToPebble(CwTransport.APP_UUID, mapOf(
                    CwTransport.KEY_SENDER to PebbleDictionaryItem.Text(title.take(31)),
                    CwTransport.KEY_BODY to PebbleDictionaryItem.Text(body.take(120))
                ))
            } finally {
                sender.close()
            }
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
