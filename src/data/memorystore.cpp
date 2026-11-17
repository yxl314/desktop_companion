#include "data/memorystore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>

#include <sqlite3.h>

namespace
{
/// 记忆库文案的翻译上下文，对应 translations/desktop_companion_en.ts 的 MemoryStore。
const char *const translationContext = "MemoryStore";

/// 取可翻译文案；无翻译时回退中文原文。
QString trText(const char *text)
{
    return QCoreApplication::translate(translationContext, text);
}

constexpr int schemaVersionCurrent = 1;
constexpr int maxSearchLimit = 1000;
constexpr int maxEntryLimit = 100000;
constexpr int maxKeywords = 48;
constexpr double keywordWeight = 10.0;

const char *const selectColumns = "id, character_id, persona_id, content, pinned, tags,"
                                  " source_message_id, created_at, last_used_at";

QString defaultDbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/memory.db");
}

QByteArray utf8(const QString &text)
{
    return text.toUtf8();
}

QString nowIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString sqliteError(sqlite3 *db)
{
    return db != nullptr ? QString::fromUtf8(sqlite3_errmsg(db)) : trText("数据库不可用");
}

QString columnText(sqlite3_stmt *statement, int column)
{
    return QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, column)));
}

void bindText(sqlite3_stmt *statement, int index, const QString &value)
{
    sqlite3_bind_text(statement, index, utf8(value).constData(), -1, SQLITE_TRANSIENT);
}

/// 保证 sqlite3_stmt 在任意返回路径上都被 finalize。
class Statement
{
public:
    Statement(sqlite3 *db, const char *sql)
    {
        m_ok = sqlite3_prepare_v2(db, sql, -1, &m_statement, nullptr) == SQLITE_OK;
    }
    ~Statement()
    {
        if (m_statement != nullptr)
            sqlite3_finalize(m_statement);
    }
    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;

    bool ok() const { return m_ok; }
    sqlite3_stmt *get() const { return m_statement; }

private:
    sqlite3_stmt *m_statement = nullptr;
    bool m_ok = false;
};

QString tagsToText(const QStringList &tags)
{
    QJsonArray array;
    for (const QString &tag : tags)
        array.append(tag);
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QStringList tagsFromText(const QString &text)
{
    QStringList tags;
    if (text.trimmed().isEmpty())
        return tags;

    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
    if (document.isArray()) {
        const QJsonArray array = document.array();
        for (const QJsonValue value : array) {
            if (value.isString())
                tags.append(value.toString());
        }
        return tags;
    }

    // 兼容手工编辑过的逗号分隔标签。
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty())
            tags.append(tag);
    }
    return tags;
}

MemoryEntry entryFromRow(sqlite3_stmt *statement)
{
    MemoryEntry entry;
    entry.id = sqlite3_column_int64(statement, 0);
    entry.characterId = columnText(statement, 1);
    entry.personaId = columnText(statement, 2);
    entry.content = columnText(statement, 3);
    entry.pinned = sqlite3_column_int(statement, 4) != 0;
    entry.tags = tagsFromText(columnText(statement, 5));
    entry.sourceMessageId = sqlite3_column_int64(statement, 6);
    entry.createdAt = columnText(statement, 7);
    entry.lastUsedAt = columnText(statement, 8);
    return entry;
}

bool isAsciiWord(QChar character)
{
    return character.unicode() < 0x80 && character.isLetterOrNumber();
}

bool isCjkLetter(QChar character)
{
    return character.unicode() > 0x7F && character.isLetter();
}

/// 把查询切成关键词：ASCII 单词整词、中文按二元组（bigram），便于无分词器时命中子串。
QStringList tokenize(const QString &query)
{
    enum class Kind
    {
        None,
        Ascii,
        Cjk
    };

    QStringList tokens;
    QSet<QString> seen;
    QString current;
    Kind kind = Kind::None;

    const auto appendToken = [&tokens, &seen](const QString &token) {
        if (token.isEmpty() || tokens.size() >= maxKeywords || seen.contains(token))
            return;
        seen.insert(token);
        tokens.append(token);
    };

    const auto flush = [&current, &kind, &appendToken]() {
        if (current.isEmpty())
            return;
        if (kind == Kind::Ascii) {
            if (current.size() >= 2)
                appendToken(current.toLower());
        } else if (current.size() == 1) {
            appendToken(current);
        } else {
            for (qsizetype index = 0; index + 1 < current.size(); ++index)
                appendToken(current.mid(index, 2));
        }
        current.clear();
    };

    for (const QChar character : query) {
        const bool asciiWord = isAsciiWord(character);
        const bool cjk = isCjkLetter(character);
        if (!asciiWord && !cjk) {
            flush();
            kind = Kind::None;
            continue;
        }
        const Kind nextKind = asciiWord ? Kind::Ascii : Kind::Cjk;
        if (nextKind != kind) {
            flush();
            kind = nextKind;
        }
        current.append(character);
        if (tokens.size() >= maxKeywords)
            break;
    }
    flush();
    return tokens;
}

