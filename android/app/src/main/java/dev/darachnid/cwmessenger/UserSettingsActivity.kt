package dev.darachnid.cwmessenger

import android.app.Activity
import android.os.Bundle
import android.widget.LinearLayout
import android.widget.ScrollView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

class UserSettingsActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val settings = CwPreferences.prefs(this)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 36, 32, 56)
        }
        setContentView(ScrollView(this).apply { addView(layout) })

        CwSettingsUi.sectionTitle(this, layout, "User settings", 22f)
        CwSettingsUi.bodyText(
            this,
            layout,
            "Saved here and sent to the watch on the next SMS or when you push below. " +
                "Round Pebble watches use vibration only for receive/send audio.",
        )

        CwSettingsUi.sectionTitle(this, layout, "Playback")
        CwSettingsUi.slider(this, layout, settings, "Speed (WPM)", "wpm", 5, 40, 1, 20)
        CwSettingsUi.slider(this, layout, settings, "Tone (Hz)", "tone_hz", 200, 1500, 25, 600)
        CwSettingsUi.slider(this, layout, settings, "Volume", "volume", 0, 100, 5, 60)
        CwSettingsUi.slider(this, layout, settings, "Haptic alignment (ms)", "sync_ms", -300, 400, 10, 80)
        CwSettingsUi.toggle(this, layout, settings, "Receive sound", "rx_audio")
        CwSettingsUi.toggle(this, layout, settings, "Receive vibrations", "rx_vibe")

        CwSettingsUi.sectionTitle(this, layout, "Keyer and paddles")
        CwSettingsUi.toggle(this, layout, settings, "Send sidetone", "tx_audio")
        CwSettingsUi.toggle(this, layout, settings, "Send vibrations", "tx_vibe")
        CwSettingsUi.toggle(this, layout, settings, "Reverse Up/Down", "reverse", false)
        CwSettingsUi.slider(
            this, layout, settings, "Straight-key dah threshold (%)",
            "threshold", 125, 300, 25, 200,
        )

        CwSettingsUi.sectionTitle(this, layout, "Demo and practice")
        CwSettingsUi.textInput(this, layout, settings, "Sample sender", "demo_sender", "Ben", 31)
        CwSettingsUi.textInput(
            this, layout, settings, "Sample message", "demo_message",
            "test", 120,
        )

        CwSettingsUi.sectionTitle(this, layout, "App behavior")
        CwSettingsUi.toggle(
            this, layout, settings,
            "Autoplay Morse on new SMS", "autoplay", default = true,
        )
        CwSettingsUi.toggle(
            this, layout, settings,
            "Play demo when opening Receive", "demo_autostart", default = true,
        )
        CwSettingsUi.toggle(
            this, layout, settings,
            "Open Send keyer after welcome (else Receive)", "start_mode_send",
            default = false,
        )
        CwSettingsUi.toggle(
            this, layout, settings,
            "Keep message history in app", "keep_history", default = true,
        )

        CwSettingsUi.sectionTitle(this, layout, "Advanced")
        CwSettingsUi.textInput(this, layout, settings, "Ollama URL", "llm_url", "", 180)
        CwSettingsUi.textInput(this, layout, settings, "Ollama model", "llm_model", "", 80)
        CwSettingsUi.bodyText(
            this,
            layout,
            "Optional local spellcheck only. Draft text never leaves devices you configure.",
        )

        CwSettingsUi.actionButton(this, layout, "Show welcome tour on watch") {
            scope.launch {
                val ok = CwDelivery.pushShowWelcome(this@UserSettingsActivity)
                android.widget.Toast.makeText(
                    this@UserSettingsActivity,
                    if (ok) "Welcome tour requested on watch" else "Could not reach watch",
                    android.widget.Toast.LENGTH_SHORT,
                ).show()
            }
        }

        CwSettingsUi.actionButton(this, layout, "Push settings to watch now") {
            scope.launch {
                val ok = CwDelivery.pushSettings(this@UserSettingsActivity)
                android.widget.Toast.makeText(
                    this@UserSettingsActivity,
                    if (ok) "Settings sent to watch" else "Could not reach watch",
                    android.widget.Toast.LENGTH_SHORT,
                ).show()
            }
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }
}
