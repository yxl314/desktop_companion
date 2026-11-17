#ifndef CHARACTERSTORE_H
#define CHARACTERSTORE_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>

#include "personafile.h"

/// 角色列表项：目录 id + 显示名 + 世界观归属 + 形象缩略图。
struct Character
{
    QString id;
    QString name;
    QString worldId;
    QString avatarPath; // avatars/ 下首个图片的绝对路径，没有则为空
    bool enabled = true;
};

/// 角色元数据，对应 characters/<id>/meta.json。
/// hitAreas / motions 的结构由命中与动效模块解释，存储层原样透传；
/// anchor 约定为归一化坐标（0~1），存储层不做校验。
struct CharacterMeta
{
    QJsonObject hitAreas;
    QJsonObject motions;
    double scale = 1.0;
    QPointF anchor = QPointF(0.5, 1.0);
    QString worldId; // 归属世界观；空串表示未归属
    bool enabled = true;
};

/// 角色目录 dataDir/characters/<id>/ 的增删改查：
/// card.md（默认人设）、meta.json、avatars/（形象）、personas/*.md（多套人设）。
/// 默认 dataDir 为 QStandardPaths::AppDataLocation；测试可传临时目录。
class CharacterStore : public QObject
{
    Q_OBJECT

public:
    explicit CharacterStore(const QString &dataDir = QString(), QObject *parent = nullptr);

    QString dataDir() const { return m_dataDir; }
    QString charactersDir() const;
    QString characterDir(const QString &id) const;
    QString cardPath(const QString &id) const;
    QString metaPath(const QString &id) const;
    QString avatarsDir(const QString &id) const;
    QString personasDir(const QString &id) const;

    /// 把名称规范成安全的目录 id；无法规范化时返回空串。
    static QString sanitizeId(const QString &raw);

    /// 扫描角色目录；跳过无法映射为安全 id 的目录。名字读 card.md，缺失时用 id。
    QList<Character> list() const;
    bool contains(const QString &id) const;

    /// 创建角色（id 由 name 规范化得到）；名字为空、id 冲突时返回 false。
    bool create(const QString &name, QString *newId, QString *error = nullptr);
    /// 修改显示名（写回 card.md），目录 id 不变。
    bool rename(const QString &id, const QString &newName, QString *error = nullptr);
    /// 递归删除角色目录（含形象与人设）。
    bool remove(const QString &id, QString *error = nullptr);

    /// 读取默认人设 card.md；card.md 缺失或解析失败时返回 false。
    bool card(const QString &id, PersonaData *data, QString *error = nullptr) const;
    /// 写入默认人设 card.md（name 为空时用 id 兜底）。
    bool saveCard(const QString &id, const PersonaData &data, QString *error = nullptr);

    /// 读取 meta.json；文件不存在时返回默认值并成功，JSON 损坏时返回 false。
    bool meta(const QString &id, CharacterMeta *meta, QString *error = nullptr) const;
    /// 写入 meta.json（原子写，保留文件中未识别的字段）。
    bool saveMeta(const QString &id, const CharacterMeta &meta, QString *error = nullptr);

    /// 更新角色归属世界观（worldId 为空表示取消归属）；写入 meta.json。
    bool setWorldId(const QString &id, const QString &newWorldId, QString *error = nullptr);
    /// 读取角色归属世界观；读取失败时返回空串。
    QString worldId(const QString &id) const;

    /// 导入形象文件到 characters/<id>/avatars/（覆盖同名文件，支持 png/jpg/jpeg/webp/gif/bmp/svg）。
    bool importAvatar(const QString &id, const QString &sourcePath, QString *storedPath, QString *error = nullptr);
    /// avatars/ 下全部图片的绝对路径（按文件名排序）。
    QStringList avatars(const QString &id) const;
    /// 首张形象（缩略图）；没有形象时返回空串。
    QString primaryAvatar(const QString &id) const;

    /// 读取全部人设（personas/*.md）；解析失败的条目跳过，id 取文件名。
    QList<PersonaData> personas(const QString &characterId) const;
    /// 读取单套人设。
    bool persona(const QString &characterId, const QString &personaId, PersonaData *data,
                 QString *error = nullptr) const;
    /// 新建或覆盖一套人设；data.id 为空时由 data.name 规范化生成，经 personaId 返回实际 id。
    bool savePersona(const QString &characterId,
                     const PersonaData &data,
                     QString *personaId = nullptr,
                     QString *error = nullptr);
    /// 修改人设显示名（文件与 id 不变）。
    bool renamePersona(const QString &characterId,
                       const QString &personaId,
                       const QString &newName,
                       QString *error = nullptr);
    /// 删除一套人设文件。
    bool removePersona(const QString &characterId, const QString &personaId, QString *error = nullptr);

signals:
    void changed();

private:
    bool writeCard(const QString &id, const PersonaData &data, QString *error) const;
    bool writeMetaJson(const QString &id, const CharacterMeta &meta, QString *error) const;

    QString m_dataDir;
};

#endif // CHARACTERSTORE_H
