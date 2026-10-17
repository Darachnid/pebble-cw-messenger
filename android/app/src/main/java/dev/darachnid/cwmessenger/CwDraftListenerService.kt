package dev.darachnid.cwmessenger

import android.Manifest
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import io.rebble.pebblekit2.client.BasePebbleListenerService
import io.rebble.pebblekit2.common.model.PebbleDictionary
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.ReceiveResult
import io.rebble.pebblekit2.common.model.WatchIdentifier
import java.util.UUID

/**
 * A watch CW draft is NOT an outgoing SMS until the user explicitly taps
 * the phone notification, chooses a recipient, and presses Send in the SMS app.
 * No SEND_SMS permission, background activity launch or phone-number guessing.
 */
class CwDraftListenerService : BasePebbleListenerService() {
    override suspend fun onMessageReceived(
        watchappUUID: UUID,
        data: PebbleDictionary,
        watch: WatchIdentifier
    ): ReceiveResult {
        if (watchappUUID != CwTransport.APP_UUID) return ReceiveResult.Nack
        val draft = (data[CwTransport.KEY_DRAFT] as? PebbleDictionaryItem.Text)
            ?.value?.trim().orEmpty()
        if (draft.isEmpty()) return ReceiveResult.Nack

        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) !=
                PackageManager.PERMISSION_GRANTED) {
            // Avoid losing a requested draft without user-visible feedback.
            return ReceiveResult.Nack
        }

        val notificationManager = getSystemService(NotificationManager::class.java)
        val channel = "cw_sms_drafts"
        notificationManager.createNotificationChannel(
            NotificationChannel(channel, "CW outgoing drafts",
                NotificationManager.IMPORTANCE_DEFAULT)
        )

        val smsIntent = Intent(Intent.ACTION_SENDTO, Uri.parse("smsto:")).apply {
            putExtra("sms_body", draft.take(160))
            flags = Intent.FLAG_ACTIVITY_NEW_TASK
        }
        if (smsIntent.resolveActivity(packageManager) == null) {
            return ReceiveResult.Nack
        }
        val pending = PendingIntent.getActivity(
            this,
            (System.currentTimeMillis() % 1000000L).toInt(),
            smsIntent,
            PendingIntent.FLAG_ONE_SHOT or PendingIntent.FLAG_IMMUTABLE
        )
        val notification = Notification.Builder(this, channel)
            .setSmallIcon(android.R.drawable.ic_dialog_email)
            .setContentTitle("CW message ready")
            .setContentText("Tap to choose recipient and send SMS")
            .setContentIntent(pending)
            .setAutoCancel(true)
            .setVisibility(Notification.VISIBILITY_PRIVATE)
            .build()
        notificationManager.notify((System.currentTimeMillis() % 1000000L).toInt(),
            notification)
        return ReceiveResult.Ack
    }
}