int countKeywordHits(const MemoryEntry &entry, const QStringList &tokens)
{
    if (tokens.isEmpty())
        return 0;

    QString haystack = entry.content;
    if (!entry.tags.isEmpty())
        haystack += QLatin1Char(' ') + entry.tags.join(QLatin1Char(' '));
    haystack = haystack.toLower();

    int hits = 0;
    for (const QString &token : tokens) {
        if (haystack.contains(token))
            ++hits;
    }
    return hits;
}

/// 0..1 的新鲜度：刚用过为 1，越久越接近 0。
double freshnessFrom(const MemoryEntry &entry)
{
    const QString stamp = entry.lastUsedAt.isEmpty() ? entry.createdAt : entry.lastUsedAt;
    if (stamp.isEmpty())
        return 0.0;
    const QDateTime time = QDateTime::fromString(stamp, Qt::ISODate);
    if (!time.isValid())
        return 0.0;

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const qint64 ageSeconds = qMax<qint64>(0, time.secsTo(now));
    return 1.0 / (1.0 + (static_cast<double>(ageSeconds) / 86400.0));
}
} // namespace

MemoryStore::MemoryStore(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath.isEmpty() ? defaultDbPath() : dbPath)
{
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    m_ready = openDatabase(m_dbPath) && ensureSchema();
    if (!m_ready) {
        if (m_lastError.isEmpty())
            setErrorText(trText("记忆库初始化失败"));
        if (m_db != nullptr) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }
}

MemoryStore::~MemoryStore()
{
    if (m_db != nullptr)
        sqlite3_close(m_db);
}

void MemoryStore::setMaxEntries(int maxEntries)
{
    m_maxEntries = qBound(1, maxEntries, maxEntryLimit);
}

void MemoryStore::setError(const QString &context)
{
    if (m_db == nullptr) {
        m_lastError = context;
        return;
    }
    m_lastError = context + QStringLiteral(": ") + QString::fromUtf8(sqlite3_errmsg(m_db));
}

void MemoryStore::setErrorText(const QString &text)
{
    m_lastError = text;
}

bool MemoryStore::fail(const QString &message, QString *error)
{
    setErrorText(message);
    if (error != nullptr)
        *error = m_lastError;
    return false;
}

void MemoryStore::recordError(const QString &message, QString *error)
{
    setErrorText(message);
    if (error != nullptr)
        *error = m_lastError;
}

bool MemoryStore::openDatabase(const QString &dbPath)
{
    const QByteArray path = utf8(dbPath);
    if (sqlite3_open_v2(path.constData(), &m_db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        setError(trText("打开记忆库失败"));
        if (m_db != nullptr) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return false;
    }
    sqlite3_busy_timeout(m_db, 3000);
    return exec(QStringLiteral("PRAGMA journal_mode = WAL"));
}

bool MemoryStore::exec(const QString &sql)
{
    if (m_db == nullptr)
        return false;

    char *message = nullptr;
    if (sqlite3_exec(m_db, utf8(sql).constData(), nullptr, nullptr, &message) != SQLITE_OK) {
        setErrorText(message != nullptr ? QString::fromUtf8(message) : trText("SQLite 执行失败"));
        sqlite3_free(message);
        return false;
    }
    return true;
}

bool MemoryStore::readSchemaVersion(int *version)
{
    if (version == nullptr)
        return false;

    Statement statement(m_db, "PRAGMA user_version");
    if (!statement.ok()) {
        setError(trText("读取 schema 版本失败"));
        return false;
    }
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        setError(trText("读取 schema 版本失败"));
        return false;
    }
    *version = sqlite3_column_int(statement.get(), 0);
    return true;
}

