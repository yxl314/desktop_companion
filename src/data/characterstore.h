/**
 * \file src/data/characterstore.h
 * \brief 角色目录持久化：characters/&lt;id&gt;/ 下的人设、元数据、形象与人设集合。
 */

#ifndef CHARACTERSTORE_H
#define CHARACTERSTORE_H

#include <QFuture>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "domain/character.h"

/**
 * \brief 异步导入形象的结果。
 */
struct AvatarImportResult
{
    bool ok = false;      ///< 是否导入成功。
    QString storedPath;   ///< 导入后的形象绝对路径（成功时有效）。
    QString error;        ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 角色目录 dataDir/characters/&lt;id&gt;/ 的增删改查。
 *
 * 目录结构：card.md（默认人设）、meta.json、avatars/（形象）、
 * personas/*.md（多套人设）。
 * 默认 dataDir 为 QStandardPaths::AppDataLocation；测试可传临时目录。
 * 本类只做存储，不包含角色交互规则。
 */
class CharacterStore : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造角色仓库。
     * @param dataDir 数据根目录；为空时使用 QStandardPaths 的应用数据目录。
     * @param parent 父对象。
     */
    explicit CharacterStore(const QString &dataDir = QString(), QObject *parent = nullptr);

    /// 数据根目录。
    QString dataDir() const { return m_dataDir; }
    /// 角色目录集合 characters/ 的绝对路径。
    QString charactersDir() const;
    /**
     * \brief 单个角色目录的绝对路径。
     * @param id 角色 id。
     * @return characters/&lt;id&gt; 的绝对路径。
     */
    QString characterDir(const QString &id) const;
    /**
     * \brief 默认人设 card.md 的路径。
     * @param id 角色 id。
     * @return card.md 的绝对路径。
     */
    QString cardPath(const QString &id) const;
    /**
     * \brief 角色元数据 meta.json 的路径。
     * @param id 角色 id。
     * @return meta.json 的绝对路径。
     */
    QString metaPath(const QString &id) const;
    /**
     * \brief 形象目录 avatars/ 的路径。
     * @param id 角色 id。
     * @return avatars/ 的绝对路径。
     */
    QString avatarsDir(const QString &id) const;
    /**
     * \brief 多套人设目录 personas/ 的路径。
     * @param id 角色 id。
     * @return personas/ 的绝对路径。
     */
    QString personasDir(const QString &id) const;

    /**
     * \brief 把名称规范成安全的目录 id。
     * @param raw 原始名称。
     * @return 安全 id；无法规范化时返回空串。
     */
    static QString sanitizeId(const QString &raw);

    /**
     * \brief 扫描角色目录。
     *
     * 跳过无法映射为安全 id 的目录；名字读 card.md，缺失时用 id。
     * @return 角色列表项；目录不存在时返回空列表。
     */
    QList<Character> list() const;
    /**
     * \brief 判断角色是否存在。
     * @param id 角色 id。
     * @return 角色目录是否存在。
     */
    bool contains(const QString &id) const;

    /**
     * \brief 创建角色（id 由 name 规范化得到）。
     * @param name 显示名。
     * @param[out] newId 非空时写入新角色 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否创建成功；名字为空、id 冲突时返回 false。
     */
    bool create(const QString &name, QString *newId, QString *error = nullptr);
    /**
     * \brief 修改显示名（写回 card.md），目录 id 不变。
     * @param id 角色 id。
     * @param newName 新显示名。
     * @param[out] error 非空时写入失败原因。
     * @return 是否修改成功。
     */
    bool rename(const QString &id, const QString &newName, QString *error = nullptr);
    /**
     * \brief 递归删除角色目录（含形象与人设）。
     * @param id 角色 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否删除成功。
     */
    bool remove(const QString &id, QString *error = nullptr);

    /**
     * \brief 读取默认人设 card.md。
     * @param id 角色 id。
     * @param[out] data 写入解析结果。
     * @param[out] error 非空时写入失败原因。
     * @return 是否解析成功；card.md 缺失或解析失败时返回 false。
     */
    bool card(const QString &id, PersonaData *data, QString *error = nullptr) const;
    /**
     * \brief 写入默认人设 card.md。
     * @param id 角色 id。
     * @param data 人设数据。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功；name 为空时用 id 兜底。
     */
    bool saveCard(const QString &id, const PersonaData &data, QString *error = nullptr);

    /**
     * \brief 读取角色元数据 meta.json。
     * @param id 角色 id。
     * @param[out] meta 写入元数据。
     * @param[out] error 非空时写入失败原因。
     * @return 是否成功；文件不存在时返回默认值并成功，JSON 损坏时返回 false。
     */
    bool meta(const QString &id, CharacterMeta *meta, QString *error = nullptr) const;
    /**
     * \brief 写入角色元数据 meta.json（原子写，保留文件中未识别的字段）。
     * @param id 角色 id。
     * @param meta 元数据。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool saveMeta(const QString &id, const CharacterMeta &meta, QString *error = nullptr);

    /**
     * \brief 更新角色归属世界观（写入 meta.json）。
     * @param id 角色 id。
     * @param newWorldId 世界观 id；空串表示取消归属。
     * @param[out] error 非空时写入失败原因。
     * @return 是否写入成功。
     */
    bool setWorldId(const QString &id, const QString &newWorldId, QString *error = nullptr);
    /**
     * \brief 读取角色归属世界观。
     * @param id 角色 id。
     * @return 世界观 id；读取失败时返回空串。
     */
    QString worldId(const QString &id) const;

