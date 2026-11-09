package dev.darachnid.cwmessenger

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

class MainActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val privacy = getSharedPreferences("privacy", MODE_PRIVATE)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 36, 32, 56)
        }
        setContentView(ScrollView(this).apply { addView(layout) })

        CwSettingsUi.sectionTitle(this, layout, "CW Messenger", 24f)
        CwSettingsUi.bodyText(
            this,
            layout,
            "SMS becomes Morse on your Pebble. Compose replies with the paddle keys " +
                "(UP dit, DOWN dah) or the straight key (SELECT).",
        )
        CwSettingsUi.sectionTitle(this, layout, "Workflow", 20f)
        CwSettingsUi.bodyText(
            this,
            layout,
            "1. Grant notification access and enable background SMS.\n" +
                "2. First watch launch: guided tour (SELECT).\n" +
                "3. New SMS opens Receive; Morse plays if autoplay is on.\n" +
                "4. HOLD DOWN to key a reply; triple BACK to spellcheck and send.\n" +
                "5. LONG UP on Receive replays the tour.",
        )

        CwSettingsUi.actionButton(this, layout, "Messages") {
            startActivity(Intent(this, MessagesActivity::class.java))
        }
        CwSettingsUi.actionButton(this, layout, "User settings") {
            startActivity(Intent(this, UserSettingsActivity::class.java))
        }

        CwSettingsUi.sectionTitle(this, layout, "Permissions", 20f)
        CwSettingsUi.toggle(this, layout, privacy, "Enable background incoming SMS", "enabled", false)
        CwSettingsUi.actionButton(this, layout, "Grant notification access") {
            startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
        }
        CwSettingsUi.bodyText(
            this,
            layout,
            "We read only your default SMS app notifications on this device. " +
                "Nothing is uploaded to a server.",
        )

        CwSettingsUi.sectionTitle(this, layout, "Tools", 20f)
        CwSettingsUi.actionButton(this, layout, "Test CW on watch") {
            scope.launch {
                val delivered = CwDelivery.showMessage(
                    this@MainActivity, "CW Test", "test",
                )
                Toast.makeText(
                    this@MainActivity,
                    if (delivered) "CW test delivered" else "Watch not connected",
                    Toast.LENGTH_LONG,
                ).show()
            }
        }
        CwSettingsUi.actionButton(this, layout, "Allow draft reply notifications") {
            if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) !=
                    PackageManager.PERMISSION_GRANTED) {
                requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 44)
            } else {
                Toast.makeText(this, "Notification permission OK", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
