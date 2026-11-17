/**
 * \file src/data/chatstore.h
 * \brief 聊天记录持久化：SQLite 单连接、自动建表、按会话保存消息。
 */

#ifndef CHATSTORE_H
#define CHATSTORE_H

#include <QObject>
#include <QString>
#include <QVector>

#include "domain/chatmessage.h"

struct sqlite3;

/**
 * \brief SQLite 聊天记录仓库。
 *
 * 单连接、自动建表；消息按会话（conversation）保存，会话与人设绑定：
 * 人设变化时 ensureConversation() 自动开新会话。
 * 本类只做存储，不包含对话交互规则。
 */
class ChatStore : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造聊天记录仓库并打开数据库。
     * @param dbPath 数据库路径；为空时使用数据目录下的 chat.db。
     * @param parent 父对象。
     */
    explicit ChatStore(const QString &dbPath = QString(), QObject *parent = nullptr);
    ~ChatStore() override;

    /// 数据库是否已就绪。
    bool ready() const { return m_ready; }
    /// 数据库文件路径。
    QString dbPath() const { return m_dbPath; }
    /// 最近一次失败原因。
    QString lastError() const { return m_lastError; }

    /**
     * \brief 确保当前会话存在；人设变化时自动开新会话。
     * @param personaId 当前人设 id。
     * @return 会话 id；数据库未就绪时返回 -1。
     */
    qint64 ensureConversation(const QString &personaId);

    /**
     * \brief 强制开一个新会话（切人设时使用）。
     * @param personaId 新人设 id。
     * @return 新会话 id；数据库未就绪时返回 -1。
     */
    qint64 newConversation(const QString &personaId);
    /// 当前会话 id；无会话时为 -1。
    qint64 conversationId() const { return m_conversationId; }
    /// 当前会话绑定的人设 id。
    QString conversationPersonaId() const { return m_personaId; }

    /**
     * \brief 读取当前会话最近若干条消息。
     * @param limit 条数上限。
     * @return 按时间正序返回的消息列表。
     */
    QVector<StoredMessage> recentMessages(int limit) const;

    /**
     * \brief 向当前会话追加一条消息。
     * @param role 消息角色（user / assistant / system）。
     * @param content 消息正文。
     * @param[out] outId 非空时写入新消息 id。
     * @return 是否写入成功。
     */
    bool appendMessage(const QString &role, const QString &content, qint64 *outId = nullptr);
    /**
     * \brief 清空当前会话的全部消息。
     * @return 是否清空成功。
     */
    bool clearCurrentMessages();
    /**
     * \brief 删除当前会话中 id 大于给定消息的消息（消息回溯）。
     * @param messageId 保留的最后一条消息 id。
     * @return 删除条数；失败返回 -1。
     */
    int truncateAfter(qint64 messageId);
    /**
     * \brief 设置某条消息的本地评价。
     * @param messageId 消息 id。
     * @param value 1 点赞、-1 点踩、0 取消。
     * @return 是否写入成功。
     */
    bool setFeedback(qint64 messageId, int value);
    /**
     * \brief 统计当前会话的点赞与点踩条数。
     * @param[out] likes 写入点赞数；可为空。
     * @param[out] dislikes 写入点踩数；可为空。
     */
    void feedbackStats(int *likes, int *dislikes) const;

signals:
    /// 聊天记录发生变化（消息写入、清空、回溯、评价更新后发出）。
    void changed();

private:
    bool openDatabase(const QString &dbPath);
    bool createSchema();
    bool exec(const QString &sql);
    bool readMeta(const QString &key, QString *value) const;
    bool writeMeta(const QString &key, const QString &value);
    void setError(const QString &context);

    sqlite3 *m_db = nullptr;   ///< SQLite 连接句柄。
    QString m_dbPath;          ///< 数据库文件路径。
    QString m_lastError;       ///< 最近一次失败原因。
    qint64 m_conversationId = -1; ///< 当前会话 id。
    QString m_personaId;       ///< 当前会话绑定的人设 id。
    bool m_ready = false;      ///< 数据库是否就绪。
};

#endif // CHATSTORE_H
