/**
 * \file src/viewmodels/chatviewmodel.cpp
 * \brief ChatViewModel 实现：流式消息合并、卡片解析、评价与对话生命周期。
 */

#include "chatviewmodel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QTime>
#include <QVariantMap>

#include "data/characterstore.h"
#include "data/chatstore.h"
#include "data/configstore.h"
#include "data/intimacyrepository.h"
#include "data/memorystore.h"
#include "data/worldstore.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"
#include "services/promptbuilder.h"

namespace
{
/// 消息节流间隔：流式 token 合并到 30ms 一帧。
constexpr int messagesFlushIntervalMs = 30;
/// 上下文重建防抖间隔：合并短时间内的角色/人设/配置变化。
constexpr int contextRefreshDelayMs = 50;
/// 消息行的时间展示格式。
QString currentTimeText()
{
    return QTime::currentTime().toString(QStringLiteral("HH:mm"));
}
} // namespace

ChatViewModel::ChatViewModel(ChatService *chat, ChatStore *store, MemoryStore *memory,
                             IntimacyService *intimacy, ConfigStore *config,
                             CharacterStore *characters, WorldStore *worlds, QObject *parent)
    : ViewModelBase(parent)
    , m_chat(chat)
    , m_store(store)
    , m_memory(memory)
    , m_intimacy(intimacy)
    , m_config(config)
    , m_characters(characters)
    , m_worlds(worlds)
{
    Q_ASSERT(m_chat != nullptr);
    Q_ASSERT(m_store != nullptr);
    Q_ASSERT(m_memory != nullptr);
    Q_ASSERT(m_intimacy != nullptr);
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_characters != nullptr);
    Q_ASSERT(m_worlds != nullptr);

    m_flushTimer.setInterval(messagesFlushIntervalMs);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &ChatViewModel::flushMessagesChanged);

    m_contextTimer.setInterval(contextRefreshDelayMs);
    m_contextTimer.setSingleShot(true);
    connect(&m_contextTimer, &QTimer::timeout, this, &ChatViewModel::applyCharacterContext);

    // WLD-L-01：把同世界观角色摘要来源接线到对话服务。
    m_chat->setWorldPeerSources(m_characters, m_worlds);

    connect(m_chat, &ChatService::userMessage, this, [this](const QString &text) {
        appendRow(QStringLiteral("user"), text, -1, 0);
        emit messagesChanged();
    });
    connect(m_chat, &ChatService::delta, this, [this](const QString &delta) {
        if (m_messages.isEmpty()
            || m_messages.constLast().toMap().value(QStringLiteral("role")).toString()
                   != QLatin1String("assistant")) {
            appendRow(QStringLiteral("assistant"), QString(), -1, 0);
        }
        const int row = m_messages.size() - 1;
        QVariantMap item = m_messages.at(row).toMap();
        item.insert(QStringLiteral("text"),
                    item.value(QStringLiteral("text")).toString() + delta);
        m_messages[row] = item;
        scheduleMessagesChanged();
    });
    connect(m_chat, &ChatService::done, this, [this] {
        if (!m_messages.isEmpty()
            && m_messages.constLast().toMap().value(QStringLiteral("role")).toString()
                   == QLatin1String("assistant")) {
            applyCardToRow(m_messages.size() - 1);
        }
        setStreaming(false);
        setStatus(QString());
        scheduleMessagesChanged();
    });
    connect(m_chat, &ChatService::failed, this, [this](const QString &message) {
        setStreaming(false);
        setError(message);
        setStatus(QString());
        scheduleMessagesChanged();
    });
    connect(m_chat, &ChatService::assistantStored, this, [this](qint64 messageId) {
        if (m_messages.isEmpty())
            return;
        const int row = m_messages.size() - 1;
        QVariantMap item = m_messages.at(row).toMap();
        item.insert(QStringLiteral("id"), messageId);
        m_messages[row] = item;
        scheduleMessagesChanged();
    });
    connect(m_chat, &ChatService::stateChanged, this, [this](const QString &state) {
        if (state == QLatin1String("thinking")) {
            setStreaming(true);
            setStatus(tr("思考中…"));
        } else if (state == QLatin1String("streaming")) {
            setStreaming(true);
            setStatus(tr("回复中…"));
        } else {
            setStreaming(false);
            setStatus(QString());
        }
    });
    connect(m_chat, &ChatService::conversationCleared, this, &ChatViewModel::reloadMessages);
    connect(m_chat, &ChatService::intimacyChanged, this, &ChatViewModel::updateHeader);
    connect(m_intimacy, &IntimacyService::changed, this, &ChatViewModel::updateHeader);

    connect(m_config, &ConfigStore::changed, this, [this] {
        updateHeader();
        if (m_applyingContext)
            return;
        const AppConfig &values = m_config->values();
        if (values.characterId != m_lastCharacterId || values.personaId != m_lastPersonaId)
            scheduleContextRefresh();
    });
    connect(m_characters, &CharacterStore::changed, this, [this] {
        updateHeader();
        if (!m_applyingContext)
            scheduleContextRefresh();
    });
    connect(m_worlds, &WorldStore::changed, this, [this] {
        if (!m_applyingContext)
            scheduleContextRefresh();
    });

    applyCharacterContext();
}

