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
            val text = CwLocalReview.spellcheck(this, prepare)
            CwReplySession.setReview(text)
            val sent = CwReplySession.display(this, CwTransport.KEY_REVIEW, text)
            if (!sent) CwReplySession.cancelReview()
            return if (sent) ReceiveResult.Ack else ReceiveResult.Nack
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
