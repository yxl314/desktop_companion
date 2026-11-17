#include "chatstore.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

#include <sqlite3.h>

#include <stacklog/stacklog.hpp>

namespace
{
QString defaultDbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/chat.db");
}

QByteArray utf8(const QString &text)
{
    return text.toUtf8();
}
} // namespace

ChatStore::ChatStore(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath.isEmpty() ? defaultDbPath() : dbPath)
{
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    m_ready = openDatabase(m_dbPath) && createSchema();
    if (!m_ready) {
        // 数据库损坏：备份后重建一次，尽量不丢历史；备份保留在现场便于排查。
        STACKLOG_WARN() << "chat store open failed, trying recovery: " << m_lastError.toStdString();
        if (m_db != nullptr) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        const QString backup = m_dbPath + QStringLiteral(".bak");
        QFile::remove(backup);
        if (QFile::exists(m_dbPath) && QFile::rename(m_dbPath, backup)) {
            m_ready = openDatabase(m_dbPath) && createSchema();
            if (m_ready)
                STACKLOG_INFO() << "chat store recovered, original backup: " << backup.toStdString();
        }
        if (!m_ready) {
            STACKLOG_ERROR() << "chat store unavailable: " << m_lastError.toStdString();
            if (m_db != nullptr) {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
        }
    }
}

ChatStore::~ChatStore()
{
    if (m_db != nullptr)
        sqlite3_close(m_db);
}

void ChatStore::setError(const QString &context)
{
    m_lastError = context + QStringLiteral(": ") + QString::fromUtf8(sqlite3_errmsg(m_db));
}

bool ChatStore::openDatabase(const QString &dbPath)
{
    const QByteArray path = utf8(dbPath);
    if (sqlite3_open_v2(path.constData(), &m_db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        setError(QStringLiteral("open"));
        return false;
    }
    sqlite3_busy_timeout(m_db, 3000);
    return exec(QStringLiteral("PRAGMA journal_mode = WAL"))
           && exec(QStringLiteral("PRAGMA foreign_keys = ON"));
}

bool ChatStore::exec(const QString &sql)
{
    if (m_db == nullptr)
        return false;
    char *error = nullptr;
    if (sqlite3_exec(m_db, utf8(sql).constData(), nullptr, nullptr, &error) != SQLITE_OK) {
        m_lastError = QString::fromUtf8(error != nullptr ? error : "sqlite error");
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool ChatStore::createSchema()
{
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS message_feedback ("
                       "message_id INTEGER PRIMARY KEY,"
                       "value INTEGER NOT NULL,"
                       "created_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS conversations ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "persona_id TEXT NOT NULL DEFAULT '',"
                       "created_at TEXT NOT NULL,"
                       "updated_at TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS messages ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "conversation_id INTEGER NOT NULL,"
                       "role TEXT NOT NULL,"
                       "content TEXT NOT NULL,"
                       "created_at TEXT NOT NULL,"
                       "FOREIGN KEY(conversation_id) REFERENCES conversations(id) ON DELETE CASCADE)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_messages_conversation"
                       " ON messages(conversation_id, id)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS meta ("
                       "key TEXT PRIMARY KEY,"
                       "value TEXT NOT NULL)"),
    };

    for (const QString &sql : statements) {
        if (!exec(sql))
            return false;
    }

    // schema 版本：1 = 0.3 起的会话/消息/meta 结构。
    int version = 0;
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db, "PRAGMA user_version", -1, &statement, nullptr) != SQLITE_OK)
        return false;
    if (sqlite3_step(statement) == SQLITE_ROW)
        version = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    if (version > 2) {
        m_lastError = QStringLiteral("chat schema 版本过新：%1").arg(version);
        return false;
    }
    return exec(QStringLiteral("PRAGMA user_version = 2"));
}

bool ChatStore::readMeta(const QString &key, QString *value) const
{
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT value FROM meta WHERE key = ?", -1, &statement, nullptr)
        != SQLITE_OK)
        return false;
    sqlite3_bind_text(statement, 1, utf8(key).constData(), -1, SQLITE_TRANSIENT);
    const bool found = sqlite3_step(statement) == SQLITE_ROW;
    if (found && value != nullptr)
        *value = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 0)));
    sqlite3_finalize(statement);
    return found;
}

bool ChatStore::writeMeta(const QString &key, const QString &value)
{
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "INSERT INTO meta(key, value) VALUES(?, ?)"
                           " ON CONFLICT(key) DO UPDATE SET value = excluded.value",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("meta prepare"));
        return false;
    }
    sqlite3_bind_text(statement, 1, utf8(key).constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, utf8(value).constData(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!ok)
        setError(QStringLiteral("meta write"));
    return ok;
}