ChatViewModel::~ChatViewModel() = default;

QVariantList ChatViewModel::messages() const
{
    return m_messages;
}

bool ChatViewModel::streaming() const
{
    return m_streaming;
}

QString ChatViewModel::chatTitle() const
{
    return m_chatTitle;
}

QString ChatViewModel::modelSummary() const
{
    return m_modelSummary;
}

bool ChatViewModel::llmReady() const
{
    return m_llmReady;
}

QString ChatViewModel::welcomeText() const
{
    return m_welcomeText;
}

QStringList ChatViewModel::quickPrompts() const
{
    return {
        tr("今天过得怎么样？"),
        tr("陪我聊一会儿吧"),
        tr("给我讲个故事"),
        tr("夸夸我"),
    };
}

int ChatViewModel::intimacyScore() const
{
    return m_intimacyScore;
}

QString ChatViewModel::intimacyLevel() const
{
    return m_intimacyLevel;
}

void ChatViewModel::send(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        setError(tr("输入不能为空"));
        return;
    }
    if (m_contextTimer.isActive()) {
        // 先落地待处理的角色/人设切换，避免它在本轮流式期间清空消息列表。
        m_contextTimer.stop();
        applyCharacterContext();
    }
    setError(QString());
    setStatus(QString());
    m_chat->send(trimmed);
}

void ChatViewModel::retry()
{
    if (m_contextTimer.isActive()) {
        m_contextTimer.stop();
        applyCharacterContext();
    }
    setError(QString());
    m_chat->retryLast();
}

void ChatViewModel::stop()
{
    m_chat->stop();
}

void ChatViewModel::restart()
{
    m_chat->restartConversation();
    setStatus(tr("已重启对话，记忆已重置"));
}

void ChatViewModel::clear()
{
    m_chat->clearConversation();
    setStatus(tr("已清空对话"));
    setError(QString());
}

void ChatViewModel::backtrack(int index)
{
    if (index < 0 || index >= m_messages.size())
        return;
    // 安全性探测：数据层 ChatStore::recentMessages 目前没有回填 StoredMessage::id（恒为 -1），
    // 走 ChatService::backtrackToIndex 会以 -1 调用 truncateAfter 而删除全部消息。
    // 在数据层补上 id/feedback 查询之前拒绝执行，避免破坏用户数据。
    const QVector<StoredMessage> probe = m_store->recentMessages(1);
    if (!probe.isEmpty() && probe.constFirst().id < 0) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState),
                    tr("消息记录缺少数据库 id，无法安全回溯（等待数据层修复）"));
        return;
    }
    m_chat->backtrackToIndex(index);
    setStatus(tr("已回溯，之后的消息与记忆已清除"));
}

void ChatViewModel::setFeedback(int index, int value)
{
    if (index < 0 || index >= m_messages.size())
        return;
    QVariantMap item = m_messages.at(index).toMap();
    const qint64 messageId = item.value(QStringLiteral("id")).toLongLong();
    if (messageId < 0) {
        setError(tr("消息尚未落盘，暂时无法评价"));
        return;
    }
    QString error;
    if (!m_store->setFeedback(messageId, value)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    item.insert(QStringLiteral("feedback"), value);
    m_messages[index] = item;
    scheduleMessagesChanged();
}

void ChatViewModel::toggleExpanded(int index)
{
    if (index < 0 || index >= m_messages.size())
        return;
    QVariantMap item = m_messages.at(index).toMap();
    item.insert(QStringLiteral("expanded"), !item.value(QStringLiteral("expanded")).toBool());
    m_messages[index] = item;
    scheduleMessagesChanged();
}

void ChatViewModel::exportChat()
{
    const QDir exportsDir(dataDir() + QStringLiteral("/exports"));
    if (!QDir().mkpath(exportsDir.absolutePath())) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), exportsDir.absolutePath());
        return;
    }
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString path = exportsDir.filePath(QStringLiteral("chat-") + stamp + QStringLiteral(".md"));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), file.errorString());
        return;
    }
    QTextStream out(&file);
    out << tr("# 与 %1 的聊天记录\n\n").arg(personaDisplayName());
    const QString assistantSpeaker = personaDisplayName();
    for (const QVariant &value : m_messages) {
        const QVariantMap item = value.toMap();
        const QString role = item.value(QStringLiteral("role")).toString();
        const QString speaker = role == QLatin1String("user") ? tr("我") : assistantSpeaker;
        out << QStringLiteral("**%1**：").arg(speaker) << item.value(QStringLiteral("text")).toString()
            << QStringLiteral("\n\n");
    }
    file.close();
    setStatus(tr("聊天记录已导出：%1").arg(path));
}

