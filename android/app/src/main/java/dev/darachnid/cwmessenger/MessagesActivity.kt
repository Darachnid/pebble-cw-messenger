package dev.darachnid.cwmessenger

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ListView
import android.widget.TextView
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import java.text.DateFormat
import java.util.Date

class MessagesActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private val store by lazy { CwMessageStore(this) }
    private lateinit var inboxList: ListView
    private lateinit var threadPanel: LinearLayout
    private lateinit var inboxPanel: LinearLayout
    private var currentThread: String? = null
    private var threadSummaries: List<CwMessageStore.ThreadSummary> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(24, 32, 24, 32)
        }
        setContentView(root)

        inboxPanel = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        threadPanel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            visibility = View.GONE
        }
        root.addView(inboxPanel)
        root.addView(threadPanel)

        CwSettingsUi.sectionTitle(this, inboxPanel, "Messages", 22f)
        CwSettingsUi.bodyText(
            this, inboxPanel,
            "History from SMS notifications while CW Bridge was enabled. No cloud sync.",
        )
        inboxList = ListView(this)
        inboxPanel.addView(inboxList)
        inboxList.onItemClickListener = AdapterView.OnItemClickListener { _, _, pos, _ ->
            openThread(threadSummaries[pos])
        }
        refreshInbox()

        val backBtn = Button(this).apply {
            text = "Back to inbox"
            setOnClickListener {
                threadPanel.visibility = View.GONE
                inboxPanel.visibility = View.VISIBLE
                currentThread = null
                refreshInbox()
            }
        }
        threadPanel.addView(backBtn)
    }

    private fun refreshInbox() {
        threadSummaries = store.listThreads()
        val rows = threadSummaries.map { t ->
            val whenStr = DateFormat.getDateTimeInstance().format(Date(t.receivedAt))
            "${t.sender}: ${t.preview.take(50)}\n$whenStr"
        }
        inboxList.adapter = ArrayAdapter(
            this, android.R.layout.simple_list_item_1, android.R.id.text1, rows,
        )
    }

    private fun openThread(thread: CwMessageStore.ThreadSummary) {
        currentThread = thread.threadKey
        inboxPanel.visibility = View.GONE
        threadPanel.visibility = View.VISIBLE
        while (threadPanel.childCount > 1) {
            threadPanel.removeViewAt(1)
        }
        val title = TextView(this).apply {
            text = thread.sender
            textSize = 18f
            setPadding(0, 8, 0, 16)
        }
        threadPanel.addView(title)

        for (msg in store.messagesForThread(thread.threadKey)) {
            threadPanel.addView(TextView(this).apply {
                text = "${msg.sender}: ${msg.body}"
                textSize = 15f
                setPadding(0, 6, 0, 6)
            })
        }

        val reply = EditText(this).apply { hint = "Type a reply" }
        threadPanel.addView(reply)

        fun sendRow(label: String, action: (String) -> Unit) {
            threadPanel.addView(Button(this).apply {
                text = label
                setOnClickListener {
                    val text = reply.text.toString().trim()
                    if (text.isEmpty()) {
                        Toast.makeText(this@MessagesActivity, "Enter text first", Toast.LENGTH_SHORT).show()
                        return@setOnClickListener
                    }
                    action(text)
                }
            })
        }

        sendRow("Send via notification reply") { text ->
            val key = store.latestNotificationKey(thread.threadKey)
            val ok = CwReplySession.sendReplyForKey(this, key ?: "", text)
            Toast.makeText(
                this,
                if (ok) "Sent" else "Reply unavailable — open Messages or use the watch",
                Toast.LENGTH_LONG,
            ).show()
        }
        sendRow("Play on watch (Morse)") { _ ->
            val latest = store.messagesForThread(thread.threadKey).lastOrNull() ?: return@sendRow
            scope.launch {
                val ok = CwDelivery.showMessage(this@MessagesActivity, latest.sender, latest.body)
                Toast.makeText(
                    this@MessagesActivity,
                    if (ok) "Sent to watch" else "Watch not connected",
                    Toast.LENGTH_SHORT,
                ).show()
            }
        }
        sendRow("Open SMS compose (fallback)") { text ->
            val intent = Intent(Intent.ACTION_SENDTO, Uri.parse("smsto:"))
            intent.putExtra("sms_body", text)
            startActivity(Intent.createChooser(intent, "Send SMS"))
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }

}
