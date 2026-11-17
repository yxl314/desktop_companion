#ifndef PERSONAFILE_H
#define PERSONAFILE_H

#include <QString>

/// 一套人设：frontmatter 中的结构化字段 + Markdown 正文。
///
/// 人设文件格式（YAML-lite）：
/// ```text
/// ---
/// name: 小柚
/// birthday: 2000-01-01
/// zodiac: 摩羯座
/// hobbies: 看书
/// favorite_food: 草莓
/// background: 来自桌面
/// ---
///
/// 正文……
/// ```
/// 支持中英文 key（称呼 / name、生日 / birthday、星座 / zodiac、
/// 爱好 / hobbies、最爱的食物 / favorite_food、背景故事 / background）。
/// 没有 frontmatter 时整文件作为正文，文件名（去扩展名）作为名字。
struct PersonaData
{
    QString id; // 人设 id（通常为其文件名去扩展名，由存储层填充）
    QString name; // 称呼 / name
    QString birthday; // 生日 / birthday
    QString zodiac; // 星座 / zodiac
    QString hobbies; // 爱好 / hobbies
    QString favoriteFood; // 最爱的食物 / favorite_food
    QString background; // 背景故事 / background
    QString body; // 正文（结束分隔符之后的内容，已去掉首尾空白）
};

/// 把任意名称规范成安全的文件 / 目录 id（只保留字母、数字、中文、'-'、'_'）；
/// 无法规范化（全部为空白或非法字符）时返回空串；Windows 设备名会自动加 '_' 前缀。
QString sanitizeFileId(const QString &raw);

/// 解析人设 .md 文件；失败时返回 false 并通过 error 给出可读原因。
bool parsePersonaFile(const QString &filePath, PersonaData *data, QString *error = nullptr);

/// 解析人设 Markdown 文本；fallbackName 用于无 frontmatter 或 frontmatter 缺少 name 时。
bool parsePersonaMarkdown(const QString &markdown,
                          const QString &fallbackName,
                          PersonaData *data,
                          QString *error = nullptr);

/// 渲染人设 Markdown：frontmatter + 空行 + 正文，始终以换行结尾。
/// 渲染使用英文 key（name / birthday / zodiac / hobbies / favorite_food / background）。
QString renderPersonaMarkdown(const PersonaData &data);

#endif // PERSONAFILE_H