bool MemoryStore::ensureSchema()
{
    int version = 0;
    if (!readSchemaVersion(&version))
        return false;
    if (version > schemaVersionCurrent) {
        setErrorText(trText("memory.db schema 版本 %1 高于程序支持的 %2，请升级应用")
                         .arg(version)
                         .arg(schemaVersionCurrent));
        return false;
    }

    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS memories ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "character_id TEXT NOT NULL,"
                       "persona_id TEXT NOT NULL DEFAULT '',"
                       "content TEXT NOT NULL,"
                       "pinned INTEGER NOT NULL DEFAULT 0,"
                       "tags TEXT NOT NULL DEFAULT '[]',"
                       "source_message_id INTEGER NOT NULL DEFAULT -1,"
                       "created_at TEXT NOT NULL,"
                       "last_used_at TEXT NOT NULL)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_memories_partition"
                       " ON memories(character_id, persona_id, pinned)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_memories_last_used"
                       " ON memories(last_used_at)"),
    };
    for (const QString &sql : statements) {
        if (!exec(sql))
            return false;
    }

    if (version < schemaVersionCurrent) {
        if (!exec(QStringLiteral("PRAGMA user_version = %1").arg(schemaVersionCurrent)))
            return false;
    }
    m_schemaVersion = schemaVersionCurrent;
    return true;
}

bool MemoryStore::add(MemoryEntry *entry, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr)
        return fail(trText("记忆库不可用"), error);
    if (entry == nullptr)
        return fail(trText("记忆条目不能为空"), error);

    const QString content = entry->content.trimmed();
    if (content.isEmpty())
        return fail(trText("记忆内容不能为空"), error);
    if (entry->characterId.isEmpty())
        return fail(trText("角色 id 不能为空"), error);
    if (count(entry->characterId, entry->personaId) >= m_maxEntries) {
        return fail(trText("记忆条目已达上限（%1 条），请先清理")
                        .arg(m_maxEntries),
                    error);
    }

    const QString now = nowIso();
    if (entry->createdAt.isEmpty())
        entry->createdAt = now;
    if (entry->lastUsedAt.isEmpty())
        entry->lastUsedAt = entry->createdAt;

    Statement statement(m_db,
                        "INSERT INTO memories(character_id, persona_id, content, pinned, tags,"
                        " source_message_id, created_at, last_used_at)"
                        " VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    if (!statement.ok())
        return fail(trText("新增记忆失败：%1").arg(sqliteError(m_db)), error);

    bindText(statement.get(), 1, entry->characterId);
    bindText(statement.get(), 2, entry->personaId);
    bindText(statement.get(), 3, content);
    sqlite3_bind_int(statement.get(), 4, entry->pinned ? 1 : 0);
    bindText(statement.get(), 5, tagsToText(entry->tags));
    sqlite3_bind_int64(statement.get(), 6, entry->sourceMessageId);
    bindText(statement.get(), 7, entry->createdAt);
    bindText(statement.get(), 8, entry->lastUsedAt);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return fail(trText("新增记忆失败：%1").arg(sqliteError(m_db)), error);

    entry->id = sqlite3_last_insert_rowid(m_db);
    entry->content = content;
    emit changed();
    return true;
}

bool MemoryStore::update(const MemoryEntry &entry, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr)
        return fail(trText("记忆库不可用"), error);
    if (entry.id < 0)
        return fail(trText("记忆 id 无效"), error);

    const QString content = entry.content.trimmed();
    if (content.isEmpty())
        return fail(trText("记忆内容不能为空"), error);

    Statement statement(m_db,
                        "UPDATE memories SET content = ?, pinned = ?, tags = ?, source_message_id = ?"
                        " WHERE id = ?");
    if (!statement.ok())
        return fail(trText("更新记忆失败：%1").arg(sqliteError(m_db)), error);

    bindText(statement.get(), 1, content);
    sqlite3_bind_int(statement.get(), 2, entry.pinned ? 1 : 0);
    bindText(statement.get(), 3, tagsToText(entry.tags));
    sqlite3_bind_int64(statement.get(), 4, entry.sourceMessageId);
    sqlite3_bind_int64(statement.get(), 5, entry.id);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return fail(trText("更新记忆失败：%1").arg(sqliteError(m_db)), error);
    if (sqlite3_changes(m_db) == 0)
        return fail(trText("记忆条目不存在：%1").arg(entry.id), error);

    emit changed();
    return true;
}

