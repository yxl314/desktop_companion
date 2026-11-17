#include "chatservice.h"

#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

#include <stacklog/stacklog.hpp>

#include "chatstore.h"
#include "configstore.h"
#include "intimacyservice.h"
#include "memorystore.h"
#include "personastore.h"

namespace
{
constexpr int maxHistoryMessages = 20;
constexpr int maxMemoriesPerPrompt = 5;
constexpr int maxExtractedMemories = 5;

/// 模型抽取只拿到推理、没有正文时的兜底：按常见自述句式提取。
QStringList heuristicMemories(const QString &userText)
{
    QStringList result;
    const QRegularExpression nameRe(QStringLiteral("我叫([^，。,.!?！？\\s]{1,20})"));
    const QRegularExpression likeRe(QStringLiteral("我喜欢([^，。,.!?！？]{1,30})"));
    const QRegularExpression liveRe(QStringLiteral("我住在([^，。,.!?！？]{1,30})"));
    const QRegularExpression rememberRe(QStringLiteral("记住([^。！？!?]{1,60})"));
    const QRegularExpression isRe(QStringLiteral("我是([^，。,.!?！？]{1,30})"));

    if (const auto match = nameRe.match(userText); match.hasMatch())
        result.append(QStringLiteral("用户叫%1").arg(match.captured(1)));
    if (const auto match = likeRe.match(userText); match.hasMatch())
        result.append(QStringLiteral("用户喜欢%1").arg(match.captured(1)));
    if (const auto match = liveRe.match(userText); match.hasMatch())
        result.append(QStringLiteral("用户住在%1").arg(match.captured(1)));
    if (const auto match = isRe.match(userText); match.hasMatch())
        result.append(QStringLiteral("用户自称：%1").arg(match.captured(1)));
    if (const auto match = rememberRe.match(userText); match.hasMatch())
        result.append(QStringLiteral("用户要求记住：%1").arg(match.captured(1)));
    return result;
}
} // namespace

ChatService::ChatService(ConfigStore *config, PersonaStore *personas, ChatStore *store,
                         MemoryStore *memory, IntimacyService *intimacy, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_personas(personas)
    , m_store(store)
    , m_memory(memory)
    , m_intimacy(intimacy)
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
        appendStored(QStringLiteral("assistant"), m_partial);
        finishExchange();
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
        appendStored(QStringLiteral("assistant"), m_partial);
        finishExchange();
    });
    connect(&m_llm, &LlmClient::failed, this, [this](const QString &message) {
        if (!m_active || m_source != Source::Llm)
            return;
        m_active = false;
        m_source = Source::None;
        emit stateChanged(QStringLiteral("idle"));
        emit failed(message);
    });
    connect(&m_llm, &LlmClient::testFinished, this, &ChatService::testFinished);

    // 记忆自动抽取：单独的 LlmClient，后台进行，不阻塞对话。
    connect(&m_extractor, &LlmClient::delta, this, [this](const QString &text) {
        m_extractBuffer += text;
    });
    connect(&m_extractor, &LlmClient::finished, this, [this] {
        m_extracting = false;
        if (m_memory == nullptr || m_characterId.isEmpty())
            return;
        const QStringList lines = m_extractBuffer.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        int added = 0;
        for (QString line : lines) {
            line = line.trimmed();
            while (line.startsWith(QLatin1Char('-')) || line.startsWith(QLatin1Char('*')))
                line = line.mid(1).trimmed();
            if (line.isEmpty() || line.size() > 200)
                continue;
            MemoryEntry entry;
            entry.characterId = m_characterId;
            entry.personaId = m_personaId;
            entry.content = line;
            QString error;
            if (m_memory->add(&entry, &error) && ++added >= maxExtractedMemories)
                break;
        }
        if (added == 0) {
            // 模型可能只输出推理（content 为空）：用本地句式兜底。
            for (const QString &line : heuristicMemories(m_lastUserText)) {
                MemoryEntry entry;
                entry.characterId = m_characterId;
                entry.personaId = m_personaId;
                entry.content = line;
                QString error;
                if (m_memory->add(&entry, &error) && ++added >= maxExtractedMemories)
                    break;
            }
        }
        STACKLOG_INFO() << "memory extraction finished, added=" << added;
        if (added > 0) {
            STACKLOG_INFO() << "auto extracted " << added << " memories";
            emit memoriesChanged();
        }
    });
    connect(&m_extractor, &LlmClient::failed, this, [this](const QString &message) {
        m_extracting = false;
        STACKLOG_WARN() << "memory extraction failed: " << message.toStdString();
    });
}

void ChatService::testConnection(const QString &baseUrl, const QString &apiKey, const QString &model)
{
    m_llm.configure(baseUrl, apiKey, model);
    m_llm.testConnection();
}

void ChatService::appendHistory(const QString &role, const QString &content)
{
    if (content.isEmpty())
        return;
    m_history.append(qMakePair(role, content));
    while (m_history.size() > maxHistoryMessages)
        m_history.removeFirst();
}

void ChatService::appendStored(const QString &role, const QString &content)
{
    if (m_store != nullptr && m_store->appendMessage(role, content))
        return;
}

QVariantList ChatService::initialMessages() const
{
    QVariantList result;
    for (const auto &entry : m_history) {
        QVariantMap item;
        item.insert(QStringLiteral("role"), entry.first);
        item.insert(QStringLiteral("text"), entry.second);
        result.append(item);
    }
    return result;
}

