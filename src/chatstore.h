#ifndef CHATSTORE_H
#define CHATSTORE_H

#include <QObject>
#include <QString>
#include <QVector>

struct sqlite3;

/// 一条持久化的聊天消息。
struct StoredMessage
{
    QString role;
    QString content;
};

/// SQLite 聊天记录：单连接，自动建表，按会话保存消息。
class ChatStore : public QObject
{
    Q_OBJECT

public:
    explicit ChatStore(const QString &dbPath = QString(), QObject *parent = nullptr);
    ~ChatStore() override;

    bool ready() const { return m_ready; }
    QString dbPath() const { return m_dbPath; }
    QString lastError() const { return m_lastError; }

    /// 确保当前会话存在；人设变化时自动开新会话并返回新 id。
    qint64 ensureConversation(const QString &personaId);

    /// 强制开一个新会话（切人设时使用）。
    qint64 newConversation(const QString &personaId);
    qint64 conversationId() const { return m_conversationId; }
    QString conversationPersonaId() const { return m_personaId; }

    /// 最近 limit 条消息（按时间正序返回）。
    QVector<StoredMessage> recentMessages(int limit) const;

    bool appendMessage(const QString &role, const QString &content);
    bool clearCurrentMessages();

signals:
    void changed();

private:
    bool openDatabase(const QString &dbPath);
    bool createSchema();
    bool exec(const QString &sql);
    bool readMeta(const QString &key, QString *value) const;
    bool writeMeta(const QString &key, const QString &value);
    void setError(const QString &context);

    sqlite3 *m_db = nullptr;
    QString m_dbPath;
    QString m_lastError;
    qint64 m_conversationId = -1;
    QString m_personaId;
    bool m_ready = false;
};

#endif // CHATSTORE_H
