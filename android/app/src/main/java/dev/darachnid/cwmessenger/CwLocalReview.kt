package dev.darachnid.cwmessenger

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

/**
 * Optional Ollama-compatible local server (e.g. an Ollama model on the same
 * phone, local network, or Tailscale). No hosted inference and no cloud API.
 * Disabled until the user explicitly supplies an endpoint and model.
 */
internal object CwLocalReview {
    suspend fun spellcheck(context: Context, draft: String): String =
        withContext(Dispatchers.IO) {
            val settings = CwPreferences.prefs(context)
            val base = settings.getString("llm_url", "")?.trim().orEmpty()
            val model = settings.getString("llm_model", "")?.trim().orEmpty()
            if (base.isBlank() || model.isBlank()) return@withContext draft
            if (!base.startsWith("http://") && !base.startsWith("https://"))
                return@withContext draft
            var connection: HttpURLConnection? = null
            try {
                val endpoint = URL(base.trimEnd('/') + "/api/generate")
                connection = endpoint.openConnection() as HttpURLConnection
                connection.requestMethod = "POST"
                connection.connectTimeout = 4000
                connection.readTimeout = 14000
                connection.doOutput = true
                connection.setRequestProperty("Content-Type", "application/json; charset=utf-8")
                val body = JSONObject()
                    .put("model", model)
                    .put("stream", false)
                    .put("prompt", """
                        Correct only obvious spelling or spacing mistakes in the following
                        Morse-decoded SMS draft. Preserve the intended meaning, wording,
                        names, radio shorthand, numbers and punctuation. Do not invent
                        information. Respond with the corrected SMS only, no explanation.
                        Input: ${draft.take(150)}
                    """.trimIndent())
                    .toString()
                connection.outputStream.use { it.write(body.toByteArray(Charsets.UTF_8)) }
                if (connection.responseCode !in 200..299) return@withContext draft
                val response = connection.inputStream.bufferedReader().use { it.readText() }
                val text = JSONObject(response).optString("response", "").trim()
                    .trim('"', '\'', ' ', '\n')
                // Never allow a blank or wildly expanded hallucinated suggestion.
                if (text.isBlank() || text.length > 150 || text.contains('\n')) draft
                else text
            } catch (_: Exception) {
                // The watch still shows the original draft when Ollama is offline.
                draft
            } finally {
                connection?.disconnect()
            }
        }
}