void ChatService::setContext(const QString &partitionKey, const QString &characterId,
                             const QString &personaId, const PromptContext &baseContext)
{
    m_baseContext = baseContext;
    m_characterId = characterId;
    m_personaId = personaId;
    if (m_contextInitialized && partitionKey == m_partitionKey)
        return;

    m_partitionKey = partitionKey;
    m_contextInitialized = true;
    m_history.clear();
    if (m_store != nullptr) {
        m_store->ensureConversation(m_partitionKey);
        for (const StoredMessage &message : m_store->recentMessages(maxHistoryMessages))
            m_history.append(qMakePair(message.role, message.content));
    }
    emit conversationCleared();
}

QString ChatService::buildPrompt(const QString &userText) const
{
    PromptContext context = m_baseContext;
    if (m_memory != nullptr && !m_characterId.isEmpty()) {
        QString error;
        const QVector<MemoryEntry> memories =
            m_memory->search(m_characterId, m_personaId, userText, maxMemoriesPerPrompt, &error);
        for (const MemoryEntry &memory : memories)
            context.memories.append(memory.content);
    }
    return PromptBuilder::build(context);
}

void ChatService::startLlm(const QString &userText)
{
    m_llm.configure(m_config->values().baseUrl, m_config->values().apiKey, m_config->values().model);
    if (!m_llm.ready())
        return;

    m_source = Source::Llm;
    emit stateChanged(QStringLiteral("thinking"));

    QJsonArray history;
    for (const auto &entry : m_history) {
        history.append(QJsonObject{{QStringLiteral("role"), entry.first},
                                   {QStringLiteral("content"), entry.second}});
    }
    m_llm.start(buildPrompt(userText), history, userText);
}

void ChatService::send(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        emit failed(QStringLiteral("输入不能为空"));
        return;
    }
    if (m_active)
        return;

    m_active = true;
    m_partial.clear();
    m_lastUserText = trimmed;
    appendStored(QStringLiteral("user"), trimmed);
    emit userMessage(trimmed);

    m_llm.configure(m_config->values().baseUrl, m_config->values().apiKey, m_config->values().model);
    if (m_llm.ready()) {
        startLlm(trimmed);
    } else {
        m_source = Source::Mock;
        m_mock.send(trimmed);
    }
}

void ChatService::finishExchange()
{
    m_active = false;
    m_source = Source::None;

    if (m_intimacy != nullptr && !m_characterId.isEmpty() && m_intimacy->loaded()) {
        m_intimacy->add(2, QStringLiteral("对话"));
        emit intimacyChanged();
    }

    if (m_config->values().autoExtractMemories && !m_partial.isEmpty())
        startExtraction(m_lastUserText, m_partial);

    emit done();
    emit stateChanged(QStringLiteral("idle"));
}

void ChatService::startExtraction(const QString &userText, const QString &assistantText)
{
    if (m_extracting || m_memory == nullptr || m_characterId.isEmpty())
        return;
    m_llm.configure(m_config->values().baseUrl, m_config->values().apiKey, m_config->values().model);
    if (!m_llm.ready())
        return;

    m_extractBuffer.clear();
    m_extracting = true;
    STACKLOG_INFO() << "memory extraction started";
    m_extractor.configure(m_config->values().baseUrl, m_config->values().apiKey,
                          m_config->values().model);
    const QString prompt = QStringLiteral(
                               "从下面的对话中提取关于用户的长期有用事实（身份、偏好、计划等），"
                               "每行一条，不要编号，最多 5 条；没有就输出空行。\n用户：%1\n助手：%2")
                               .arg(userText, assistantText.left(800));
    m_extractor.start(QString(), QJsonArray(), prompt);
}

void ChatService::stop()
{
    if (!m_active)
        return;

    if (m_source == Source::Llm) {
        m_llm.abort();
        appendHistory(QStringLiteral("assistant"), m_partial);
        appendStored(QStringLiteral("assistant"), m_partial);
        finishExchange();
    } else if (m_source == Source::Mock) {
        m_mock.cancel();
    }
}

void ChatService::resetPartition(bool resetMemory)
{
    m_history.clear();
    if (resetMemory && m_memory != nullptr && !m_characterId.isEmpty()) {
        m_memory->resetForPersona(m_characterId, m_personaId);
        emit memoriesChanged();
    }
    emit conversationCleared();
}

void ChatService::clearConversation()
{
    if (m_store != nullptr)
        m_store->clearCurrentMessages();
    resetPartition(false);
}

void ChatService::restartConversation()
{
    if (m_store != nullptr)
        m_store->newConversation(m_partitionKey);
    resetPartition(true);
}

void ChatService::backtrackToMessage(qint64 messageId)
{
    if (m_store != nullptr && m_store->truncateAfter(messageId) < 0)
        return;
    m_history.clear();
    if (m_store != nullptr) {
        for (const StoredMessage &message : m_store->recentMessages(maxHistoryMessages))
            m_history.append(qMakePair(message.role, message.content));
    }
    resetPartition(true);
}

void ChatService::backtrackToIndex(int index)
{
    if (m_store == nullptr || index < 0)
        return;
    const QVector<StoredMessage> messages = m_store->recentMessages(10000);
    if (index >= messages.size())
        return;
    backtrackToMessage(messages.at(index).id);
}
