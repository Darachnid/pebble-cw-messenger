package dev.darachnid.cwmessenger

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

/** Local inbox from notification posts only (no READ_SMS). */
internal class CwMessageStore(context: Context) :
    SQLiteOpenHelper(context.applicationContext, "cw_messages.db", null, 1) {

    data class Message(
        val id: Long,
        val threadKey: String,
        val sender: String,
        val body: String,
        val receivedAt: Long,
        val notificationKey: String?,
    )

    data class ThreadSummary(
        val threadKey: String,
        val sender: String,
        val preview: String,
        val receivedAt: Long,
    )

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(
            """
            CREATE TABLE messages (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              thread_key TEXT NOT NULL,
              sender TEXT NOT NULL,
              body TEXT NOT NULL,
              received_at INTEGER NOT NULL,
              notification_key TEXT,
              signature INTEGER NOT NULL
            )
            """.trimIndent()
        )
        db.execSQL("CREATE INDEX idx_thread ON messages(thread_key, received_at DESC)")
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        db.execSQL("DROP TABLE IF EXISTS messages")
        onCreate(db)
    }

    fun insert(
        threadKey: String,
        sender: String,
        body: String,
        receivedAt: Long,
        notificationKey: String?,
        signature: Int,
    ): Long {
        val db = writableDatabase
        db.delete(
            "messages",
            "signature = ?",
            arrayOf(signature.toString()),
        )
        val id = db.insert(
            "messages",
            null,
            ContentValues().apply {
                put("thread_key", threadKey)
                put("sender", sender)
                put("body", body)
                put("received_at", receivedAt)
                put("notification_key", notificationKey)
                put("signature", signature)
            },
        )
        prune(db)
        return id
    }

    private fun prune(db: SQLiteDatabase) {
        val count = db.compileStatement("SELECT COUNT(*) FROM messages").simpleQueryForLong()
        if (count > 200) {
            db.execSQL(
                "DELETE FROM messages WHERE id NOT IN " +
                    "(SELECT id FROM messages ORDER BY received_at DESC LIMIT 200)"
            )
        }
    }

    fun listThreads(limit: Int = 50): List<ThreadSummary> {
        val db = readableDatabase
        val out = mutableListOf<ThreadSummary>()
        db.rawQuery(
            """
            SELECT thread_key, sender, body, MAX(received_at) AS ts
            FROM messages
            GROUP BY thread_key
            ORDER BY ts DESC
            LIMIT ?
            """.trimIndent(),
            arrayOf(limit.toString()),
        ).use { c ->
            while (c.moveToNext()) {
                out += ThreadSummary(
                    c.getString(0),
                    c.getString(1),
                    c.getString(2),
                    c.getLong(3),
                )
            }
        }
        return out
    }

    fun messagesForThread(threadKey: String): List<Message> {
        val db = readableDatabase
        val out = mutableListOf<Message>()
        db.query(
            "messages",
            arrayOf("id", "thread_key", "sender", "body", "received_at", "notification_key"),
            "thread_key = ?",
            arrayOf(threadKey),
            null,
            null,
            "received_at ASC",
        ).use { c ->
            while (c.moveToNext()) {
                out += Message(
                    c.getLong(0),
                    c.getString(1),
                    c.getString(2),
                    c.getString(3),
                    c.getLong(4),
                    c.getString(5),
                )
            }
        }
        return out
    }

    fun latestNotificationKey(threadKey: String): String? {
        val db = readableDatabase
        db.query(
            "messages",
            arrayOf("notification_key"),
            "thread_key = ? AND notification_key IS NOT NULL",
            arrayOf(threadKey),
            null,
            null,
            "received_at DESC",
            "1",
        ).use { c ->
            if (c.moveToFirst()) return c.getString(0)
        }
        return null
    }
}
