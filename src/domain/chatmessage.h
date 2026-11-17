/**
 * \file src/domain/chatmessage.h
 * \brief 聊天领域的值类型：一条持久化的聊天消息及其本地评价。
 */

#ifndef DOMAIN_CHATMESSAGE_H
#define DOMAIN_CHATMESSAGE_H

#include <QString>

/**
 * \brief 一条持久化的聊天消息。
 *
 * feedback 承载用户评价：0 未评价、1 点赞、-1 点踩（仅存本地）。
 */
struct StoredMessage
{
    qint64 id = -1;    ///< 数据库行 id，未落盘时为 -1。
    QString role;      ///< 角色：user / assistant / system。
    QString content;   ///< 消息正文。
    int feedback = 0;  ///< 本地评价：0 未评价 1 点赞 -1 点踩。
};

#endif // DOMAIN_CHATMESSAGE_H
