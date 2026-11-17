#include "bridge.h"

#include <QApplication>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFile>
#include <QUrl>
#include <QProcess>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QImage>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QMenu>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

#include <stacklog/stacklog.hpp>

#include "chatservice.h"
#include "autostart.h"
#include "characterpack.h"
#include "characterstore.h"
#include "chatstore.h"
#include "errorcodes.h"
#include "hitareameta.h"
#include "intimacyservice.h"
#include "memorystore.h"
#include "personafile.h"
#include "promptbuilder.h"
#include "worldstore.h"
#include "configstore.h"
#include "personastore.h"

namespace
{
QJsonArray hitAreasToArray(const QJsonObject &map)
{
    QJsonArray array;
    for (const QJsonValue &value : map)
        array.append(value);
    return array;
}

QStringList idleLines()
{
    return {
        QStringLiteral("嗯？"),
        QStringLiteral("摸摸头～"),
        QStringLiteral("今天也要加油呀！"),
        QStringLiteral("要不要聊聊天？"),
        QStringLiteral("别戳啦，痒～"),
        QStringLiteral("我在呢。"),
    };
}

QStringList presetNames()
{
    return {
        QStringLiteral("Ollama"),
        QStringLiteral("LM Studio"),
        QStringLiteral("DeepSeek"),
        QStringLiteral("OpenAI"),
        QStringLiteral("OpenCode Go"),
        QStringLiteral("自定义"),
    };
}
} // namespace

Bridge::Bridge(QObject *parent)
    : QObject(parent)
    , m_config(new ConfigStore())
    , m_personas(new PersonaStore())
    , m_store(new ChatStore())
    , m_memory(new MemoryStore())
    , m_intimacy(new IntimacyService())
    , m_chat(new ChatService(m_config, m_personas, m_store, m_memory, m_intimacy, this))
    , m_characters(new CharacterStore())
    , m_worlds(new WorldStore())
{
    const QVector<QPointF> positions = m_config->values().petPositions;
    if (!positions.isEmpty() && positions.at(0).x() >= 0 && positions.at(0).y() >= 0) {
        m_petX = positions.at(0).x();
        m_petY = positions.at(0).y();
        m_hasSavedPos = true;
    } else {
        m_petX = 0;
        m_petY = 0;
        m_hasSavedPos = false;
    }

    m_bubbleTimer.setSingleShot(true);
    connect(&m_bubbleTimer, &QTimer::timeout, this, [this] { setBubbleVisible(false); });

    connect(m_chat, &ChatService::userMessage, this, [this](const QString &text) {
        emit chatMessage(QStringLiteral("user"), text);
    });
    connect(m_chat, &ChatService::delta, this, &Bridge::chatDelta);
    connect(m_chat, &ChatService::done, this, &Bridge::chatDone);
    connect(m_chat, &ChatService::failed, this, &Bridge::chatError);
    connect(m_chat, &ChatService::conversationCleared, this, &Bridge::conversationCleared);
    connect(m_chat, &ChatService::testFinished, this, [this](bool ok, const QString &message) {
        setTestResult((ok ? QStringLiteral("✅ ") : QStringLiteral("⚠️ ")) + message);
    });
    connect(m_chat, &ChatService::stateChanged, this, [this](const QString &state) {
        if (state == QLatin1String("thinking"))
            setStatusText(QStringLiteral("思考中…"));
        else if (state == QLatin1String("streaming"))
            setStatusText(QStringLiteral("回复中…"));
        else
            setStatusText(QString());
        emit chatStateChanged(state);
    });
    connect(m_characters, &CharacterStore::changed, this, &Bridge::refreshCharacters);
    connect(m_worlds, &WorldStore::changed, this, &Bridge::refreshWorlds);
    connect(m_memory, &MemoryStore::changed, this, &Bridge::memoriesChanged);
    connect(m_intimacy, &IntimacyService::changed, this, &Bridge::intimacyChanged);
    connect(m_chat, &ChatService::memoriesChanged, this, &Bridge::memoriesChanged);
    connect(m_chat, &ChatService::intimacyChanged, this, &Bridge::intimacyChanged);

    m_proactiveTimer.setSingleShot(true);
    connect(&m_proactiveTimer, &QTimer::timeout, this, [this] {
        if (!m_chat->busy() && m_petVisible && m_config->values().proactiveFrequency > 0
            && m_config->values().mode == 0) {
            const QStringList lines = idleLines();
            m_bubbleText = lines.at(QRandomGenerator::global()->bounded(lines.size()));
            emit bubbleTextChanged();
            positionBubble(m_petX, m_petY);
            setBubbleVisible(true);
            m_bubbleTimer.start(4200);
        }
        scheduleProactive();
    });
    scheduleProactive();

    m_ballX = m_config->values().ballX >= 0 ? m_config->values().ballX : 900;
    m_ballY = m_config->values().ballY >= 0 ? m_config->values().ballY : 600;

    ensureDefaultCharacter();
    applyCharacterContext();
    refreshCharacters();
    refreshWorlds();
}

Bridge::~Bridge() = default;

void Bridge::setPetVisible(bool visible)
{
    if (m_petVisible == visible)
        return;
    m_petVisible = visible;
    emit petVisibleChanged();
}

void Bridge::setChatVisible(bool visible)
{
    if (m_chatVisible == visible)
        return;
    m_chatVisible = visible;
    emit chatVisibleChanged();
}

void Bridge::setManagerPage(int page)
{
    if (m_managerPage == page)
        return;
    m_managerPage = page;
    emit managerPageChanged();
}

void Bridge::setManagerVisible(bool visible)
{
    if (m_managerVisible == visible)
        return;
    m_managerVisible = visible;
    emit managerVisibleChanged();
    if (visible)
        emit settingsChanged();
}

void Bridge::setBubbleVisible(bool visible)
{
    if (m_bubbleVisible == visible)
        return;
    m_bubbleVisible = visible;
    emit bubbleVisibleChanged();
}

