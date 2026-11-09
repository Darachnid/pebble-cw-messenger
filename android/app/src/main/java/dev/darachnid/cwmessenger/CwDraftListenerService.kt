package dev.darachnid.cwmessenger

import android.Manifest
import android.os.Build
import android.content.pm.PackageManager
import io.rebble.pebblekit2.client.BasePebbleListenerService
import io.rebble.pebblekit2.common.model.PebbleDictionary
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.ReceiveResult
import io.rebble.pebblekit2.common.model.WatchIdentifier
import java.util.UUID
import kotlinx.coroutines.launch

/**
 * Two-phase reply: triple Back -> optional local spellcheck -> watch review;
 * Select -> exact draft confirmation -> original SMS notification RemoteInput.
 * Never auto-reply, never guess recipients, never send via SEND_SMS.
 */
class CwDraftListenerService : BasePebbleListenerService() {
    override suspend fun onMessageReceived(
        watchappUUID: UUID,
        data: PebbleDictionary,
        watch: WatchIdentifier
    ): ReceiveResult {
        if (watchappUUID != CwTransport.APP_UUID) return ReceiveResult.Nack
        val prepare = (data[CwTransport.KEY_DRAFT] as? PebbleDictionaryItem.Text)
            ?.value?.trim().orEmpty()
        if (prepare.isNotEmpty()) {
            if (prepare.length > 150) return ReceiveResult.Nack
            val reviewId = CwReplySession.beginReview()
            val appContext = applicationContext
            CwReplySession.background.launch {
                val text = CwLocalReview.spellcheck(appContext, prepare)
                if (CwReplySession.finishReview(reviewId, text)) {
                    val sent = CwReplySession.display(
                        appContext, CwTransport.KEY_REVIEW, text)
                    if (!sent) CwReplySession.cancelReview()
                }
            }
            return ReceiveResult.Ack
        }

        val confirm = (data[CwTransport.KEY_CONFIRM] as? PebbleDictionaryItem.Text)
            ?.value?.trim().orEmpty()
        if (confirm.isBlank()) return ReceiveResult.Nack

        val reply = CwReplySession.consume(confirm)
        if (reply == null) {
            CwReplySession.display(this, CwTransport.KEY_STATUS,
                "NO SMS REPLY ACTION")
            return ReceiveResult.Nack
        }
        val sent = CwReplySession.sendViaNotification(this, reply, confirm)
        CwReplySession.display(this, CwTransport.KEY_STATUS,
            if (sent) "SMS REPLY SENT" else "SMS SEND FAILED")
        return if (sent) ReceiveResult.Ack else ReceiveResult.Nack
    }
}
