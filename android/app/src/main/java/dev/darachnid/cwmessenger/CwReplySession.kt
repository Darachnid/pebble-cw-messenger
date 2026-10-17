package dev.darachnid.cwmessenger

import android.app.Notification
import android.app.PendingIntent
import android.app.RemoteInput
import android.content.Context
import android.content.Intent
import android.service.notification.StatusBarNotification
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.TransmissionResult

/**
 * Volatile binding between the most recently received SMS notification and
 * its in-notification Android reply action. If Android kills the process,
 * the binding disappears rather than risk replying to the wrong person.
 */
internal object CwReplySession {
    private var notificationKey: String? = null
    private var replyAction: Notification.Action? = null
    private var review: String? = null
    private var reviewId: Long = 0

    @Synchronized
    fun remember(sbn: StatusBarNotification) {
        val action = sbn.notification.actions?.firstOrNull { a ->
            a.remoteInputs?.any { it.allowFreeFormInput } == true &&
                a.actionIntent != null
        }
        notificationKey = sbn.key
        replyAction = action
        review = null
        reviewId++
    }

    @Synchronized
    fun setReview(text: String): Long {
        review = text
        return ++reviewId
    }

    @Synchronized
    fun cancelReview() {
        review = null
        reviewId++
    }

    /**
     * Atomic consume: the reviewed string MUST match exactly, and an actual
     * RemoteInput action from the original SMS notification MUST be available.
     */
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