void Bridge::setStatusText(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void Bridge::setTestResult(const QString &text)
{
    m_testResult = text;
    emit testResultChanged();
}

bool Bridge::busy() const
{
    return m_chat->busy();
}

bool Bridge::autoStart() const
{
    return autostart::enabled();
}

void Bridge::setAutoStart(bool enabled)
{
    autostart::setEnabled(enabled);
    emit autoStartChanged();
}

QVariantList Bridge::initialMessages() const
{
    return m_chat->initialMessages();
}

QString Bridge::personaName() const
{
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        PersonaData persona;
        QString error;
        const QString personaId = m_config->values().personaId;
        if (!personaId.isEmpty() && m_characters->persona(characterId, personaId, &persona, &error))
            return persona.name;
        if (m_characters->card(characterId, &persona, &error))
            return persona.name;
    }
    return m_personas->find(m_config->values().personaId).name;
}

QString Bridge::modelSummary() const
{
    if (!llmReady())
        return QStringLiteral("本地 mock（未配置模型）");
    return QStringLiteral("%1 · %2").arg(presetName(), modelName());
}

bool Bridge::llmReady() const
{
    return !m_config->values().baseUrl.isEmpty() && !m_config->values().model.isEmpty();
}

QString Bridge::presetName() const
{
    return m_config->values().preset;
}

QString Bridge::baseUrl() const
{
    return m_config->values().baseUrl;
}

QString Bridge::apiKey() const
{
    return m_config->values().apiKey;
}

QString Bridge::modelName() const
{
    return m_config->values().model;
}

QString Bridge::appVersion() const
{
#ifdef DESKTOP_COMPANION_VERSION
    return QStringLiteral(DESKTOP_COMPANION_VERSION);
#else
    return QStringLiteral("0.2.0");
#endif
}

QString Bridge::dataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Bridge::logDir() const
{
    return dataDir() + QStringLiteral("/logs");
}

QStringList Bridge::presets() const
{
    return presetNames();
}

QVariantList Bridge::personas() const
{
    QVariantList result;
    const QString characterId = m_config->values().characterId;
    const QString currentId = m_config->values().personaId;

    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        PersonaData card;
        QString error;
        if (m_characters->card(characterId, &card, &error)) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), QString());
            item.insert(QStringLiteral("name"), card.name.isEmpty() ? QStringLiteral("默认人设") : card.name);
            item.insert(QStringLiteral("builtin"), false);
            item.insert(QStringLiteral("current"), currentId.isEmpty());
            result.append(item);
        }
        for (const PersonaData &persona : m_characters->personas(characterId)) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), persona.id);
            item.insert(QStringLiteral("name"), persona.name);
            item.insert(QStringLiteral("builtin"), false);
            item.insert(QStringLiteral("current"), persona.id == currentId);
            result.append(item);
        }
        return result;
    }

    for (const Persona &persona : m_personas->personas()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), persona.id);
        item.insert(QStringLiteral("name"), persona.name);
        item.insert(QStringLiteral("builtin"), persona.builtin);
        item.insert(QStringLiteral("current"), persona.id == currentId || (currentId.isEmpty() && persona.builtin));
        result.append(item);
    }
    return result;
}

QVariantMap Bridge::presetDefaults(const QString &preset) const
{
    QVariantMap defaults;
    if (preset == QLatin1String("Ollama")) {
        defaults.insert(QStringLiteral("baseUrl"), QStringLiteral("http://127.0.0.1:11434/v1"));
        defaults.insert(QStringLiteral("model"), QStringLiteral("qwen2.5:7b"));
        defaults.insert(QStringLiteral("apiKey"), QString());
    } else if (preset == QLatin1String("LM Studio")) {
        defaults.insert(QStringLiteral("baseUrl"), QStringLiteral("http://127.0.0.1:1234/v1"));
        defaults.insert(QStringLiteral("model"), QStringLiteral("local-model"));
        defaults.insert(QStringLiteral("apiKey"), QString());
    } else if (preset == QLatin1String("DeepSeek")) {
        defaults.insert(QStringLiteral("baseUrl"), QStringLiteral("https://api.deepseek.com/v1"));
        defaults.insert(QStringLiteral("model"), QStringLiteral("deepseek-chat"));
        defaults.insert(QStringLiteral("apiKey"), QString());
    } else if (preset == QLatin1String("OpenAI")) {
        defaults.insert(QStringLiteral("baseUrl"), QStringLiteral("https://api.openai.com/v1"));
        defaults.insert(QStringLiteral("model"), QStringLiteral("gpt-4o-mini"));
        defaults.insert(QStringLiteral("apiKey"), QString());
    } else if (preset == QLatin1String("OpenCode Go")) {
        defaults.insert(QStringLiteral("baseUrl"), QStringLiteral("https://opencode.ai/zen/go/v1"));
        defaults.insert(QStringLiteral("model"), QStringLiteral("deepseek-v4.1-flash"));
        defaults.insert(QStringLiteral("apiKey"), QString());
    } else {
        defaults.insert(QStringLiteral("baseUrl"), m_config->values().baseUrl);
        defaults.insert(QStringLiteral("model"), m_config->values().model);
        defaults.insert(QStringLiteral("apiKey"), m_config->values().apiKey);
    }
    return defaults;
}

