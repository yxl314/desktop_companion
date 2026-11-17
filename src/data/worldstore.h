/**
 * \file src/data/worldstore.h
 * \brief 世界观目录持久化：worlds/&lt;id&gt;/world.md 的增删改查。
 */

#ifndef WORLDSTORE_H
#define WORLDSTORE_H

#include <QFuture>
#include <QList>
#include <QObject>
#include <QString>

#include "domain/character.h"

/**
 * \brief 异步导入世界观的结果。
 */
struct WorldImportResult
{
    bool ok = false;   ///< 是否导入成功。
    QString worldId;   ///< 成功时的世界观 id。
    QString error;     ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 世界观目录 dataDir/worlds/&lt;id&gt;/ 的增删改查。
 *
 * 角色归属由 CharacterStore::setWorldId 写入角色 meta.json，
 * 本类不重复实现角色存储；只做世界观文件的存储。
 */
class WorldStore : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造世界观仓库。
     * @param dataDir 数据根目录；为空时使用 QStandardPaths 的应用数据目录。
     * @param parent 父对象。
     */
    explicit WorldStore(const QString &dataDir = QString(), QObject *parent = nullptr);

    /// 数据根目录。
    QString dataDir() const { return m_dataDir; }
    /// 世界观目录集合 worlds/ 的绝对路径。
    QString worldsDir() const;
    /**
     * \brief 单个世界观目录的绝对路径。
     * @param id 世界观 id。
     * @return worlds/&lt;id&gt; 的绝对路径。
     */
    QString worldDir(const QString &id) const;
    /**
     * \brief 世界观文件 world.md 的路径。
     * @param id 世界观 id。
     * @return world.md 的绝对路径。
     */
    QString worldPath(const QString &id) const;

    /**
     * \brief 扫描世界观目录。
     *
     * 跳过无法映射为安全 id、缺少 world.md 或 world.md 解析失败的目录。
     * @return 世界观列表；目录不存在时返回空列表。
     */
    QList<World> list() const;
    /**
     * \brief 判断世界观是否存在。
     * @param id 世界观 id。
     * @return 世界观目录是否存在。
     */
    bool contains(const QString &id) const;

    /**
     * \brief 读取一个世界观。
     * @param id 世界观 id。
     * @param[out] world 写入读取结果。
     * @param[out] error 非空时写入失败原因。
     * @return 是否成功；文件缺失或解析失败时返回 false。
     */
    bool read(const QString &id, World *world, QString *error = nullptr) const;
    /**
     * \brief 写入一个世界观（目录不存在则创建）。
     * @param id 世界观 id。
     * @param world 世界观数据；name 为空时用 id 兜底。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool write(const QString &id, const World &world, QString *error = nullptr);

    /**
     * \brief 新建世界观（id 由 name 规范化得到）。
     * @param name 显示名。
     * @param[out] newId 非空时写入新世界观 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否创建成功；名字为空、id 冲突时返回 false。
     */
    bool create(const QString &name, QString *newId, QString *error = nullptr);
    /**
     * \brief 导入外部世界观的 .md 文件。
     *
     * id 取文件名规范化，显示名优先 frontmatter name。
     * @param filePath 外部 .md 路径。
     * @param[out] newId 非空时写入新世界观 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否导入成功。
     */
    bool importFrom(const QString &filePath, QString *newId, QString *error = nullptr);
    /**
     * \brief 异步导入外部世界观 .md（G-H-04：耗时操作不冻结 UI）。
     *
     * 文件复制与解析在全局线程池执行；成功后回到对象所属线程发出 changed()。
     * 错误文案与同步版 importFrom 一致。
     * @param filePath 外部 .md 路径。
     * @return 异步结果；成功时包含新世界观 id。
     */
    QFuture<WorldImportResult> importFromAsync(const QString &filePath);
    /**
     * \brief 修改显示名（写回 world.md），目录 id 不变。
     * @param id 世界观 id。
     * @param newName 新显示名。
     * @param[out] error 非空时写入失败原因。
     * @return 是否修改成功。
     */
    bool rename(const QString &id, const QString &newName, QString *error = nullptr);
    /**
     * \brief 递归删除世界观目录。
     * @param id 世界观 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否删除成功。
     */
    bool remove(const QString &id, QString *error = nullptr);

signals:
    /// 世界观数据发生变化（创建、导入、改名、删除、写入后发出）。
    void changed();

private:
    bool writeWorld(const QString &id, const World &world, QString *error) const;
    /// 世界观导入的实际实现（不发出 changed()，供同步 / 异步入口共用）。
    bool importFromImpl(const QString &filePath, QString *newId, QString *error);

    QString m_dataDir; ///< 数据根目录。
};

#endif // WORLDSTORE_H
