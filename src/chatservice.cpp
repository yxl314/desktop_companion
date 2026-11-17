#include "chatservice.h"

#include <QJsonObject>

#include "configstore.h"
#include "personastore.h"

namespace
{
constexpr int maxHistoryMessages = 20;
} // namespace

ChatService::ChatService(ConfigStore *config, PersonaStore *personas, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_personas(personas)
{
    connect(&m_mock, &ChatEngine::delta, this, [this](const QString &text) {
        if (!m_active || m_source != Source::Mock)
            return;
        m_partial += text;
        emit delta(text);
    });
    connect(&m_mock, &ChatEngine::stateChanged, this, [this](const QString &state) {
        if (m_active && m_source == Source::Mock)
            emit stateChanged(state);
    });
    connect(&m_mock, &ChatEngine::done, this, [this] {
        if (!m_active || m_source != Source::Mock)
            return;
        appendHistory(QStringLiteral("assistant"), m_partial);
        m_active = false;
        m_source = Source::None;
        emit done();
        emit stateChanged(QStringLiteral("idle"));
    });

    connect(&m_llm, &LlmClient::delta, this, [this](const QString &text) {
        if (!m_active || m_source != Source::Llm)
            return;
        if (m_partial.isEmpty())
            emit stateChanged(QStringLiteral("streaming"));
        m_partial += text;
        emit delta(text);
    });
    connect(&m_llm, &LlmClient::finished, this, [this] {
        if (!m_active || m_source != Source::Llm)
            return;
        appendHistory(QStringLiteral("assistant"), m_partial);
        m_active = false;
        m_source = Source::None;
        emit done();
        emit stateChanged(QStringLiteral("idle"));
    });
    connect(&m_llm, &LlmClient::testFinished, this, &ChatService::testFinished);
    connect(&m_llm, &LlmClient::failed, this, [this](const QString &message) {
        if (!m_active || m_source != Source::Llm)
            return;
        m_active = false;
        m_source = Source::None;
        emit stateChanged(QStringLiteral("idle"));
        emit failed(message);
    });
}

void ChatService::appendHistory(const QString &role, const QString &content)
{
    if (content.isEmpty())
        return;
    m_history.append(qMakePair(role, content));
    while (m_history.size() > maxHistoryMessages)
        m_history.removeFirst();
}

QString ChatService::systemPrompt() const
{
    return m_personas->find(m_config->values().personaId).prompt;
}

void ChatService::send(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || m_active)
        return;

    m_active = true;
    m_partial.clear();
    emit userMessage(trimmed);

    m_llm.configure(m_config->values().baseUrl, m_config->values().apiKey, m_config->values().model);

    if (m_llm.ready()) {
        m_source = Source::Llm;
        emit stateChanged(QStringLiteral("thinking"));

        QJsonArray history;
        for (const auto &entry : m_history) {
            history.append(QJsonObject{{QStringLiteral("role"), entry.first},
                                       {QStringLiteral("content"), entry.second}});
        }
        m_llm.start(systemPrompt(), history, trimmed);
    } else {
        m_source = Source::Mock;
        m_mock.send(trimmed);
    }
}

void ChatService::sendMock(const QString &text)
{
    m_source = Source::Mock;
    m_mock.send(text);
}

void ChatService::stop()
{
    if (!m_active)
        return;

    if (m_source == Source::Llm) {
        m_llm.abort();
        appendHistory(QStringLiteral("assistant"), m_partial);
        m_active = false;
        m_source = Source::None;
        emit done();
        emit stateChanged(QStringLiteral("idle"));
    } else if (m_source == Source::Mock) {
        m_mock.cancel();
    }
}

void ChatService::clearConversation()
{
    m_history.clear();
    emit conversationCleared();
}

void ChatService::testConnection(const QString &baseUrl, const QString &apiKey, const QString &model)
{
    m_llm.configure(baseUrl, apiKey, model);
    m_llm.testConnection();
}