void Bridge::petClicked(int index, double x, double y, double width, double height)
{
    m_activePetIndex = index;
    const QPointF position = petPosition(index);
    m_petX = position.x();
    m_petY = position.y();
    const QVector<QString> petCharacters = m_config->values().petCharacters;
    if (index < petCharacters.size() && !petCharacters.at(index).isEmpty()
        && petCharacters.at(index) != m_config->values().characterId) {
        selectCharacter(petCharacters.at(index));
    }
    static const QStringList headLines = {
        QStringLiteral("别摸头啦～"),
        QStringLiteral("头好痒…"),
        QStringLiteral("摸头会长高的！"),
    };
    static const QStringList bodyLines = idleLines();
    static const QStringList feetLines = {
        QStringLiteral("别戳脚，站不稳啦！"),
        QStringLiteral("要摔倒了——"),
        QStringLiteral("脚底痒痒的～"),
    };

    QStringList areaLines;
    QString areaId;
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        CharacterMeta meta;
        QString error;
        if (m_characters->meta(characterId, &meta, &error) && !meta.hitAreas.isEmpty()) {
            HitAreaMeta hitAreaMeta;
            const QJsonObject root{{QStringLiteral("hitAreas"), hitAreasToArray(meta.hitAreas)}};
            if (hitAreaMeta.loadFromJson(QJsonDocument(root).toJson(), &error)) {
                const QPointF normalized(width > 0.0 ? x / width : 0.5,
                                         height > 0.0 ? y / height : 0.5);
                areaId = hitAreaMeta.hitAreaAt(normalized);
                if (const HitArea *area = hitAreaMeta.find(areaId))
                    areaLines = area->lines;
            }
        }
    }

    const double ratio = height > 0.0 ? y / height : 0.5;
    const QStringList *lines = &bodyLines;
    if (ratio < 0.32)
        lines = &headLines;
    else if (ratio > 0.72)
        lines = &feetLines;
    STACKLOG_DEBUG() << "pet clicked at " << x << "," << y << " area=" << areaId.toStdString()
                     << " ratio=" << ratio;

    QStringList chosen = areaLines.isEmpty() ? *lines : areaLines;
    if (m_intimacy->loaded()) {
        const int score = m_intimacy->score();
        if (score >= IntimacyService::trustedThreshold)
            chosen.append(QStringLiteral("最喜欢你了！"));
        else if (score >= IntimacyService::closeThreshold)
            chosen.append(QStringLiteral("今天也一起玩吧～"));
        m_intimacy->add(1, QStringLiteral("点击"));
        emit intimacyChanged();
    }
    m_bubbleText = chosen.at(QRandomGenerator::global()->bounded(chosen.size()));
    emit bubbleTextChanged();
    positionBubble(m_petX, m_petY);
    setBubbleVisible(true);
    m_bubbleTimer.start(4200);
    scheduleProactive();
}

void Bridge::positionBubble(double x, double y)
{
    m_bubbleX = x + 40;
    m_bubbleY = (y - 110 < 10) ? y + 330 : y - 110;
    emit bubblePosChanged();
}

void Bridge::setPetPosition(int index, double x, double y)
{
    m_config->setPetPosition(index, x, y);
    m_config->save();
    m_hasSavedPos = true;
    if (index == 0) {
        m_petX = x;
        m_petY = y;
        emit petPosChanged();
    }
    emit petLayoutChanged();
}

QPointF Bridge::petPosition(int index) const
{
    const QVector<QPointF> positions = m_config->values().petPositions;
    if (index >= 0 && index < positions.size() && positions.at(index).x() >= 0
        && positions.at(index).y() >= 0) {
        return positions.at(index);
    }
    return QPointF(m_petX + index * 90.0, m_petY + index * 40.0);
}

QVariantList Bridge::pets() const
{
    QVariantList result;
    const int count = m_config->values().petCount;
    const QVector<QString> characterIds = m_config->values().petCharacters;
    for (int i = 0; i < count; ++i) {
        const QPointF position = petPosition(i);
        QString avatar = currentAvatarPath();
        QString characterId;
        if (i < characterIds.size() && !characterIds.at(i).isEmpty()
            && m_characters->contains(characterIds.at(i))) {
            characterId = characterIds.at(i);
            const QString custom = m_characters->primaryAvatar(characterId);
            if (!custom.isEmpty())
                avatar = QUrl::fromLocalFile(custom).toString();
        }
        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("x"), position.x());
        item.insert(QStringLiteral("y"), position.y());
        item.insert(QStringLiteral("characterId"), characterId);
        item.insert(QStringLiteral("avatar"), avatar);
        result.append(item);
    }
    return result;
}

int Bridge::petCount() const
{
    return m_config->values().petCount;
}

void Bridge::setPetCount(int count)
{
    m_config->setPetCount(count);
    m_config->save();
    emit petLayoutChanged();
}

void Bridge::openChat()
{
    STACKLOG_INFO() << "open chat window";
    setChatVisible(true);
    setBubbleVisible(false);
}

void Bridge::closeChat()
{
    setChatVisible(false);
}

void Bridge::sendChat(const QString &text)
{
    // 日志脱敏：只记录长度，不写用户原文。
    STACKLOG_INFO() << "chat send: " << text.size() << " chars";
    scheduleProactive();
    m_chat->send(text);
}

void Bridge::stopReply()
{
    m_chat->stop();
}

void Bridge::clearConversation()
{
    STACKLOG_INFO() << "clear conversation";
    m_chat->clearConversation();
    setStatusText(QString());
}

void Bridge::openManager()
{
    setManagerVisible(true);
}

void Bridge::closeManager()
{
    setManagerVisible(false);
}

void Bridge::saveLlmSettings(const QString &preset, const QString &baseUrl, const QString &apiKey,
                             const QString &model)
{
    m_config->setLlm(preset, baseUrl.trimmed(), apiKey.trimmed(), model.trimmed());
    QString error;
    if (!m_config->save(&error))
        STACKLOG_ERROR() << "save config failed: " << error.toStdString();
    emit settingsChanged();
    STACKLOG_INFO() << "llm settings saved, preset=" << preset.toStdString();
}

void Bridge::testLlmConnection(const QString &baseUrl, const QString &apiKey, const QString &model)
{
    setTestResult(QStringLiteral("测试中…"));
    m_chat->testConnection(baseUrl, apiKey, model);
}

void Bridge::importPersona()
{
    const QString path = QFileDialog::getOpenFileName(nullptr,
                                                      QStringLiteral("导入人设（Markdown）"),
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!m_personas->importFrom(path, &error)) {
        setStatusText(error);
        STACKLOG_WARN() << "import persona failed: " << error.toStdString();
        return;
    }
    emit personasChanged();
    setStatusText(QStringLiteral("已导入人设：%1").arg(QFileInfo(path).completeBaseName()));
}