qint64 ChatStore::ensureConversation(const QString &personaId)
{
    if (!m_ready)
        return -1;
    if (m_conversationId >= 0 && m_personaId == personaId)
        return m_conversationId;

    if (m_conversationId < 0) {
        QString stored;
        if (readMeta(QStringLiteral("current_conversation"), &stored)) {
            bool ok = false;
            const qint64 id = stored.toLongLong(&ok);
            if (ok) {
                sqlite3_stmt *statement = nullptr;
                if (sqlite3_prepare_v2(m_db, "SELECT persona_id FROM conversations WHERE id = ?",
                                       -1, &statement, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int64(statement, 1, id);
                    if (sqlite3_step(statement) == SQLITE_ROW) {
                        m_conversationId = id;
                        m_personaId = QString::fromUtf8(
                            reinterpret_cast<const char *>(sqlite3_column_text(statement, 0)));
                    }
                    sqlite3_finalize(statement);
                }
            }
        }
    }

    if (m_conversationId >= 0 && m_personaId == personaId)
        return m_conversationId;

    return newConversation(personaId);
}

qint64 ChatStore::newConversation(const QString &personaId)
{
    if (!m_ready)
        return -1;

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "INSERT INTO conversations(persona_id, created_at, updated_at)"
                           " VALUES(?, ?, ?)",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("conversation prepare"));
        return -1;
    }
    sqlite3_bind_text(statement, 1, utf8(personaId).constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, utf8(now).constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, utf8(now).constData(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!ok) {
        setError(QStringLiteral("conversation insert"));
        return -1;
    }

    m_conversationId = sqlite3_last_insert_rowid(m_db);
    m_personaId = personaId;
    writeMeta(QStringLiteral("current_conversation"), QString::number(m_conversationId));
    emit changed();
    return m_conversationId;
}

QVector<StoredMessage> ChatStore::recentMessages(int limit) const
{
    QVector<StoredMessage> messages;
    if (!m_ready || m_conversationId < 0 || m_db == nullptr)
        return messages;

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "SELECT role, content FROM messages"
                           " WHERE conversation_id = ? ORDER BY id DESC LIMIT ?",
                           -1, &statement, nullptr) != SQLITE_OK)
        return messages;
    sqlite3_bind_int64(statement, 1, m_conversationId);
    sqlite3_bind_int(statement, 2, limit);

    while (sqlite3_step(statement) == SQLITE_ROW) {
        StoredMessage message;
        message.role = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 0)));
        message.content = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 1)));
        messages.prepend(message);
    }
    sqlite3_finalize(statement);
    return messages;
}

bool ChatStore::appendMessage(const QString &role, const QString &content, qint64 *outId)
{
    if (!m_ready || m_conversationId < 0 || content.isEmpty())
        return false;

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "INSERT INTO messages(conversation_id, role, content, created_at)"
                           " VALUES(?, ?, ?, ?)",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("message prepare"));
        return false;
    }
    sqlite3_bind_int64(statement, 1, m_conversationId);
    sqlite3_bind_text(statement, 2, utf8(role).constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, utf8(content).constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, utf8(now).constData(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!ok) {
        setError(QStringLiteral("message insert"));
        STACKLOG_ERROR() << "append chat message failed: " << m_lastError.toStdString();
        return false;
    }

    sqlite3_stmt *touch = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE conversations SET updated_at = ? WHERE id = ?",
                           -1, &touch, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(touch, 1, utf8(now).constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(touch, 2, m_conversationId);
        sqlite3_step(touch);
        sqlite3_finalize(touch);
    }
    if (outId != nullptr)
        *outId = sqlite3_last_insert_rowid(m_db);
    emit changed();
    return true;
}

bool ChatStore::setFeedback(qint64 messageId, int value)
{
    if (!m_ready || messageId < 0)
        return false;
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "INSERT INTO message_feedback(message_id, value, created_at)"
                           " VALUES(?, ?, ?)"
                           " ON CONFLICT(message_id) DO UPDATE SET value = excluded.value",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("feedback prepare"));
        return false;
    }
    const QByteArray now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8();
    sqlite3_bind_int64(statement, 1, messageId);
    sqlite3_bind_int(statement, 2, value);
    sqlite3_bind_text(statement, 3, now.constData(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    if (!ok)
        setError(QStringLiteral("feedback"));
    sqlite3_finalize(statement);
    return ok;
}

void ChatStore::feedbackStats(int *likes, int *dislikes) const
{
    if (likes != nullptr)
        *likes = 0;
    if (dislikes != nullptr)
        *dislikes = 0;
    if (!m_ready || m_conversationId < 0)
        return;
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "SELECT IFNULL(SUM(CASE WHEN f.value > 0 THEN 1 ELSE 0 END), 0),"
                           " IFNULL(SUM(CASE WHEN f.value < 0 THEN 1 ELSE 0 END), 0)"
                           " FROM messages m JOIN message_feedback f ON f.message_id = m.id"
                           " WHERE m.conversation_id = ?",
                           -1, &statement, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_int64(statement, 1, m_conversationId);
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if (likes != nullptr)
            *likes = sqlite3_column_int(statement, 0);
        if (dislikes != nullptr)
            *dislikes = sqlite3_column_int(statement, 1);
    }
    sqlite3_finalize(statement);
}

int ChatStore::truncateAfter(qint64 messageId)
{
    if (!m_ready || m_conversationId < 0)
        return -1;

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db,
                           "DELETE FROM messages WHERE conversation_id = ? AND id > ?",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("truncate prepare"));
        return -1;
    }
    sqlite3_bind_int64(statement, 1, m_conversationId);
    sqlite3_bind_int64(statement, 2, messageId);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    int removed = -1;
    if (ok) {
        removed = sqlite3_changes(m_db);
        emit changed();
    } else {
        setError(QStringLiteral("truncate"));
    }
    sqlite3_finalize(statement);
    return removed;
}

bool ChatStore::clearCurrentMessages()
{
    if (!m_ready || m_conversationId < 0)
        return false;

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM messages WHERE conversation_id = ?",
                           -1, &statement, nullptr) != SQLITE_OK) {
        setError(QStringLiteral("clear prepare"));
        return false;
    }
    sqlite3_bind_int64(statement, 1, m_conversationId);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!ok) {
        setError(QStringLiteral("clear"));
        return false;
    }
    emit changed();
    return true;
}
