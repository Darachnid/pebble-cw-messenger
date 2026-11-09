package dev.darachnid.cwmessenger

import android.content.Context
import android.content.SharedPreferences
import android.text.Editable
import android.text.InputFilter
import android.text.TextWatcher
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextView

internal object CwSettingsUi {
    fun sectionTitle(ctx: Context, layout: LinearLayout, text: String, sizeSp: Float = 20f) {
        layout.addView(TextView(ctx).apply {
            this.text = text
            textSize = sizeSp
            setPadding(0, 24, 0, 8)
        })
    }

    fun bodyText(ctx: Context, layout: LinearLayout, text: String) {
        layout.addView(TextView(ctx).apply {
            this.text = text
            textSize = 15f
            setPadding(0, 8, 0, 12)
        })
    }

    fun actionButton(ctx: Context, layout: LinearLayout, label: String, listener: () -> Unit) {
        layout.addView(Button(ctx).apply {
            text = label
            setOnClickListener { listener() }
        })
    }

    fun toggle(
        ctx: Context,
        layout: LinearLayout,
        settings: SharedPreferences,
        label: String,
        name: String,
        default: Boolean = true,
        onChange: (() -> Unit)? = null,
    ) {
        layout.addView(Switch(ctx).apply {
            text = label
            isChecked = settings.getBoolean(name, default)
            setOnCheckedChangeListener { _, state ->
                settings.edit().putBoolean(name, state).apply()
                CwPreferences.markConfigured(ctx)
                onChange?.invoke()
            }
        })
    }

    fun slider(
        ctx: Context,
        layout: LinearLayout,
        settings: SharedPreferences,
        label: String,
        name: String,
        low: Int,
        high: Int,
        step: Int,
        fallback: Int,
        onChange: (() -> Unit)? = null,
    ) {
        val labelView = TextView(ctx).apply {
            textSize = 15f
            setPadding(0, 16, 0, 0)
        }
        layout.addView(labelView)
        val bar = SeekBar(ctx).apply {
            max = (high - low) / step
            progress = ((settings.getInt(name, fallback) - low) / step).coerceIn(0, max)
        }
        labelView.text = "$label: ${low + bar.progress * step}"
        bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (!fromUser) return
                val v = low + progress * step
                labelView.text = "$label: $v"
                settings.edit().putInt(name, v).apply()
                CwPreferences.markConfigured(ctx)
                onChange?.invoke()
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        layout.addView(bar)
    }

    fun textInput(
        ctx: Context,
        layout: LinearLayout,
        settings: SharedPreferences,
        label: String,
        name: String,
        fallback: String,
        limit: Int,
    ) {
        bodyText(ctx, layout, label)
        layout.addView(EditText(ctx).apply {
            setSingleLine(true)
            filters = arrayOf(InputFilter.LengthFilter(limit))
            setText(settings.getString(name, fallback) ?: fallback)
            addTextChangedListener(object : TextWatcher {
                override fun beforeTextChanged(s: CharSequence?, a: Int, c: Int, f: Int) {}
                override fun onTextChanged(s: CharSequence?, a: Int, b: Int, count: Int) {
                    settings.edit().putString(name, s?.toString() ?: "").apply()
                    CwPreferences.markConfigured(ctx)
                }
                override fun afterTextChanged(s: Editable?) {}
            })
        })
    }
}
