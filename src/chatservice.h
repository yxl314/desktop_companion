#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariantList>

#include "chatengine.h"
#include "llmclient.h"

class ChatStore;
class ConfigStore;
class PersonaStore;

/// 对话服务：配置了模型走 LLM，否则退回本地 mock；维护最近若干轮历史。
class ChatService : public QObject
{
    Q_OBJECT

public:
    ChatService(ConfigStore *config, PersonaStore *personas, ChatStore *store, QObject *parent = nullptr);

    bool llmReady() const { return m_llm.ready(); }
    bool busy() const { return m_active; }

    void send(const QString &text);
    void stop();
    void clearConversation();

    /// 切换对话上下文：分区键（角色+人设）与 system prompt 覆盖。
    /// 分区键变化时开新会话并重新加载历史。
    void setContext(const QString &partitionKey, const QString &systemPrompt);
    QString partitionKey() const { return m_partitionKey; }

    /// 切人设：开一个新会话并清空上下文。
    void switchPersona(const QString &personaId);

    /// 启动时给聊天窗恢复的历史消息。
    QVariantList initialMessages() const;

    /// 用给定的连接参数做一次 /models 连通性检查，结果通过 testFinished 返回。
    void testConnection(const QString &baseUrl, const QString &apiKey, const QString &model);

signals:
    void userMessage(const QString &text);
    void delta(const QString &text);
    void done();
    void failed(const QString &message);
    void stateChanged(const QString &state);
    void conversationCleared();
    void testFinished(bool ok, const QString &message);

private:
    enum class Source
    {
        None,
        Mock,
        Llm
    };

    void sendMock(const QString &text);
    void appendHistory(const QString &role, const QString &content);
    QString systemPrompt() const;

    void appendStored(const QString &role, const QString &content);

    ConfigStore *m_config = nullptr;
    PersonaStore *m_personas = nullptr;
    ChatStore *m_store = nullptr;
    ChatEngine m_mock;
    LlmClient m_llm;
    QList<QPair<QString, QString>> m_history;
    QString m_partial;
    QString m_partitionKey;
    bool m_contextInitialized = false;
    QString m_promptOverride;
    Source m_source = Source::None;
    bool m_active = false;
};

#endif // CHATSERVICE_H
