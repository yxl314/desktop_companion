#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariantList>

#include "chatengine.h"
#include "llmclient.h"
#include "promptbuilder.h"

class ChatStore;
class ConfigStore;
class IntimacyService;
class MemoryStore;
class PersonaStore;

/// 对话服务：LLM 优先、未配置回退 mock；负责历史、记忆注入、好感度与自动抽取。
class ChatService : public QObject
{
    Q_OBJECT

public:
    ChatService(ConfigStore *config, PersonaStore *personas, ChatStore *store,
                MemoryStore *memory, IntimacyService *intimacy, QObject *parent = nullptr);

    bool llmReady() const { return m_llm.ready(); }
    bool busy() const { return m_active; }

    void send(const QString &text);
    void stop();
    void clearConversation();
    void restartConversation();
    void retryLast();
    void backtrackToMessage(qint64 messageId);
    void backtrackToIndex(int index);

    /// 设置对话上下文：分区键、角色/人设（记忆分区）、system prompt 基础上下文。
    void setContext(const QString &partitionKey, const QString &characterId, const QString &personaId,
                    const PromptContext &baseContext);
    QString partitionKey() const { return m_partitionKey; }
    QString characterId() const { return m_characterId; }
    QString personaId() const { return m_personaId; }

    QVariantList initialMessages() const;

    /// 用给定连接参数做一次 /models 连通性检查。
    void testConnection(const QString &baseUrl, const QString &apiKey, const QString &model);

signals:
    void userMessage(const QString &text);
    void delta(const QString &text);
    void done();
    void failed(const QString &message);
    void stateChanged(const QString &state);
    void conversationCleared();
    void memoriesChanged();
    void intimacyChanged();
    void testFinished(bool ok, const QString &message);

private:
    enum class Source
    {
        None,
        Mock,
        Llm
    };

    void appendHistory(const QString &role, const QString &content);
    void appendStored(const QString &role, const QString &content);
    void startLlm(const QString &userText);
    void finishExchange();
    QString buildPrompt(const QString &userText) const;
    void resetPartition(bool resetMemory);
    void startExtraction(const QString &userText, const QString &assistantText);

    ConfigStore *m_config = nullptr;
    PersonaStore *m_personas = nullptr;
    ChatStore *m_store = nullptr;
    MemoryStore *m_memory = nullptr;
    IntimacyService *m_intimacy = nullptr;
    ChatEngine m_mock;
    LlmClient m_llm;
    LlmClient m_extractor;
    QList<QPair<QString, QString>> m_history;
    QString m_partial;
    QString m_lastUserText;
    QString m_extractBuffer;
    QString m_partitionKey;
    QString m_characterId;
    QString m_personaId;
    PromptContext m_baseContext;
    Source m_source = Source::None;
    bool m_active = false;
    bool m_contextInitialized = false;
    bool m_extracting = false;
};

#endif // CHATSERVICE_H