void Bridge::removePersona(const QString &personaId)
{
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        if (personaId.isEmpty())
            return;
        QString error;
        if (!m_characters->removePersona(characterId, personaId, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        if (m_config->values().personaId == personaId) {
            m_config->setPersonaId(QString());
            m_config->save();
        }
        applyCharacterContext();
        emit personasChanged();
        emit settingsChanged();
        return;
    }

    if (!m_personas->remove(personaId))
        return;
    if (m_config->values().personaId == personaId) {
        m_config->setPersonaId(QString());
        m_config->save();
    }
    emit personasChanged();
    emit settingsChanged();
}

void Bridge::selectPersona(const QString &personaId)
{
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        m_config->setPersonaId(personaId);
        m_config->save();
        applyCharacterContext();
        emit settingsChanged();
        emit personasChanged();
        setStatusText(QStringLiteral("已切换人设：%1").arg(personaName()));
        return;
    }

    m_config->setPersonaId(personaId);
    m_config->save();
    applyCharacterContext();
    emit settingsChanged();
    emit personasChanged();
    setStatusText(QStringLiteral("已切换人设：%1").arg(personaName()));
}

void Bridge::togglePet()
{
    STACKLOG_DEBUG() << "toggle pet visibility";
    setPetVisible(!m_petVisible);
}

void Bridge::notifyAlreadyRunning()
{
    m_bubbleText = QStringLiteral("我已经在桌面上啦～");
    emit bubbleTextChanged();
    positionBubble(m_petX, m_petY);
    setBubbleVisible(true);
    m_bubbleTimer.start(4200);
}

void Bridge::showPetMenu(int index)
{
    m_activePetIndex = index;
    QMenu menu;
    QAction *chatAction = menu.addAction(QStringLiteral("打开聊天"));
    QAction *characterAction = menu.addAction(QStringLiteral("切换角色"));
    QAction *settingsAction = menu.addAction(QStringLiteral("设置…"));
    QAction *throughAction = menu.addAction(m_config->values().clickThrough
                                                ? QStringLiteral("关闭点击穿透")
                                                : QStringLiteral("开启点击穿透"));
    QAction *hideAction = menu.addAction(QStringLiteral("隐藏桌宠"));
    menu.addSeparator();
    QAction *quitAction = menu.addAction(QStringLiteral("退出"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == chatAction)
        openChat();
    else if (chosen == characterAction)
        cycleCharacter();
    else if (chosen == settingsAction)
        openManager();
    else if (chosen == throughAction)
        setClickThrough(!m_config->values().clickThrough);
    else if (chosen == hideAction)
        setPetVisible(false);
    else if (chosen == quitAction)
        quitApp();
}

void Bridge::openDataFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(dataDir()));
}

void Bridge::openLogFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir()));
}


QString Bridge::currentCharacterId() const
{
    return m_config->values().characterId;
}

QString Bridge::currentPersonaId() const
{
    return m_config->values().personaId;
}

QVariantList Bridge::characters() const
{
    QVariantList result;
    const QString currentId = m_config->values().characterId;
    for (const Character &character : m_characters->list()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), character.id);
        item.insert(QStringLiteral("name"), character.name);
        item.insert(QStringLiteral("worldId"), character.worldId);
        item.insert(QStringLiteral("avatar"), character.avatarPath);
        item.insert(QStringLiteral("enabled"), character.enabled);
        item.insert(QStringLiteral("current"), character.id == currentId);
        result.append(item);
    }
    return result;
}

QVariantList Bridge::worlds() const
{
    QVariantList result;
    QString currentWorld = QString();
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty())
        currentWorld = m_characters->worldId(characterId);

    for (const World &world : m_worlds->list()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), world.id);
        item.insert(QStringLiteral("name"), world.name);
        item.insert(QStringLiteral("current"), world.id == currentWorld);
        World full;
        if (m_worlds->read(world.id, &full))
            item.insert(QStringLiteral("body"), full.content);
        result.append(item);
    }
    return result;
}

QString Bridge::currentCharacterName() const
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty())
        return personaName();
    for (const Character &character : m_characters->list()) {
        if (character.id == characterId)
            return character.name;
    }
    return characterId;
}

void Bridge::refreshCharacters()
{
    emit charactersChanged();
    emit avatarPathChanged();
    emit petLayoutChanged();
}

void Bridge::refreshWorlds()
{
    emit worldsChanged();
}

void Bridge::reportInfo(const QString &text)
{
    setStatusText(text);
}

void Bridge::reportError(quint32 code, const QString &detail)
{
    const AppError error = errors::make(static_cast<ErrorCode>(code), detail);
    setStatusText(error.toString());
    STACKLOG_WARN() << error.toString().toStdString();
}

void Bridge::ensureDefaultCharacter()
{
    if (!m_config->values().characterId.isEmpty() || !m_characters->list().isEmpty())
        return;

    QString newId;
    QString error;
    if (!m_characters->create(QStringLiteral("小柚"), &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    m_config->setCharacterId(newId);
    m_config->save();
    STACKLOG_INFO() << "created default character: " << newId.toStdString();
}

QString Bridge::buildCharacterPrompt(const QString &characterId, const QString &personaId) const
{
    PersonaData persona;
    QString error;
    const bool hasPersona = !personaId.isEmpty()
                           && m_characters->persona(characterId, personaId, &persona, &error);
    if (!hasPersona && !m_characters->card(characterId, &persona, &error))
        return QString();

    QString worldBody;
    const QString worldId = m_characters->worldId(characterId);
    if (!worldId.isEmpty()) {
        World world;
        if (m_worlds->read(worldId, &world, &error))
            worldBody = world.content;
    }

    PromptContext context;
    context.characterId = characterId;
    context.personaName = persona.name;
    context.personaBody = persona.body;
    if (context.personaName.isEmpty())
        context.personaName = characterId;
    if (!persona.hobbies.isEmpty())
        context.situation = QStringLiteral("爱好：%1；最爱的食物：%2").arg(persona.hobbies, persona.favoriteFood);
    context.worldBody = worldBody;
    context.constraints = PromptBuilder::defaultConstraints();
    return PromptBuilder::build(context);
}

void Bridge::applyCharacterContext()
{
    const QString characterId = m_config->values().characterId;
    const QString personaId = m_config->values().personaId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
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
            context.situation = QStringLiteral("爱好：%1；最爱的食物：%2")
                                    .arg(persona.hobbies, persona.favoriteFood);
        }
        context.worldBody = worldBody;
        context.constraints = PromptBuilder::defaultConstraints();

        const QString partition = QStringLiteral("character/%1/%2")
                                      .arg(characterId, personaId.isEmpty() ? QStringLiteral("default") : personaId);
        m_chat->setContext(partition, characterId, personaId, context);

        QString intimacyError;
        m_intimacy->load(characterId, &intimacyError);
        emit intimacyChanged();
        emit avatarPathChanged();
    } else {
        const Persona legacy = m_personas->find(personaId);
        PromptContext context;
        context.personaName = legacy.name;
        context.personaBody = legacy.prompt;
        context.constraints = PromptBuilder::defaultConstraints();
        m_chat->setContext(personaId, QString(), personaId, context);
    }
    emit settingsChanged();
}

