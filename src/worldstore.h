#ifndef WORLDSTORE_H
#define WORLDSTORE_H

#include <QList>
#include <QObject>
#include <QString>

/// 一个世界观：dataDir/worlds/<id>/world.md。
struct World
{
    QString id;
    QString name; // 显示名；world.md frontmatter 的 name 字段
    QString content; // 正文，作为提示词的世界观段落
};

/// 世界观目录 dataDir/worlds/<id>/ 的增删改查。
/// 角色归属由 CharacterStore::setWorldId 写入角色 meta.json，本类不重复实现角色存储。
class WorldStore : public QObject
{
    Q_OBJECT

public:
    explicit WorldStore(const QString &dataDir = QString(), QObject *parent = nullptr);

    QString dataDir() const { return m_dataDir; }
    QString worldsDir() const;
    QString worldDir(const QString &id) const;
    QString worldPath(const QString &id) const;

    /// 扫描世界观目录；跳过无法映射为安全 id、缺少 world.md 或 world.md 解析失败的目录。
    QList<World> list() const;
    bool contains(const QString &id) const;

    /// 读取一个世界观；文件缺失或解析失败时返回 false。
    bool read(const QString &id, World *world, QString *error = nullptr) const;
    /// 写入一个世界观（目录不存在则创建）；name 为空时用 id 兜底。
    bool write(const QString &id, const World &world, QString *error = nullptr);

    /// 新建世界观（id 由 name 规范化得到）；名字为空、id 冲突时返回 false。
    bool create(const QString &name, QString *newId, QString *error = nullptr);
    /// 导入外部世界观的 .md（id 取文件名规范化，显示名优先 frontmatter name）。
    bool importFrom(const QString &filePath, QString *newId, QString *error = nullptr);
    /// 修改显示名（写回 world.md），目录 id 不变。
    bool rename(const QString &id, const QString &newName, QString *error = nullptr);
    /// 递归删除世界观目录。
    bool remove(const QString &id, QString *error = nullptr);

signals:
    void changed();

private:
    bool writeWorld(const QString &id, const World &world, QString *error) const;

    QString m_dataDir;
};

#endif // WORLDSTORE_H
