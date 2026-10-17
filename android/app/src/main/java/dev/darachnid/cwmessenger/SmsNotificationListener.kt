package dev.darachnid.cwmessenger

import android.app.Notification
import android.provider.Telephony
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.text.Normalizer

/**
 * Default-SMS-app notification bridge. This app does not request READ_SMS or
 * SEND_SMS, and remains off until the user grants Notification Access AND
 * enables the in-app forwarding switch. No notification contents are logged.
 */
class SmsNotificationListener : NotificationListenerService() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val sentRecently = LinkedHashMap<String, Int>()
    // Process launches/transfers serially: two incoming SMS notifications must
    // not race to open the watch or discard each other's AppMessage.
    private val deliveryLock = Mutex()

    private fun cwFriendly(input: String, max: Int): String {
        val normalized = Normalizer.normalize(input, Normalizer.Form.NFKD)
        val out = StringBuilder(max)
        val permitted = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,?/:=+-@'"
        for (char in normalized.uppercase()) {
            if (Character.getType(char) == Character.NON_SPACING_MARK.toInt()) continue
            if (out.length >= max) break
            out.append(if (permitted.contains(char)) char else '?')
        }
        return out.toString().trim()
    }

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        if (sbn == null || !getSharedPreferences("privacy", MODE_PRIVATE)
                .getBoolean("enabled", false)) return

        // Restrict to the current DEFAULT SMS app, not arbitrary notifications.
        val defaultSmsPackage = Telephony.Sms.getDefaultSmsPackage(this) ?: return
        if (sbn.packageName != defaultSmsPackage) return
        val notification = sbn.notification
        if ((notification.flags and Notification.FLAG_GROUP_SUMMARY) != 0) return
        if (notification.category != Notification.CATEGORY_MESSAGE) return

        val senderText = notification.extras.getCharSequence(Notification.EXTRA_TITLE)
            ?.toString()?.trim().orEmpty()
        val bodyText = (notification.extras.getCharSequence(Notification.EXTRA_BIG_TEXT)
            ?: notification.extras.getCharSequence(Notification.EXTRA_TEXT))
            ?.toString()?.trim().orEmpty()
        if (senderText.isBlank() || bodyText.isBlank()) return

        // Watch AppMessage buffers and C strings are small. Normalize Morse
        // support explicitly; do not cut a UTF-8 code point in the middle.
        val title = cwFriendly(senderText, 31)
        // Explicit ellipsis if longer than watch-sized 120-character payload;
        // this initial beta does not silently suggest it read the full SMS.
        val body = if (bodyText.length > 120) {
            cwFriendly(bodyText, 117) + "..."
        } else {
            cwFriendly(bodyText, 120)
        }
        if (title.isBlank() || body.isBlank()) return

        val key = sbn.key
        val signature = (title + "\u0000" + body + "\u0000" + sbn.postTime).hashCode()
        synchronized(sentRecently) {
            if (sentRecently[key] == signature) return
            // Reserve the in-flight message so rapid notification updates
            // do not generate duplicates while awaiting Pebble ACK.
            sentRecently[key] = signature
            if (sentRecently.size > 64) {
                sentRecently.remove(sentRecently.keys.first())
            }
        }

        scope.launch {
            deliveryLock.withLock {
                // Recheck opt-in in case the user switched it off while this
                // notification was queued. Do not wake the watch if disabled.
                val allowed = getSharedPreferences("privacy", MODE_PRIVATE)
                    .getBoolean("enabled", false)
                val transferred = allowed &&
                    CwDelivery.showMessage(this@SmsNotificationListener, title, body)
                if (!transferred) {
                    synchronized(sentRecently) {
                        if (sentRecently[key] == signature) sentRecently.remove(key)
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
