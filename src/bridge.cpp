#include "bridge.h"

#include <QApplication>
#include <QDesktopServices>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

#include <stacklog/stacklog.hpp>

#include "chatservice.h"
#include "autostart.h"
#include "chatstore.h"
#include "configstore.h"
#include "personastore.h"

namespace
{
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
    , m_chat(new ChatService(m_config, m_personas, m_store, this))
{
    m_petX = m_config->values().petX;
    m_petY = m_config->values().petY;
    m_hasSavedPos = m_petX >= 0 && m_petY >= 0;

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
    const QString currentId = m_config->values().personaId;
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

void Bridge::petClicked(double x, double y, double height)
{
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

    const double ratio = height > 0.0 ? y / height : 0.5;
    const QStringList *lines = &bodyLines;
    if (ratio < 0.32)
        lines = &headLines;
    else if (ratio > 0.72)
        lines = &feetLines;
    STACKLOG_DEBUG() << "pet clicked at " << x << "," << y << " ratio=" << ratio;

    m_bubbleText = lines->at(QRandomGenerator::global()->bounded(lines->size()));
    emit bubbleTextChanged();
    positionBubble(x, y);
    setBubbleVisible(true);
    m_bubbleTimer.start(4200);
    setPetPosition(x, y);
}

void Bridge::positionBubble(double x, double y)
{
    m_bubbleX = x + 40;
    m_bubbleY = (y - 110 < 10) ? y + 330 : y - 110;
    emit bubblePosChanged();
}

void Bridge::setPetPosition(double x, double y)
{
    m_petX = x;
    m_petY = y;
    m_hasSavedPos = true;
    m_config->setPetPosition(x, y);
    m_config->save();
    emit petPosChanged();
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
    STACKLOG_INFO() << "chat send: " << text.toStdString();
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
    m_config->setPersonaId(personaId);
    m_config->save();
    m_chat->switchPersona(personaId);
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

void Bridge::showPetMenu()
{
    QMenu menu;
    QAction *chatAction = menu.addAction(QStringLiteral("打开聊天"));
    QAction *settingsAction = menu.addAction(QStringLiteral("设置…"));
    QAction *hideAction = menu.addAction(QStringLiteral("隐藏桌宠"));
    menu.addSeparator();
    QAction *quitAction = menu.addAction(QStringLiteral("退出"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == chatAction)
        openChat();
    else if (chosen == settingsAction)
        openManager();
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

void Bridge::quitApp()
{
    STACKLOG_INFO() << "quit requested";
    QApplication::quit();
}
