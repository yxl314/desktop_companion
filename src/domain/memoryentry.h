/**
 * \file src/domain/memoryentry.h
 * \brief 记忆领域的值类型：一条结构化长期记忆。
 */

#ifndef DOMAIN_MEMORYENTRY_H
#define DOMAIN_MEMORYENTRY_H

#include <QString>
#include <QStringList>

/**
 * \brief 一条结构化记忆。
 *
 * character/persona 决定记忆分区，pinned 条目每轮对话必注入；
 * 持久化由 data/MemoryStore（SQLite）负责。
 */
struct MemoryEntry
{
    qint64 id = -1;             ///< 数据库行 id，未落盘时为 -1。
    QString characterId;        ///< 所属角色 id（分区键）。
    QString personaId;          ///< 所属人设 id（分区键）。
    QString content;            ///< 记忆正文。
    bool pinned = false;        ///< 是否置顶（置顶条目每轮必注入）。
    QStringList tags;           ///< 标签列表。
    qint64 sourceMessageId = -1; ///< 来源消息 id，-1 表示非对话抽取。
    QString createdAt;          ///< 创建时间（ISO 8601）。
    QString lastUsedAt;         ///< 最近使用时间（ISO 8601）。
};

#endif // DOMAIN_MEMORYENTRY_H
