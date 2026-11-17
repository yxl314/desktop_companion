/**
 * \file src/data/memorystore.h
 * \brief 记忆持久化：SQLite 记忆库，按 (角色, 人设) 分区检索。
 */

#ifndef MEMORYSTORE_H
#define MEMORYSTORE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "domain/memoryentry.h"

struct sqlite3;

/**
 * \brief SQLite 记忆库：单连接、WAL、按 (characterId, personaId) 精确分区。
 *
 * 检索策略：置顶（pin）条目全部返回；其余条目按「关键词命中数 × 权重 +
 * last_used_at 新鲜度」排序取前 N 条（默认 5）。返回的条目会刷新
 * last_used_at，便于下次排序反映使用频率。
 */
class MemoryStore : public QObject
{
    Q_OBJECT

public:
    static constexpr int defaultMaxEntries = 200; ///< 每个分区的默认条目上限。
    static constexpr int defaultSearchLimit = 5;  ///< 检索时默认返回条数。

    /**
     * \brief 构造记忆库并打开数据库。
     * @param dbPath 数据库路径；为空时使用数据目录下的 memory.db。
     * @param parent 父对象。
     */
    explicit MemoryStore(const QString &dbPath = QString(), QObject *parent = nullptr);
    ~MemoryStore() override;

    /// 数据库是否已就绪。
    bool ready() const { return m_ready; }
    /// 数据库文件路径。
    QString dbPath() const { return m_dbPath; }
    /// 最近一次失败原因。
    QString lastError() const { return m_lastError; }
    /// 数据库 schema 版本。
    int schemaVersion() const { return m_schemaVersion; }

    /**
     * \brief 设置每个分区的条目上限，超出后 add() 返回失败原因。
     * @param maxEntries 条目上限。
     */
    void setMaxEntries(int maxEntries);
    /// 当前分区条目上限。
    int maxEntries() const { return m_maxEntries; }

    /**
     * \brief 新增一条记忆。
     * @param[in,out] entry 记忆条目；成功后 id 为数据库行 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否新增成功；created_at/last_used_at 为空时自动填当前时间。
     */
    bool add(MemoryEntry *entry, QString *error = nullptr);

    /**
     * \brief 更新正文、pin、标签与来源消息（分区与时间戳不可改）。
     * @param entry 更新后的条目。
     * @param[out] error 非空时写入失败原因。
     * @return 是否更新成功。
     */
    bool update(const MemoryEntry &entry, QString *error = nullptr);
    /**
     * \brief 删除一条记忆。
     * @param id 记忆 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否删除成功。
     */
    bool remove(qint64 id, QString *error = nullptr);
    /**
     * \brief 设置一条记忆的置顶状态。
     * @param id 记忆 id。
     * @param pinned 是否置顶。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool setPinned(qint64 id, bool pinned, QString *error = nullptr);

    /**
     * \brief 读取分区内全部条目。
     * @param characterId 角色 id（分区键）。
     * @param personaId 人设 id（分区键）。
     * @return 置顶在前，其余按创建时间倒序。
     */
    QVector<MemoryEntry> list(const QString &characterId, const QString &personaId) const;

    /**
     * \brief 分区内检索记忆。
     *
     * limit ≤ 0 时使用 defaultSearchLimit；检索后刷新返回条目的 last_used_at。
     * @param characterId 角色 id（分区键）。
     * @param personaId 人设 id（分区键）。
     * @param query 查询关键词。
     * @param limit 返回条数上限。
     * @param[out] error 非空时写入失败原因。
     * @return 按相关度排序的条目列表。
     */
    QVector<MemoryEntry> search(const QString &characterId, const QString &personaId,
                                const QString &query, int limit = defaultSearchLimit,
                                QString *error = nullptr);

    /**
     * \brief 统计分区内条目数。
     * @param characterId 角色 id（分区键）。
     * @param personaId 人设 id（分区键）。
     * @return 条目数。
     */
    int count(const QString &characterId, const QString &personaId) const;

    /**
     * \brief 清空某个分区的记忆（回溯或重启对话时使用）。
     * @param characterId 角色 id（分区键）。
     * @param personaId 人设 id（分区键）。
     * @param[out] error 非空时写入失败原因。
     * @return 是否清空成功；分区为空也算成功。
     */
    bool resetForPersona(const QString &characterId, const QString &personaId, QString *error = nullptr);

signals:
    /// 记忆数据发生变化（新增、更新、删除、清空后发出）。
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

    sqlite3 *m_db = nullptr;   ///< SQLite 连接句柄。
    QString m_dbPath;          ///< 数据库文件路径。
    QString m_lastError;       ///< 最近一次失败原因。
    int m_maxEntries = defaultMaxEntries; ///< 每个分区的条目上限。
    int m_schemaVersion = 0;   ///< 数据库 schema 版本。
    bool m_ready = false;      ///< 数据库是否就绪。
};

#endif // MEMORYSTORE_H