void Bridge::createCharacter(const QString &name)
{
    QString newId;
    QString error;
    if (!m_characters->create(name, &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    m_config->setCharacterId(newId);
    m_config->setPersonaId(QString());
    m_config->save();
    applyCharacterContext();
    refreshCharacters();
    reportInfo(QStringLiteral("已创建角色：%1").arg(name));
}

void Bridge::renameCharacter(const QString &id, const QString &newName)
{
    QString error;
    if (!m_characters->rename(id, newName, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshCharacters();
    emit settingsChanged();
    reportInfo(QStringLiteral("已重命名角色"));
}

void Bridge::removeCharacter(const QString &id)
{
    QString error;
    if (!m_characters->remove(id, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    if (m_config->values().characterId == id) {
        m_config->setCharacterId(QString());
        m_config->setPersonaId(QString());
        m_config->save();
        applyCharacterContext();
    }
    refreshCharacters();
    reportInfo(QStringLiteral("已删除角色"));
}

void Bridge::selectCharacter(const QString &id)
{
    if (!m_characters->contains(id)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("角色不存在：%1").arg(id));
        return;
    }
    m_config->setCharacterId(id);
    m_config->setPersonaId(QString());
    m_config->save();
    applyCharacterContext();
    refreshCharacters();
    refreshWorlds();
}

void Bridge::importAvatarFile()
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("选择角色形象"),
                                                      QString(), QStringLiteral("图片 (*.png *.jpg *.jpeg *.webp *.gif *.bmp)"));
    if (path.isEmpty())
        return;
    QString storedPath;
    QString error;
    if (!m_characters->importAvatar(characterId, path, &storedPath, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidImage), error);
        return;
    }
    refreshCharacters();
    const QImage image(path);
    if (!image.isNull() && !image.hasAlphaChannel())
        reportInfo(QStringLiteral("已更新角色形象；图片无透明通道，点击穿透会按整张图矩形生效"));
    else
        reportInfo(QStringLiteral("已更新角色形象"));
}

QVariantMap Bridge::personaFields(const QString &personaId) const
{
    QVariantMap map;
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty())
        return map;

    PersonaData persona;
    QString error;
    const bool ok = personaId.isEmpty() ? m_characters->card(characterId, &persona, &error)
                                        : m_characters->persona(characterId, personaId, &persona, &error);
    if (!ok)
        return map;

    map.insert(QStringLiteral("id"), personaId);
    map.insert(QStringLiteral("name"), persona.name);
    map.insert(QStringLiteral("birthday"), persona.birthday);
    map.insert(QStringLiteral("zodiac"), persona.zodiac);
    map.insert(QStringLiteral("hobbies"), persona.hobbies);
    map.insert(QStringLiteral("favoriteFood"), persona.favoriteFood);
    map.insert(QStringLiteral("background"), persona.background);
    map.insert(QStringLiteral("body"), persona.body);
    return map;
}

void Bridge::savePersonaFields(const QVariantMap &fields)
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }

    PersonaData data;
    data.id = fields.value(QStringLiteral("id")).toString();
    data.name = fields.value(QStringLiteral("name")).toString().trimmed();
    data.birthday = fields.value(QStringLiteral("birthday")).toString();
    data.zodiac = fields.value(QStringLiteral("zodiac")).toString();
    data.hobbies = fields.value(QStringLiteral("hobbies")).toString();
    data.favoriteFood = fields.value(QStringLiteral("favoriteFood")).toString();
    data.background = fields.value(QStringLiteral("background")).toString();
    data.body = fields.value(QStringLiteral("body")).toString();

    QString error;
    if (data.id.isEmpty()) {
        if (!m_characters->saveCard(characterId, data, &error)) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
    } else {
        if (!m_characters->savePersona(characterId, data, nullptr, &error)) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
    }
    applyCharacterContext();
    emit personasChanged();
    reportInfo(QStringLiteral("人设已保存"));
}

void Bridge::createPersona(const QString &name)
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }
    PersonaData data;
    data.name = name.trimmed().isEmpty() ? QStringLiteral("新人设") : name.trimmed();
    QString personaId;
    QString error;
    if (!m_characters->savePersona(characterId, data, &personaId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
        return;
    }
    m_config->setPersonaId(personaId);
    m_config->save();
    applyCharacterContext();
    emit personasChanged();
    reportInfo(QStringLiteral("已创建人设：%1").arg(data.name));
}

