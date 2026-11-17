/**
 * \file src/services/personafile.h
 * \brief 人设 .md 文件解析与渲染（YAML-lite frontmatter + Markdown 正文）。
 */

#ifndef PERSONAFILE_H
#define PERSONAFILE_H

#include <QFuture>
#include <QString>

#include "domain/character.h"

/**
 * \brief 异步解析人设文件的结果。
 */
struct PersonaParseResult
{
    bool ok = false;   ///< 是否解析成功。
    PersonaData data;  ///< 解析结果（ok 为 true 时有效）。
    QString error;     ///< 失败原因（可读文案，失败时非空）。
};

/**
 * \brief 把任意名称规范成安全的文件 / 目录 id。
 *
 * 只保留字母、数字、中文、'-'、'_'；无法规范化（全部为空白或非法字符）时
 * 返回空串；Windows 设备名会自动加 '_' 前缀。
 * @param raw 原始名称。
 * @return 安全 id；无法规范化时返回空串。
 */
QString sanitizeFileId(const QString &raw);

/**
 * \brief 解析人设 .md 文件。
 * @param filePath 人设文件路径。
 * @param[out] data 写入解析结果。
 * @param[out] error 非空时写入可读原因。
 * @return 是否解析成功。
 */
bool parsePersonaFile(const QString &filePath, PersonaData *data, QString *error = nullptr);

/**
 * \brief 异步解析人设 .md 文件（G-H-04：文件较大或磁盘慢时不冻结 UI）。
 *
 * 在全局线程池中执行 parsePersonaFile，解析结果与错误文案和同步版一致；
 * 调用方可配合 QFutureWatcher 回到界面线程处理。
 * @param filePath 人设文件路径。
 * @return 异步解析结果；ok 为真时 data 有效。
 */
QFuture<PersonaParseResult> parsePersonaFileAsync(const QString &filePath);

/**
 * \brief 解析人设 Markdown 文本。
 * @param markdown 人设 Markdown 文本。
 * @param fallbackName 无 frontmatter 或 frontmatter 缺少 name 时使用的名字。
 * @param[out] data 写入解析结果。
 * @param[out] error 非空时写入可读原因。
 * @return 是否解析成功。
 */
bool parsePersonaMarkdown(const QString &markdown,
                          const QString &fallbackName,
                          PersonaData *data,
                          QString *error = nullptr);

/**
 * \brief 渲染人设 Markdown：frontmatter + 空行 + 正文，始终以换行结尾。
 *
 * 渲染使用英文 key（name / birthday / zodiac / hobbies /
 * favorite_food / background）。
 * @param data 人设数据。
 * @return 渲染后的 Markdown 文本。
 */
QString renderPersonaMarkdown(const PersonaData &data);

#endif // PERSONAFILE_H
