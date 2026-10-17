package dev.darachnid.cwmessenger

import android.Manifest
import android.app.Activity
import android.os.Build
import android.content.pm.PackageManager
import android.widget.Toast
import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import android.widget.Button
import android.widget.LinearLayout
import android.widget.Switch
import android.widget.TextView
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import java.util.UUID

/**
 * Explicit opt-in: never forward notifications without BOTH Android
 * notification-listener access and the user-controlled in-app toggle.
 */
class MainActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val prefs = getSharedPreferences("privacy", MODE_PRIVATE)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 48, 32, 32)
        }
        fun text(value: String) = root.addView(TextView(this).apply {
            text = value
            textSize = 16f
            setPadding(0, 0, 0, 24)
        })
        text("CW Messenger — experimental SMS receiver")
        text("Only notifications from your DEFAULT SMS app are eligible. " +
             "Message content is forwarded directly to your paired Pebble, " +
             "not uploaded to a server. Turn this off anytime.")
        val allow = Switch(this).apply {
            text = "Forward incoming SMS to Pebble"
            isChecked = prefs.getBoolean("enabled", false)
            setOnCheckedChangeListener { _, checked ->
                prefs.edit().putBoolean("enabled", checked).apply()
            }
        }
        root.addView(allow)
        root.addView(Button(this).apply {
            text = "Grant Android notification access"
            setOnClickListener {
                startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
            }
        })
        root.addView(Button(this).apply {
            text = "Allow draft handoff notifications"
            setOnClickListener {
                if (Build.VERSION.SDK_INT >= 33 &&
                    checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) !=
                        PackageManager.PERMISSION_GRANTED) {
                    requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 44)
                } else {
                    Toast.makeText(this@MainActivity,
                        "Draft notifications already allowed on this Android version.",
                        Toast.LENGTH_LONG).show()
                }
            }
        })
        root.addView(Button(this).apply {
            text = "Send a harmless test message to Pebble"
            setOnClickListener {
                scope.launch {
                    val connection = DefaultPebbleSender(this@MainActivity)
                    try {
                        connection.startAppOnTheWatch(CwTransport.APP_UUID)
                        kotlinx.coroutines.delay(1000)
                        val result = connection.sendDataToPebble(
                            CwTransport.APP_UUID, mapOf(
                                CwTransport.KEY_SENDER to PebbleDictionaryItem.Text("CW Test"),
                                CwTransport.KEY_BODY to PebbleDictionaryItem.Text("test message")
                            )
                        )
                        Toast.makeText(this@MainActivity,
                            "Pebble test transfer: " + result::class.java.simpleName,
                            Toast.LENGTH_LONG).show()
                    } catch (ex: Exception) {
                        Toast.makeText(this@MainActivity,
                            "Pebble connection failed. Check pairing and companion support.",
                            Toast.LENGTH_LONG).show()
                    } finally {
                        connection.close()
                    }
                }
            }
        })
        text("To send from the watch: double-press Back in SEND CW, then press " +
             "Select to hand the CW draft to this phone. Tap the phone notification, " +
             "choose a recipient and press Send in the messaging app. " +
             "Sending never happens automatically.")
        setContentView(root)
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