    /**
     * \brief 收集同一世界观下其他已启用角色的摘要（WLD-L-01 同世界观角色互知）。
     *
     * 摘要格式为「名字（一句话人设）」：名字取 card.md 的 name，缺失时退回目录 id；
     * 一句话人设取 card.md 正文首行（去掉 Markdown 标题/引用/列表符号后截断到 40 字符）。
     * 仅返回 meta.json 中 enabled 为 true、worldId 相同的其他角色。
     * @param worldId 世界观 id；空串时返回空列表。
     * @param excludeId 需要排除的角色 id（通常是当前对话角色）。
     * @return 角色摘要列表，按角色目录名排序；没有同世界观伙伴时返回空列表。
     */
    QStringList worldPeerSummaries(const QString &worldId, const QString &excludeId) const;

    /**
     * \brief 导入形象文件到 characters/&lt;id&gt;/avatars/。
     *
     * 覆盖同名文件，支持 png/jpg/jpeg/webp/gif/bmp/svg。
     * @param id 角色 id。
     * @param sourcePath 源图片路径。
     * @param[out] storedPath 非空时写入导入后的绝对路径。
     * @param[out] error 非空时写入失败原因。
     * @return 是否导入成功。
     */
    bool importAvatar(const QString &id, const QString &sourcePath, QString *storedPath, QString *error = nullptr);
    /**
     * \brief 异步导入形象文件（G-H-04：耗时操作不冻结 UI）。
     *
     * 文件复制在全局线程池执行；完成后在对象所属线程发出 changed()，
     * 错误文案与同步版 importAvatar 一致。成功后由 QFuture 结果返回存储路径。
     * @param id 角色 id。
     * @param sourcePath 源图片路径。
     * @return 异步结果；ok 为真时 storedPath 有效。
     */
    QFuture<AvatarImportResult> importAvatarAsync(const QString &id, const QString &sourcePath);
    /**
     * \brief avatars/ 下全部图片的绝对路径（按文件名排序）。
     * @param id 角色 id。
     * @return 图片路径列表；没有形象时返回空列表。
     */
    QStringList avatars(const QString &id) const;
    /**
     * \brief 读取首张形象作为缩略图。
     * @param id 角色 id。
     * @return 首张形象的绝对路径；没有形象时返回空串。
     */
    QString primaryAvatar(const QString &id) const;

    /**
     * \brief 读取全部人设（personas/*.md）。
     * @param characterId 角色 id。
     * @return 人设列表；解析失败的条目跳过，id 取文件名。
     */
    QList<PersonaData> personas(const QString &characterId) const;
    /**
     * \brief 读取单套人设。
     * @param characterId 角色 id。
     * @param personaId 人设 id。
     * @param[out] data 写入解析结果。
     * @param[out] error 非空时写入失败原因。
     * @return 是否读取成功。
     */
    bool persona(const QString &characterId, const QString &personaId, PersonaData *data,
                 QString *error = nullptr) const;
    /**
     * \brief 新建或覆盖一套人设。
     * @param characterId 角色 id。
     * @param data 人设数据；data.id 为空时由 data.name 规范化生成。
     * @param[out] personaId 非空时写入实际人设 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否保存成功。
     */
    bool savePersona(const QString &characterId,
                     const PersonaData &data,
                     QString *personaId = nullptr,
                     QString *error = nullptr);
    /**
     * \brief 修改人设显示名（文件与 id 不变）。
     * @param characterId 角色 id。
     * @param personaId 人设 id。
     * @param newName 新显示名。
     * @param[out] error 非空时写入失败原因。
     * @return 是否修改成功。
     */
    bool renamePersona(const QString &characterId,
                       const QString &personaId,
                       const QString &newName,
                       QString *error = nullptr);
    /**
     * \brief 删除一套人设文件。
     * @param characterId 角色 id。
     * @param personaId 人设 id。
     * @param[out] error 非空时写入失败原因。
     * @return 是否删除成功。
     */
    bool removePersona(const QString &characterId, const QString &personaId, QString *error = nullptr);

signals:
    /// 角色数据发生变化（创建、改名、删除、保存人设/元数据后发出）。
    void changed();

private:
    bool writeCard(const QString &id, const PersonaData &data, QString *error) const;
    bool writeMetaJson(const QString &id, const CharacterMeta &meta, QString *error) const;
    /// 形象导入的实际实现（不发出 changed()，供同步 / 异步入口共用）。
    bool importAvatarImpl(const QString &id, const QString &sourcePath, QString *storedPath, QString *error);

    QString m_dataDir; ///< 数据根目录。
};

#endif // CHARACTERSTORE_H
