/**
 * \file src/domain/character.h
 * \brief 角色领域的值类型：人设数据、角色列表项、角色元数据与世界观。
 *
 * 本文件只包含纯数据定义，不依赖 Qt GUI、不做任何 IO；
 * 存储由 data/CharacterStore、data/WorldStore 负责，
 * 人设文件的解析与渲染由 services/personafile 负责。
 */

#ifndef DOMAIN_CHARACTER_H
#define DOMAIN_CHARACTER_H

#include <QJsonObject>
#include <QPointF>
#include <QString>

/**
 * \brief 一套人设：frontmatter 中的结构化字段 + Markdown 正文。
 *
 * 人设文件格式（YAML-lite）：
 * \code{.text}
 * ---
 * name: 小柚
 * birthday: 2000-01-01
 * zodiac: 摩羯座
 * hobbies: 看书
 * favorite_food: 草莓
 * background: 来自桌面
 * ---
 *
 * 正文……
 * \endcode
 *
 * 支持中英文 key（称呼 / name、生日 / birthday、星座 / zodiac、
 * 爱好 / hobbies、最爱的食物 / favorite_food、背景故事 / background）。
 * 没有 frontmatter 时整文件作为正文，文件名（去扩展名）作为名字。
 */
struct PersonaData
{
    QString id;           ///< 人设 id（通常为其文件名去扩展名，由存储层填充）。
    QString name;         ///< 称呼 / name。
    QString birthday;     ///< 生日 / birthday。
    QString zodiac;       ///< 星座 / zodiac。
    QString hobbies;      ///< 爱好 / hobbies。
    QString favoriteFood; ///< 最爱的食物 / favorite_food。
    QString background;   ///< 背景故事 / background。
    QString body;         ///< 正文（结束分隔符之后的内容，已去掉首尾空白）。
};

/**
 * \brief 角色列表项：目录 id + 显示名 + 世界观归属 + 形象缩略图。
 */
struct Character
{
    QString id;        ///< 角色目录 id。
    QString name;      ///< 显示名。
    QString worldId;   ///< 归属世界观 id，空串表示未归属。
    QString avatarPath; ///< avatars/ 下首个图片的绝对路径，没有则为空。
    bool enabled = true; ///< 是否启用该角色。
};

/**
 * \brief 角色元数据，对应 characters/&lt;id&gt;/meta.json。
 *
 * hitAreas / motions 的结构由命中与动效模块解释，存储层原样透传；
 * anchor 约定为归一化坐标（0~1），存储层不做校验。
 */
struct CharacterMeta
{
    QJsonObject hitAreas; ///< 命中区定义，原样透传。
    QJsonObject motions;  ///< 动效定义，原样透传。
    double scale = 1.0;   ///< 整体缩放系数。
    QPointF anchor = QPointF(0.5, 1.0); ///< 归一化锚点。
    QString worldId;      ///< 归属世界观；空串表示未归属。
    bool enabled = true;  ///< 是否启用该角色。
};

/**
 * \brief 一个世界观：dataDir/worlds/&lt;id&gt;/world.md。
 */
struct World
{
    QString id;      ///< 世界观目录 id。
    QString name;    ///< 显示名；world.md frontmatter 的 name 字段。
    QString content; ///< 正文，作为提示词的世界观段落。
};

#endif // DOMAIN_CHARACTER_H