void ChatViewModel::openModelSettings()
{
    emit modelSettingsRequested();
}

void ChatViewModel::reloadMessages()
{
    const QVariantList history = m_chat->initialMessages();
    m_messages.clear();
    m_messages.reserve(history.size());
    for (const QVariant &value : history) {
        const QVariantMap item = value.toMap();
        m_messages.append(makeRow(item.value(QStringLiteral("role")).toString(),
                                  item.value(QStringLiteral("text")).toString(),
                                  item.value(QStringLiteral("id")).toLongLong(),
                                  item.value(QStringLiteral("feedback")).toInt(), QString()));
    }
    scheduleMessagesChanged();
}

void ChatViewModel::appendRow(const QString &role, const QString &text, qint64 id, int feedback)
{
    m_messages.append(makeRow(role, text, id, feedback, currentTimeText()));
}

void ChatViewModel::applyCardToRow(int row)
{
    if (row < 0 || row >= m_messages.size())
        return;
    QVariantMap item = m_messages.at(row).toMap();
    const QString text = item.value(QStringLiteral("text")).toString();
    QString title;
    QString body;
    const bool card = parseCard(text, &title, &body);
    item.insert(QStringLiteral("card"), card);
    item.insert(QStringLiteral("cardTitle"), title);
    item.insert(QStringLiteral("cardBody"), body);
    m_messages[row] = item;
}

QVariantMap ChatViewModel::makeRow(const QString &role, const QString &text, qint64 id, int feedback,
                                   const QString &time) const
{
    QString title;
    QString body;
    const bool card = parseCard(text, &title, &body);

    QVariantMap item;
    item.insert(QStringLiteral("id"), id);
    item.insert(QStringLiteral("role"), role);
    item.insert(QStringLiteral("text"), text);
    item.insert(QStringLiteral("feedback"), feedback);
    item.insert(QStringLiteral("card"), card);
    item.insert(QStringLiteral("cardTitle"), title);
    item.insert(QStringLiteral("cardBody"), body);
    item.insert(QStringLiteral("expanded"), false);
    item.insert(QStringLiteral("time"), time);
    return item;
}

bool ChatViewModel::parseCard(const QString &text, QString *title, QString *body)
{
    if (title != nullptr)
        title->clear();
    if (body != nullptr)
        body->clear();
    if (!text.startsWith(QStringLiteral("```card")))
        return false;

    // 与旧 Chat.qml 的 parseCard 保持一致：去掉围栏后的前导空白，再取最后一个 ``` 之前的内容。
    QString rest = text.mid(7);
    int begin = 0;
    while (begin < rest.size() && rest.at(begin).isSpace())
        ++begin;
    rest = rest.mid(begin);

    const int close = rest.lastIndexOf(QStringLiteral("```"));
    const QString inner = close >= 0 ? rest.left(close) : rest;
    const int newline = inner.indexOf(QLatin1Char('\n'));
    if (title != nullptr)
        *title = (newline >= 0 ? inner.left(newline) : inner).trimmed();
    if (body != nullptr)
        *body = (newline >= 0 ? inner.mid(newline + 1) : QString()).trimmed();
    return true;
}