void Bridge::importPersonaFile()
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("导入人设（Markdown）"),
                                                      QString(), QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty())
        return;

    PersonaData data;
    QString error;
    if (!parsePersonaFile(path, &data, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
        return;
    }
    for (const PersonaData &existing : m_characters->personas(characterId)) {
        if (existing.name == data.name) {
            reportError(static_cast<quint32>(ErrorCode::FileNameConflict),
                        QStringLiteral("人设已存在：%1").arg(data.name));
            return;
        }
    }
    QString personaId;
    if (!m_characters->savePersona(characterId, data, &personaId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
        return;
    }
    m_config->setPersonaId(personaId);
    m_config->save();
    applyCharacterContext();
    emit personasChanged();
    reportInfo(QStringLiteral("已导入人设：%1").arg(data.name));
}

void Bridge::createWorld(const QString &name)
{
    QString newId;
    QString error;
    if (!m_worlds->create(name, &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshWorlds();
    reportInfo(QStringLiteral("已创建世界观：%1").arg(name));
}

void Bridge::renameWorld(const QString &id, const QString &newName)
{
    QString error;
    if (!m_worlds->rename(id, newName, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshWorlds();
    reportInfo(QStringLiteral("世界观已重命名"));
}

void Bridge::removeWorld(const QString &id)
{
    QString error;
    if (!m_worlds->remove(id, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshWorlds();
    reportInfo(QStringLiteral("世界观已删除"));
}

void Bridge::saveWorldBody(const QString &id, const QString &body)
{
    World world;
    QString error;
    if (!m_worlds->read(id, &world, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileReadFailed), error);
        return;
    }
    world.content = body;
    if (!m_worlds->write(id, world, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshWorlds();
    applyCharacterContext();
    emit worldsChanged();
    reportInfo(QStringLiteral("世界观已保存"));
}

void Bridge::importWorldFile()
{
    const QString path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("导入世界观（Markdown）"),
                                                      QString(), QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty())
        return;
    QString newId;
    QString error;
    if (!m_worlds->importFrom(path, &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshWorlds();
    reportInfo(QStringLiteral("已导入世界观"));
}

void Bridge::assignCharacterWorld(const QString &worldId)
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }
    QString error;
    if (!m_characters->setWorldId(characterId, worldId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    applyCharacterContext();
    refreshWorlds();
    refreshCharacters();
    reportInfo(QStringLiteral("已更新角色世界观"));
}

QString Bridge::currentCharacterDir() const
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty())
        return QString();
    return m_characters->characterDir(characterId);
}

void Bridge::exportCharacterPack()
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色"));
        return;
    }
    const QString defaultName = characterId + QStringLiteral("-pack.zip");
    const QString path = QFileDialog::getSaveFileName(nullptr, QStringLiteral("导出角色包"),
                                                      QDir::homePath() + QLatin1Char('/') + defaultName,
                                                      QStringLiteral("Zip (*.zip)"));
    if (path.isEmpty())
        return;
    QString error;
    if (!characterpack::exportPack(m_characters->characterDir(characterId), path, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    reportInfo(QStringLiteral("角色包已导出：%1").arg(path));
}

void Bridge::importCharacterPack()
{
    const QString path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("导入角色包"),
                                                      QString(), QStringLiteral("Zip (*.zip)"));
    if (path.isEmpty())
        return;
    QString newId;
    QString error;
    if (!characterpack::importPack(path, m_characters->charactersDir(), &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::IncompleteCharacterPack), error);
        return;
    }
    refreshCharacters();
    reportInfo(QStringLiteral("已导入角色：%1").arg(newId));
}

void Bridge::exportChatHistory()
{
    if (m_store == nullptr) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), QStringLiteral("聊天库不可用"));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(nullptr, QStringLiteral("导出聊天记录"),
                                                      QDir::homePath() + QStringLiteral("/chat.md"),
                                                      QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), file.errorString());
        return;
    }
    QTextStream out(&file);
    out << QStringLiteral("# 与 %1 的聊天记录\n\n").arg(currentCharacterName());
    for (const StoredMessage &message : m_store->recentMessages(10000)) {
        const QString speaker = message.role == QLatin1String("user") ? QStringLiteral("我") : currentCharacterName();
        out << QStringLiteral("**%1**：").arg(speaker) << message.content << QStringLiteral("\n\n");
    }
    file.close();
    reportInfo(QStringLiteral("聊天记录已导出：%1").arg(path));
}

void Bridge::exportLogs()
{
    const QString targetDir = QFileDialog::getExistingDirectory(nullptr, QStringLiteral("选择日志导出目录"));
    if (targetDir.isEmpty())
        return;

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString dest = targetDir + QStringLiteral("/desktop_companion-logs-") + stamp;
    if (!QDir().mkpath(dest)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), dest);
        return;
    }

    int copied = 0;
    QDirIterator iterator(logDir(), QDir::Files);
    while (iterator.hasNext()) {
        const QString source = iterator.next();
        const QString target = dest + QLatin1Char('/') + QFileInfo(source).fileName();
        if (QFile::copy(source, target))
            ++copied;
    }
    reportInfo(QStringLiteral("已导出 %1 个日志文件到 %2").arg(copied).arg(dest));
}


QVariantList Bridge::memories() const
{
    QVariantList result;
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty() || !m_memory->ready())
        return result;
    for (const MemoryEntry &entry : m_memory->list(characterId, m_config->values().personaId)) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), entry.id);
        item.insert(QStringLiteral("content"), entry.content);
        item.insert(QStringLiteral("pinned"), entry.pinned);
        item.insert(QStringLiteral("tags"), entry.tags.join(QStringLiteral(", ")));
        item.insert(QStringLiteral("createdAt"), entry.createdAt);
        result.append(item);
    }
    return result;
}

int Bridge::intimacyScore() const
{
    return m_intimacy->loaded() ? m_intimacy->score() : 0;
}

QString Bridge::intimacyLevel() const
{
    return m_intimacy->loaded() ? m_intimacy->levelName() : QStringLiteral("陌生");
}

bool Bridge::autoExtractMemories() const
{
    return m_config->values().autoExtractMemories;
}

void Bridge::setAutoExtractMemories(bool enabled)
{
    m_config->setMemoryPrefs(enabled, m_config->values().proactiveFrequency);
    m_config->save();
    emit settingsChanged();
}

int Bridge::proactiveFrequency() const
{
    return m_config->values().proactiveFrequency;
}

void Bridge::setProactiveFrequency(int frequency)
{
    m_config->setMemoryPrefs(m_config->values().autoExtractMemories, qBound(0, frequency, 3));
    m_config->save();
    scheduleProactive();
    emit settingsChanged();
}

bool Bridge::idleMotion() const
{
    return m_config->values().idleMotion;
}

void Bridge::setIdleMotion(bool enabled)
{
    m_config->setPetBehavior(enabled, m_config->values().idleAmplitude,
                             m_config->values().clickThrough, m_config->values().debugHitAreas);
    m_config->save();
    emit settingsChanged();
}

double Bridge::idleAmplitude() const
{
    return m_config->values().idleAmplitude;
}

void Bridge::setIdleAmplitude(double amplitude)
{
    m_config->setPetBehavior(m_config->values().idleMotion, qBound(0.0, amplitude, 3.0),
                             m_config->values().clickThrough, m_config->values().debugHitAreas);
    m_config->save();
    emit settingsChanged();
}

