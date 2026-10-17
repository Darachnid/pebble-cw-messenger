package dev.darachnid.cwmessenger

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.text.InputFilter
import android.text.TextWatcher
import android.text.Editable
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * The native Android companion replaces Clay: native + PKJS cannot both route
 * CW Messenger's AppMessages. CW preferences are applied to the watch with
 * the next incoming SMS; edits never automatically open the watchapp.
 */
class MainActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val privacy = getSharedPreferences("privacy", MODE_PRIVATE)
        val settings = CwPreferences.prefs(this)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 36, 32, 56)
        }
        val scroll = ScrollView(this).apply { addView(layout) }
        setContentView(scroll)

        fun text(value: String, size: Float = 15f) {
            layout.addView(TextView(this).apply {
                this.text = value
                textSize = size
                setPadding(0, 10, 0, 14)
            })
        }
        fun toggle(label: String, name: String, default: Boolean = true,
                   isPrivacy: Boolean = false) {
            val value = if (isPrivacy) privacy.getBoolean(name, default)
                else settings.getBoolean(name, default)
            layout.addView(Switch(this).apply {
                this.text = label
                isChecked = value
                setOnCheckedChangeListener { _, state ->
                    if (isPrivacy) privacy.edit().putBoolean(name, state).apply()
                    else {
                        settings.edit().putBoolean(name, state).apply()
                        CwPreferences.markConfigured(this@MainActivity)
                    }
                }
            })
        }
        fun slider(label: String, name: String, low: Int, high: Int,
                   step: Int, fallback: Int) {
            val labelView = TextView(this).apply {
                textSize = 15f
                setPadding(0, 16, 0, 0)
            }
            layout.addView(labelView)
            val bar = SeekBar(this).apply {
                max = (high - low) / step
                progress = ((settings.getInt(name, fallback) - low) / step)
                    .coerceIn(0, max)
            }
            labelView.text = "$label: ${low + bar.progress * step}"
            bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(
                    seekBar: SeekBar?, progress: Int, fromUser: Boolean
                ) {
                    if (!fromUser) return
                    val value = low + progress * step
                    labelView.text = "$label: $value"
                    settings.edit().putInt(name, value).apply()
                    CwPreferences.markConfigured(this@MainActivity)
                }
                override fun onStartTrackingTouch(seekBar: SeekBar?) {}
                override fun onStopTrackingTouch(seekBar: SeekBar?) {}
            })
            layout.addView(bar)
        }
        fun input(label: String, name: String, fallback: String, limit: Int) {
            text(label)
            layout.addView(EditText(this).apply {
                setSingleLine(true)
                filters = arrayOf(InputFilter.LengthFilter(limit))
                setText(settings.getString(name, fallback) ?: fallback)
                addTextChangedListener(object : TextWatcher {
                    override fun beforeTextChanged(s: CharSequence?, a: Int, c: Int, f: Int) {}
                    override fun onTextChanged(s: CharSequence?, a: Int, b: Int, count: Int) {
                        settings.edit().putString(name, s?.toString() ?: "").apply()
                        CwPreferences.markConfigured(this@MainActivity)
                    }
                    override fun afterTextChanged(s: Editable?) {}
                })
            })
        }
        fun action(label: String, listener: () -> Unit) {
            layout.addView(Button(this).apply {
                text = label
                setOnClickListener { listener() }
            })
        }

        text("CW Messenger", 24f)
        text("Background SMS → Morse on Pebble Time 2. A new SMS from the phone's default messaging app opens Receive. The watch automatically closes after playback unless you interact with it.")
        toggle("Enable background incoming SMS", "enabled", false, true)
        action("Grant notification access") {
            startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
        }
        text("Android notification access lets this service see notification content. We filter messages from the default SMS app and do not upload them to a server. The service requires explicit Android permission and the enabled switch.")

        text("CW playback", 20f)
        slider("Speed (WPM)", "wpm", 5, 40, 1, 20)
        slider("Square-wave pitch (Hz)", "tone_hz", 200, 1500, 25, 600)
        slider("Speaker volume", "volume", 0, 100, 5, 60)
        slider("Haptic alignment (ms)", "sync_ms", -300, 400, 10, 80)
        toggle("Receive sound", "rx_audio")
        toggle("Receive vibrations", "rx_vibe")

        text("Sending / keyer", 20f)
        toggle("Send sidetone", "tx_audio")
        toggle("Send vibrations", "tx_vibe")
        toggle("Reverse Up/Down dits and dahs", "reverse", false)
        slider("Straight-key dah threshold (% of dit)", "threshold", 125, 300, 25, 200)

        text("Optional private local language model", 20f)
        text("Use an Ollama server on your own device, local network or Tailnet. The original draft is used when the model is not configured or unavailable. This feature never contacts a public AI service.")
        input("Local Ollama URL (e.g. http://100.x.x.x:11434)", "llm_url", "", 180)
        input("Ollama model (e.g. llama3.2:3b)", "llm_model", "", 80)
        text("Only explicitly configured servers receive CW draft text. Review every proposed correction on your watch before sending.")
                text("Optional manual Receive demo", 20f)
        input("Sample sender", "demo_sender", "Ben", 31)
        input("Sample message", "demo_message", "good night bens best", 120)
        text("Changes save immediately on Android and are applied to the watch with the next delivered SMS. Previously calibrated watch settings are preserved until you edit them.")

        action("Test background launch and Morse") {
            scope.launch {
                val delivered = CwDelivery.showMessage(
                    this@MainActivity, "CW Test", "good night bens best")
                Toast.makeText(this@MainActivity,
                    if (delivered) "CW test delivered to Pebble"
                    else "CW test not delivered. Check Pebble pairing / watch app installation.",
                    Toast.LENGTH_LONG).show()
            }
        }
        action("Allow outgoing CW draft notifications") {
            if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) !=
                    PackageManager.PERMISSION_GRANTED) {
                requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 44)
            } else {
                Toast.makeText(this, "Draft notification permission already available.",
                    Toast.LENGTH_SHORT).show()
            }
        }
        text("Outgoing CW: in Send mode, triple-press Back to prepare your draft. After optional local spellcheck, the watch shows a corrected proposal. Press Select on the watch to reply through Google Messages' available notification Reply action, or Back to cancel. If that action is unavailable, the app will not send anything.")
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
