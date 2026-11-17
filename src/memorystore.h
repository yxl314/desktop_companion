#ifndef MEMORYSTORE_H
#define MEMORYSTORE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct sqlite3;

/// 一条结构化记忆；character/persona 决定分区，pinned 条目每轮必注入。
struct MemoryEntry
{
    qint64 id = -1;
    QString characterId;
    QString personaId;
    QString content;
    bool pinned = false;
    QStringList tags;
    qint64 sourceMessageId = -1;
    QString createdAt;
    QString lastUsedAt;
};

/// SQLite 记忆库：单连接、WAL、按 (characterId, personaId) 精确分区。
///
/// 检索策略：置顶（pin）条目全部返回；其余条目按「关键词命中数 × 权重 + last_used_at 新鲜度」
/// 排序取前 N 条（默认 5）。返回的条目会刷新 last_used_at，便于下次排序反映使用频率。
class MemoryStore : public QObject
{
    Q_OBJECT

public:
    static constexpr int defaultMaxEntries = 200;
    static constexpr int defaultSearchLimit = 5;

    /// dbPath 为空时使用数据目录下的 memory.db；测试可传临时库路径。
    explicit MemoryStore(const QString &dbPath = QString(), QObject *parent = nullptr);
    ~MemoryStore() override;

    bool ready() const { return m_ready; }
    QString dbPath() const { return m_dbPath; }
    QString lastError() const { return m_lastError; }
    int schemaVersion() const { return m_schemaVersion; }

    /// 每个分区的条目上限，超出后 add 返回失败原因；默认 defaultMaxEntries。
    void setMaxEntries(int maxEntries);
    int maxEntries() const { return m_maxEntries; }

    /// 新增记忆；成功后 entry->id 为数据库行 id，created_at/last_used_at 为空时自动填当前时间。
    bool add(MemoryEntry *entry, QString *error = nullptr);

    /// 更新正文、pin、标签与来源消息（分区与时间戳不可改）。
    bool update(const MemoryEntry &entry, QString *error = nullptr);
    bool remove(qint64 id, QString *error = nullptr);
    bool setPinned(qint64 id, bool pinned, QString *error = nullptr);

    /// 分区内全部条目：置顶在前，其余按创建时间倒序。
    QVector<MemoryEntry> list(const QString &characterId, const QString &personaId) const;

    /// 分区内检索；limit ≤ 0 时使用 defaultSearchLimit。检索后刷新返回条目的 last_used_at。
    QVector<MemoryEntry> search(const QString &characterId, const QString &personaId,
                                const QString &query, int limit = defaultSearchLimit,
                                QString *error = nullptr);

    /// 分区内条目数。
    int count(const QString &characterId, const QString &personaId) const;

    /// 清空某个分区的记忆（回溯或重启对话时使用）；分区为空也算成功。
    bool resetForPersona(const QString &characterId, const QString &personaId, QString *error = nullptr);

signals:
    void changed();

private:
    bool openDatabase(const QString &dbPath);
    bool ensureSchema();
    bool readSchemaVersion(int *version);
    bool exec(const QString &sql);
    bool fail(const QString &message, QString *error);
    void recordError(const QString &message, QString *error);
    void setError(const QString &context);
    void setErrorText(const QString &text);

    sqlite3 *m_db = nullptr;
    QString m_dbPath;
    QString m_lastError;
    int m_maxEntries = defaultMaxEntries;
    int m_schemaVersion = 0;
    bool m_ready = false;
};

#endif // MEMORYSTORE_H