bool MemoryStore::remove(qint64 id, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr)
        return fail(trText("记忆库不可用"), error);
    if (id < 0)
        return fail(trText("记忆 id 无效"), error);

    Statement statement(m_db, "DELETE FROM memories WHERE id = ?");
    if (!statement.ok())
        return fail(trText("删除记忆失败：%1").arg(sqliteError(m_db)), error);
    sqlite3_bind_int64(statement.get(), 1, id);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return fail(trText("删除记忆失败：%1").arg(sqliteError(m_db)), error);
    if (sqlite3_changes(m_db) == 0)
        return fail(trText("记忆条目不存在：%1").arg(id), error);

    emit changed();
    return true;
}

bool MemoryStore::setPinned(qint64 id, bool pinned, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr)
        return fail(trText("记忆库不可用"), error);
    if (id < 0)
        return fail(trText("记忆 id 无效"), error);

    Statement statement(m_db, "UPDATE memories SET pinned = ? WHERE id = ?");
    if (!statement.ok())
        return fail(trText("更新置顶失败：%1").arg(sqliteError(m_db)), error);
    sqlite3_bind_int(statement.get(), 1, pinned ? 1 : 0);
    sqlite3_bind_int64(statement.get(), 2, id);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return fail(trText("更新置顶失败：%1").arg(sqliteError(m_db)), error);
    if (sqlite3_changes(m_db) == 0)
        return fail(trText("记忆条目不存在：%1").arg(id), error);

    emit changed();
    return true;
}

QVector<MemoryEntry> MemoryStore::list(const QString &characterId, const QString &personaId) const
{
    QVector<MemoryEntry> entries;
    if (!m_ready || m_db == nullptr)
        return entries;

    const QString sql = QStringLiteral("SELECT %1 FROM memories"
                                       " WHERE character_id = ? AND persona_id = ?"
                                       " ORDER BY pinned DESC, id DESC")
                            .arg(QString::fromLatin1(selectColumns));
    const QByteArray sqlBytes = sql.toUtf8();
    Statement statement(m_db, sqlBytes.constData());
    if (!statement.ok())
        return entries;

    bindText(statement.get(), 1, characterId);
    bindText(statement.get(), 2, personaId);
    while (sqlite3_step(statement.get()) == SQLITE_ROW)
        entries.append(entryFromRow(statement.get()));
    return entries;
}

int MemoryStore::count(const QString &characterId, const QString &personaId) const
{
    if (!m_ready || m_db == nullptr)
        return 0;

    Statement statement(m_db,
                        "SELECT COUNT(*) FROM memories WHERE character_id = ? AND persona_id = ?");
    if (!statement.ok())
        return 0;

    bindText(statement.get(), 1, characterId);
    bindText(statement.get(), 2, personaId);
    if (sqlite3_step(statement.get()) != SQLITE_ROW)
        return 0;
    return sqlite3_column_int(statement.get(), 0);
}

