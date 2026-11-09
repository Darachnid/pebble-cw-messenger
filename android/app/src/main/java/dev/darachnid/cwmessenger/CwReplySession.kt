package dev.darachnid.cwmessenger

import android.app.Notification
import android.app.PendingIntent
import android.app.RemoteInput
import android.content.Context
import android.content.Intent
import android.service.notification.StatusBarNotification
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.TransmissionResult

/**
 * Bindings between SMS notifications and RemoteInput reply actions.
 * Watch draft confirm still uses the most recent binding via [consume].
 */
internal object CwReplySession {
    val background = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private data class Binding(val action: Notification.Action?, val at: Long)

    private var notificationKey: String? = null
    private var replyAction: Notification.Action? = null
    private val bindings = LinkedHashMap<String, Binding>()
    private var review: String? = null
    private var reviewId: Long = 0

    private const val TTL_MS = 86_400_000L
    private const val MAX_BINDINGS = 48

    @Synchronized
    fun remember(sbn: StatusBarNotification) {
        val action = sbn.notification.actions?.firstOrNull { a ->
            a.remoteInputs?.any { it.allowFreeFormInput } == true &&
                a.actionIntent != null
        }
        notificationKey = sbn.key
        replyAction = action
        bindings[sbn.key] = Binding(action, System.currentTimeMillis())
        while (bindings.size > MAX_BINDINGS) {
            bindings.remove(bindings.keys.first())
        }
        review = null
        reviewId++
    }

    @Synchronized
    fun replyActionFor(key: String?): Notification.Action? {
        if (key == null) return null
        val binding = bindings[key] ?: return null
        if (System.currentTimeMillis() - binding.at > TTL_MS) {
            bindings.remove(key)
            return null
        }
        return binding.action
    }

    @Synchronized
    fun sendReplyForKey(context: Context, notificationKey: String, text: String): Boolean {
        val action = replyActionFor(notificationKey) ?: return false
        return sendViaNotification(context, action, text)
    }

    @Synchronized
    fun beginReview(): Long {
        review = null
        return ++reviewId
    }

    @Synchronized
    fun finishReview(id: Long, text: String): Boolean {
        if (id != reviewId) return false
        review = text
        return true
    }

    @Synchronized
    fun cancelReview() {
        review = null
        reviewId++
    }

    @Synchronized
    fun consume(text: String): Notification.Action? {
        if (review == null || review != text) return null
        review = null
        reviewId++
        return replyAction
    }

    @Synchronized
    fun hasReplyAction(): Boolean = replyAction != null

    fun sendViaNotification(context: Context, action: Notification.Action, text: String): Boolean {
        val inputs = action.remoteInputs ?: return false
        val input = inputs.firstOrNull { it.allowFreeFormInput } ?: return false
        return try {
            val intent = Intent()
            RemoteInput.addResultsToIntent(inputs, intent, android.os.Bundle().apply {
                putCharSequence(input.resultKey, text)
            })
            action.actionIntent.send(context, 0, intent)
            true
        } catch (_: PendingIntent.CanceledException) {
            false
        } catch (_: Exception) {
            false
        }
    }

    suspend fun display(context: Context, key: UInt, message: String): Boolean {
        val sender = DefaultPebbleSender(context.applicationContext)
        return try {
            val result = sender.sendDataToPebble(
                CwTransport.APP_UUID,
                mapOf(key to PebbleDictionaryItem.Text(message.take(159)))
            )
            result?.values?.any { it is TransmissionResult.Success } == true
        } catch (_: Exception) {
            false
        } finally {
            sender.close()
        }
    }
}