bool Bridge::clickThrough() const
{
    return m_config->values().clickThrough;
}

void Bridge::setClickThrough(bool enabled)
{
    m_config->setPetBehavior(m_config->values().idleMotion, m_config->values().idleAmplitude,
                             enabled, m_config->values().debugHitAreas);
    m_config->save();
    emit settingsChanged();
}

bool Bridge::debugHitAreas() const
{
    return m_config->values().debugHitAreas;
}

void Bridge::setDebugHitAreas(bool enabled)
{
    m_config->setPetBehavior(m_config->values().idleMotion, m_config->values().idleAmplitude,
                             m_config->values().clickThrough, enabled);
    m_config->save();
    emit settingsChanged();
}

bool Bridge::edgeHide() const
{
    return m_config->values().edgeHide;
}

void Bridge::setEdgeHide(bool enabled)
{
    m_config->setEdgeHide(enabled);
    m_config->save();
    emit settingsChanged();
}

QString Bridge::currentAvatarPath() const
{
    const QString characterId = m_config->values().characterId;
    if (!characterId.isEmpty() && m_characters->contains(characterId)) {
        const QString avatar = m_characters->primaryAvatar(characterId);
        if (!avatar.isEmpty())
            return QUrl::fromLocalFile(avatar).toString();
    }
    return QStringLiteral("qrc:/pet.png");
}

QVariantList Bridge::hitAreaRects() const
{
    QVariantList result;
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty() || !m_characters->contains(characterId))
        return result;
    CharacterMeta meta;
    QString error;
    const bool hasMeta = m_characters->meta(characterId, &meta, &error);
    STACKLOG_DEBUG() << "hitAreaRects: hasMeta=" << hasMeta << " keys=" << meta.hitAreas.keys().size()
                     << " error=" << error.toStdString();
    if (!hasMeta || meta.hitAreas.isEmpty())
        return result;

    HitAreaMeta hitAreaMeta;
    const QJsonObject root{{QStringLiteral("hitAreas"), hitAreasToArray(meta.hitAreas)}};
    if (!hitAreaMeta.loadFromJson(QJsonDocument(root).toJson(), &error)) {
        STACKLOG_WARN() << "hitAreaRects parse failed: " << error.toStdString();
        return result;
    }
    STACKLOG_DEBUG() << "hitAreaRects: areas=" << hitAreaMeta.areaCount();
    for (const HitArea &area : hitAreaMeta.areas()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), area.id);
        item.insert(QStringLiteral("name"), area.name);
        item.insert(QStringLiteral("x"), area.rect.x());
        item.insert(QStringLiteral("y"), area.rect.y());
        item.insert(QStringLiteral("w"), area.rect.width());
        item.insert(QStringLiteral("h"), area.rect.height());
        result.append(item);
    }
    return result;
}

void Bridge::cycleCharacter()
{
    const QList<Character> list = m_characters->list();
    if (list.isEmpty()) {
        reportInfo(QStringLiteral("还没有角色"));
        return;
    }
    const QVector<QString> petCharacters = m_config->values().petCharacters;
    QString current;
    if (m_activePetIndex >= 0 && m_activePetIndex < petCharacters.size())
        current = petCharacters.at(m_activePetIndex);
    if (current.isEmpty())
        current = m_config->values().characterId;

    int index = 0;
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).id == current) {
            index = (i + 1) % list.size();
            break;
        }
    }
    m_config->setPetCharacter(m_activePetIndex, list.at(index).id);
    m_config->save();
    emit petLayoutChanged();
    reportInfo(QStringLiteral("桌宠 %1 已切换到：%2").arg(m_activePetIndex + 1).arg(list.at(index).name));
}

int Bridge::mode() const
{
    return m_config->values().mode;
}

void Bridge::setMode(int mode)
{
    m_config->setMode(mode);
    m_config->save();
    scheduleProactive();
    emit settingsChanged();
}

bool Bridge::ballVisible() const
{
    return m_config->values().ballVisible;
}

void Bridge::setBallVisible(bool visible)
{
    m_config->setBall(visible, m_ballX, m_ballY);
    m_config->save();
    emit settingsChanged();
}

bool Bridge::onboardingDone() const
{
    return m_config->values().onboardingDone;
}

void Bridge::setOnboardingDone(bool done)
{
    m_config->setOnboardingDone(done);
    m_config->save();
    emit settingsChanged();
}

void Bridge::setBallPosition(double x, double y)
{
    m_ballX = x;
    m_ballY = y;
    m_config->setBall(m_config->values().ballVisible, x, y);
    m_config->save();
    emit ballPosChanged();
}

void Bridge::toggleMuteMode()
{
    setMode(m_config->values().mode == 2 ? 0 : 2);
    reportInfo(m_config->values().mode == 2 ? QStringLiteral("已进入勿扰模式")
                                           : QStringLiteral("已退出勿扰模式"));
}

int Bridge::language() const
{
    return m_config->values().language;
}

void Bridge::setLanguage(int language)
{
    m_config->setLanguage(language);
    m_config->save();
    emit settingsChanged();
}

void Bridge::restartApp()
{
    m_config->save();
    const QString exe = QCoreApplication::applicationFilePath();
    QProcess::startDetached(QStringLiteral("cmd.exe"),
                            {QStringLiteral("/c"), QStringLiteral("start"), QString(),
                             QDir::toNativeSeparators(exe)});
    QTimer::singleShot(150, this, [this] { quitApp(); });
}

double Bridge::temperature() const
{
    return m_config->values().temperature;
}

void Bridge::setTemperature(double temperature)
{
    m_config->setGeneration(temperature, m_config->values().maxTokens);
    m_config->save();
    emit settingsChanged();
}

int Bridge::maxTokens() const
{
    return m_config->values().maxTokens;
}

void Bridge::setMaxTokens(int maxTokens)
{
    m_config->setGeneration(m_config->values().temperature, maxTokens);
    m_config->save();
    emit settingsChanged();
}

int Bridge::theme() const
{
    return m_config->values().theme;
}

void Bridge::setTheme(int theme)
{
    m_config->setTheme(theme);
    m_config->save();
    emit settingsChanged();
}