QVector<MemoryEntry> MemoryStore::search(const QString &characterId, const QString &personaId,
                                         const QString &query, int limit, QString *error)
{
    QVector<MemoryEntry> results;
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr) {
        fail(trText("记忆库不可用"), error);
        return results;
    }
    if (characterId.isEmpty()) {
        fail(trText("角色 id 不能为空"), error);
        return results;
    }
    if (limit <= 0)
        limit = defaultSearchLimit;
    limit = qMin(limit, maxSearchLimit);

    const QStringList tokens = tokenize(query);
    QVector<MemoryEntry> pinned;
    QVector<MemoryEntry> candidates;

    const QString sql = QStringLiteral("SELECT %1 FROM memories"
                                       " WHERE character_id = ? AND persona_id = ? ORDER BY id")
                            .arg(QString::fromLatin1(selectColumns));
    const QByteArray sqlBytes = sql.toUtf8();
    Statement statement(m_db, sqlBytes.constData());
    if (!statement.ok()) {
        fail(trText("检索记忆失败：%1").arg(sqliteError(m_db)), error);
        return results;
    }

    bindText(statement.get(), 1, characterId);
    bindText(statement.get(), 2, personaId);
    while (sqlite3_step(statement.get()) == SQLITE_ROW) {
        MemoryEntry entry = entryFromRow(statement.get());
        if (entry.pinned)
            pinned.append(entry);
        else
            candidates.append(entry);
    }

    const auto timestampOf = [](const MemoryEntry &entry) {
        const QString stamp = entry.lastUsedAt.isEmpty() ? entry.createdAt : entry.lastUsedAt;
        const QDateTime time = QDateTime::fromString(stamp, Qt::ISODate);
        return time.isValid() ? time.toSecsSinceEpoch() : static_cast<qint64>(0);
    };

    std::ranges::sort(pinned, [&timestampOf](const MemoryEntry &left, const MemoryEntry &right) {
        const qint64 leftTime = timestampOf(left);
        const qint64 rightTime = timestampOf(right);
        if (leftTime != rightTime)
            return leftTime > rightTime;
        return left.id > right.id;
    });

    struct ScoredMemory
    {
        MemoryEntry entry;
        int hits = 0;
        double score = 0.0;
    };

    QVector<ScoredMemory> scored;
    scored.reserve(candidates.size());
    for (const MemoryEntry &entry : candidates) {
        ScoredMemory item;
        item.entry = entry;
        item.hits = countKeywordHits(entry, tokens);
        item.score = (static_cast<double>(item.hits) * keywordWeight) + freshnessFrom(entry);
        scored.append(item);
    }
    std::ranges::sort(scored, [&timestampOf](const ScoredMemory &left, const ScoredMemory &right) {
        if (left.score > right.score)
            return true;
        if (left.score < right.score)
            return false;
        if (left.hits != right.hits)
            return left.hits > right.hits;
        const qint64 leftTime = timestampOf(left.entry);
        const qint64 rightTime = timestampOf(right.entry);
        if (leftTime != rightTime)
            return leftTime > rightTime;
        return left.entry.id > right.entry.id;
    });

    results.reserve(pinned.size() + qMin<qsizetype>(scored.size(), limit));
    for (const MemoryEntry &entry : pinned)
        results.append(entry);
    for (qsizetype index = 0; index < scored.size() && index < limit; ++index)
        results.append(scored.at(index).entry);
    if (results.isEmpty())
        return results;

    // 检索即算一次使用：批量刷新 last_used_at，失败时仍返回本次结果。
    const QString now = nowIso();
    for (MemoryEntry &entry : results)
        entry.lastUsedAt = now;

    if (!exec(QStringLiteral("BEGIN IMMEDIATE"))) {
        recordError(m_lastError, error);
        return results;
    }

    bool updated = true;
    {
        Statement touch(m_db, "UPDATE memories SET last_used_at = ? WHERE id = ?");
        if (!touch.ok()) {
            updated = false;
        } else {
            for (const MemoryEntry &entry : results) {
                bindText(touch.get(), 1, now);
                sqlite3_bind_int64(touch.get(), 2, entry.id);
                if (sqlite3_step(touch.get()) != SQLITE_DONE) {
                    updated = false;
                    break;
                }
                sqlite3_reset(touch.get());
                sqlite3_clear_bindings(touch.get());
            }
        }
    }

    if (!updated) {
        exec(QStringLiteral("ROLLBACK"));
        recordError(trText("刷新记忆使用时间失败：%1").arg(sqliteError(m_db)), error);
        return results;
    }
    if (!exec(QStringLiteral("COMMIT"))) {
        exec(QStringLiteral("ROLLBACK"));
        recordError(trText("提交记忆使用时间失败：%1").arg(sqliteError(m_db)), error);
    }
    return results;
}

bool MemoryStore::resetForPersona(const QString &characterId, const QString &personaId, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (!m_ready || m_db == nullptr)
        return fail(trText("记忆库不可用"), error);
    if (characterId.isEmpty())
        return fail(trText("角色 id 不能为空"), error);

    Statement statement(m_db, "DELETE FROM memories WHERE character_id = ? AND persona_id = ?");
    if (!statement.ok())
        return fail(trText("清空记忆失败：%1").arg(sqliteError(m_db)), error);
    bindText(statement.get(), 1, characterId);
    bindText(statement.get(), 2, personaId);

    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return fail(trText("清空记忆失败：%1").arg(sqliteError(m_db)), error);

    if (sqlite3_changes(m_db) > 0)
        emit changed();
    return true;
}