void ChatViewModel::scheduleMessagesChanged()
{
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void ChatViewModel::flushMessagesChanged()
{
    emit messagesChanged();
}

void ChatViewModel::scheduleContextRefresh()
{
    m_contextTimer.start();
}

void ChatViewModel::setStreaming(bool streaming)
{
    if (m_streaming != streaming) {
        m_streaming = streaming;
        emit streamingChanged();
    }
    setBusy(streaming);
}

void ChatViewModel::applyCharacterContext()
{
    if (m_applyingContext)
        return;
    m_applyingContext = true;

    QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && !m_characters->contains(characterId)) {
        // 配置指向已删除的角色：清空选择，随后按「无角色」逻辑重建默认角色。
        characterId.clear();
        m_config->setCharacterId(QString());
        m_config->setPersonaId(QString());
        QString saveError;
        m_config->save(&saveError);
    }

    // 首次启动：没有角色时创建默认角色，保证对话上下文可用（与 Bridge 行为一致）。
    if (characterId.isEmpty() && m_config->values().characterId.isEmpty()) {
        const QList<Character> existing = m_characters->list();
        if (existing.isEmpty()) {
            QString newId;
            QString error;
            if (m_characters->create(QStringLiteral("小柚"), &newId, &error)) {
                m_config->setCharacterId(newId);
                m_config->setPersonaId(QString());
                QString saveError;
                m_config->save(&saveError);
                characterId = newId;
            } else {
                reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            }
        } else {
            characterId = existing.constFirst().id;
            m_config->setCharacterId(characterId);
            m_config->setPersonaId(QString());
            QString saveError;
            m_config->save(&saveError);
        }
    }

    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        const QString personaId = m_config->values().personaId;
        PersonaData persona;
        QString error;
        const bool hasPersona = !personaId.isEmpty()
                                && m_characters->persona(characterId, personaId, &persona, &error);
        if (!hasPersona)
            m_characters->card(characterId, &persona, &error);

        QString worldBody;
        const QString worldId = m_characters->worldId(characterId);
        if (!worldId.isEmpty()) {
            World world;
            if (m_worlds->read(worldId, &world, &error))
                worldBody = world.content;
        }

        PromptContext context;
        context.characterId = characterId;
        context.personaName = persona.name.isEmpty() ? characterId : persona.name;
        context.personaBody = persona.body;
        if (!persona.hobbies.isEmpty() || !persona.favoriteFood.isEmpty()) {
            context.situation = tr("爱好：%1；最爱的食物：%2")
                                    .arg(persona.hobbies, persona.favoriteFood);
        }
        context.worldBody = worldBody;
        context.constraints = PromptBuilder::defaultConstraints();

        const QString partition =
            QStringLiteral("character/%1/%2")
                .arg(characterId, personaId.isEmpty() ? QStringLiteral("default") : personaId);
        m_chat->setContext(partition, characterId, personaId, context);

        QString intimacyError;
        m_intimacy->load(characterId, &intimacyError);
        m_intimacy->setDecayEnabled(m_config->values().intimacyDecay);
        if (m_config->values().intimacyDecay)
            m_intimacy->applyDailyDecay();
    } else {
        PromptContext context;
        context.personaName = QStringLiteral("小柚");
        context.constraints = PromptBuilder::defaultConstraints();
        m_chat->setContext(QStringLiteral("default"), QString(), QString(), context);
    }

    m_lastCharacterId = m_config->values().characterId;
    m_lastPersonaId = m_config->values().personaId;

    m_applyingContext = false;
    updateHeader();
    setStreaming(false);
    setBusy(false);
}

void ChatViewModel::updateHeader()
{
    const AppConfig &values = m_config->values();
    const bool llmReady = !values.baseUrl.isEmpty() && !values.model.isEmpty();
    if (m_llmReady != llmReady) {
        m_llmReady = llmReady;
        emit llmReadyChanged();
    }

    const QString summary = llmReady
                                ? QStringLiteral("%1 · %2").arg(values.preset, values.model)
                                : tr("本地 mock（未配置模型）");
    if (m_modelSummary != summary) {
        m_modelSummary = summary;
        emit modelSummaryChanged();
    }

    const QString name = personaDisplayName();
    const bool intimacyLoaded = m_intimacy->loaded();
    const QString level = intimacyLoaded ? m_intimacy->levelName() : tr("陌生");
    const QString title = intimacyLoaded ? QStringLiteral("%1 · %2").arg(name, level) : name;
    if (m_chatTitle != title) {
        m_chatTitle = title;
        emit chatTitleChanged();
    }

    const QString welcome = tr("和 %1 聊点什么吧。").arg(name);
    if (m_welcomeText != welcome) {
        m_welcomeText = welcome;
        emit welcomeTextChanged();
    }

    const int score = intimacyLoaded ? m_intimacy->score() : 0;
    if (m_intimacyScore != score) {
        m_intimacyScore = score;
        emit intimacyScoreChanged();
    }
    if (m_intimacyLevel != level) {
        m_intimacyLevel = level;
        emit intimacyLevelChanged();
    }
}

QString ChatViewModel::personaDisplayName() const
{
    const AppConfig &values = m_config->values();
    const QString characterId = values.characterId;
    if (characterId.isEmpty())
        return values.personaId.isEmpty() ? QStringLiteral("小柚") : values.personaId;
    if (!m_characters->contains(characterId))
        return characterId;

    PersonaData persona;
    QString error;
    if (!values.personaId.isEmpty()
        && m_characters->persona(characterId, values.personaId, &persona, &error)
        && !persona.name.isEmpty()) {
        return persona.name;
    }
    if (m_characters->card(characterId, &persona, &error) && !persona.name.isEmpty())
        return persona.name;
    for (const Character &character : m_characters->list()) {
        if (character.id == characterId)
            return character.name;
    }
    return characterId;
}

QString ChatViewModel::dataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}