bool Bridge::darkTheme() const
{
    if (m_config->values().theme == 1)
        return true;
    if (m_config->values().theme == 2) {
        return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
    }
    return false;
}

QString Bridge::windowColor() const
{
    return darkTheme() ? QStringLiteral("#1e1e1e") : QStringLiteral("#faf9f6");
}

QString Bridge::cardColor() const
{
    return darkTheme() ? QStringLiteral("#2a2a2a") : QStringLiteral("#ffffff");
}

QString Bridge::borderColor() const
{
    return darkTheme() ? QStringLiteral("#3d3d3d") : QStringLiteral("#e2ded6");
}

QString Bridge::textColor() const
{
    return darkTheme() ? QStringLiteral("#e8e2d8") : QStringLiteral("#4a3b2e");
}

QString Bridge::secondaryTextColor() const
{
    return darkTheme() ? QStringLiteral("#a89f90") : QStringLiteral("#8a7a60");
}

void Bridge::checkUpdate()
{
    const QString url = m_config->values().updateUrl;
    if (url.isEmpty()) {
        m_updateResult = QStringLiteral("未配置更新地址（config.json 的 updateUrl 可填版本清单 URL）");
        emit updateResultChanged();
        setStatusText(m_updateResult);
        return;
    }
    m_updateResult = QStringLiteral("正在检查更新…");
    emit updateResultChanged();

    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(8000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_updateResult = QStringLiteral("检查失败：%1").arg(reply->errorString());
            emit updateResultChanged();
            return;
        }
        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            m_updateResult = QStringLiteral("检查失败：版本清单格式错误");
            emit updateResultChanged();
            return;
        }
        const QJsonObject root = document.object();
        const QString latest = root.value(QStringLiteral("version")).toString();
        const QVersionNumber current = QVersionNumber::fromString(QCoreApplication::applicationVersion());
        const QVersionNumber remote = QVersionNumber::fromString(latest);
        if (!remote.isNull() && remote > current) {
            m_updateResult = QStringLiteral("发现新版本 %1：%2")
                                 .arg(latest, root.value(QStringLiteral("url")).toString());
        } else {
            m_updateResult = QStringLiteral("已是最新版本（%1）").arg(current.toString());
        }
        emit updateResultChanged();
    });
}

void Bridge::scheduleProactive()
{
    m_proactiveTimer.stop();
    const int frequency = m_config->values().proactiveFrequency;
    if (frequency <= 0 || m_config->values().mode != 0)
        return;
    int intervalMs = 10 * 60 * 1000;
    if (frequency == 2)
        intervalMs = 5 * 60 * 1000;
    else if (frequency == 3)
        intervalMs = 2 * 60 * 1000;
    m_proactiveTimer.start(intervalMs);
}

void Bridge::addMemory(const QString &content)
{
    const QString characterId = m_config->values().characterId;
    const QString trimmed = content.trimmed();
    if (characterId.isEmpty() || trimmed.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), QStringLiteral("请先选择角色并填写记忆内容"));
        return;
    }
    MemoryEntry entry;
    entry.characterId = characterId;
    entry.personaId = m_config->values().personaId;
    entry.content = trimmed;
    QString error;
    if (!m_memory->add(&entry, &error)) {
        reportError(static_cast<quint32>(ErrorCode::MemoryLimitReached), error);
        return;
    }
    emit memoriesChanged();
    reportInfo(QStringLiteral("已添加记忆"));
}

void Bridge::updateMemory(qint64 id, const QString &content)
{
    if (content.trimmed().isEmpty())
        return;
    const QString characterId = m_config->values().characterId;
    for (const MemoryEntry &entry : m_memory->list(characterId, m_config->values().personaId)) {
        if (entry.id != id)
            continue;
        MemoryEntry updated = entry;
        updated.content = content.trimmed();
        QString error;
        if (!m_memory->update(updated, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        emit memoriesChanged();
        reportInfo(QStringLiteral("记忆已更新"));
        return;
    }
}

void Bridge::removeMemory(qint64 id)
{
    QString error;
    if (!m_memory->remove(id, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    emit memoriesChanged();
}

void Bridge::toggleMemoryPin(qint64 id)
{
    const QString characterId = m_config->values().characterId;
    for (const MemoryEntry &entry : m_memory->list(characterId, m_config->values().personaId)) {
        if (entry.id != id)
            continue;
        QString error;
        if (!m_memory->setPinned(id, !entry.pinned, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        emit memoriesChanged();
        return;
    }
}

QString Bridge::feedbackSummary() const
{
    int likes = 0;
    int dislikes = 0;
    m_store->feedbackStats(&likes, &dislikes);
    return QStringLiteral("%1 / %2").arg(likes).arg(dislikes);
}

void Bridge::setMessageFeedback(qint64 messageId, int value)
{
    if (m_store->setFeedback(messageId, value))
        emit feedbackChanged();
}

QVariantList Bridge::records() const
{
    QVariantList result;
    if (m_store == nullptr || !m_store->ready())
        return result;
    for (const StoredMessage &message : m_store->recentMessages(300)) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), message.id);
        item.insert(QStringLiteral("role"), message.role);
        item.insert(QStringLiteral("text"), message.content);
        item.insert(QStringLiteral("feedback"), message.feedback);
        result.append(item);
    }
    return result;
}

void Bridge::setCharacterEnabled(const QString &id, bool enabled)
{
    CharacterMeta meta;
    QString error;
    if (!m_characters->meta(id, &meta, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), error);
        return;
    }
    meta.enabled = enabled;
    if (!m_characters->saveMeta(id, meta, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    refreshCharacters();
}

void Bridge::retryChat()
{
    m_chat->retryLast();
}

void Bridge::restartConversation()
{
    m_chat->restartConversation();
    reportInfo(QStringLiteral("已重启对话，记忆已重置"));
}

void Bridge::backtrackToIndex(int index)
{
    m_chat->backtrackToIndex(index);
    emit memoriesChanged();
    reportInfo(QStringLiteral("已回溯，之后的消息与记忆已清除"));
}

void Bridge::quitApp()
{
    STACKLOG_INFO() << "quit requested";
    QApplication::quit();
}
