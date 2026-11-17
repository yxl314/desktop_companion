/**
 * \file src/app/application.cpp
 * \brief 应用外壳实现：Model/ViewModel 组装、QML 上下文、窗口/托盘/快捷键。
 */

#include "app/application.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include <stacklog/stacklog.hpp>

#include "app/windowregistry.h"
#include "data/characterstore.h"
#include "data/chatstore.h"
#include "data/configstore.h"
#include "data/intimacyrepository.h"
#include "data/memorystore.h"
#include "data/personastore.h"
#include "data/worldstore.h"
#include "platform/foregroundwatcher.h"
#include "platform/globalhotkey.h"
#include "platform/petwindowhelper.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"
#include "viewmodels/viewmodels.h"

Application::Application(QGuiApplication &app, QObject *parent)
    : QObject(parent)
    , m_app(app)
{
}

Application::~Application() = default;

void Application::setupServices()
{
    m_config = new ConfigStore(QString(), this);
    m_personas = new PersonaStore(QString(), this);
    m_store = new ChatStore(QString(), this);
    m_memory = new MemoryStore(QString(), this);
    m_characters = new CharacterStore(QString(), this);
    m_worlds = new WorldStore(QString(), this);
    m_intimacy = new IntimacyService(QString(), this);
    m_chat = new ChatService(m_config, m_personas, m_store, m_memory, m_intimacy, this);
    m_chat->setWorldPeerSources(m_characters, m_worlds);
    m_dialogs = new DialogService(this);
}

void Application::setupViewModels()
{
    m_appViewModel = new AppViewModel(m_config, this, m_dialogs);
    m_chatViewModel = new ChatViewModel(m_chat, m_store, m_memory, m_intimacy, m_config,
                                        m_characters, m_worlds, this);
    m_charactersViewModel = new CharactersViewModel(m_characters, m_worlds, m_config,
                                                    m_dialogs, this);
    m_personasViewModel = new PersonasViewModel(m_personas, m_chat, m_config, m_dialogs,
                                                m_characters, m_intimacy, this);
    m_worldsViewModel = new WorldsViewModel(m_worlds, m_characters, m_dialogs, m_config, this);
    m_memoriesViewModel = new MemoriesViewModel(m_memory, m_chat, this);
    m_modelsViewModel = new ModelSettingsViewModel(m_config, m_chat, this);
    m_recordsViewModel = new RecordsViewModel(m_store, m_dialogs, this);
    m_dataViewModel = new DataViewModel(m_config, m_store, m_chat, m_dialogs, this);
    m_petViewModel = new PetViewModel(m_config, m_characters, m_intimacy, this);
}

void Application::setupContextProperties()
{
    QQmlContext *context = m_engine->rootContext();
    context->setContextProperty(QStringLiteral("appVM"), m_appViewModel);
    context->setContextProperty(QStringLiteral("chatVM"), m_chatViewModel);
    context->setContextProperty(QStringLiteral("charactersVM"), m_charactersViewModel);
    context->setContextProperty(QStringLiteral("personasVM"), m_personasViewModel);
    context->setContextProperty(QStringLiteral("worldsVM"), m_worldsViewModel);
    context->setContextProperty(QStringLiteral("memoriesVM"), m_memoriesViewModel);
    context->setContextProperty(QStringLiteral("modelsVM"), m_modelsViewModel);
    context->setContextProperty(QStringLiteral("recordsVM"), m_recordsViewModel);
    context->setContextProperty(QStringLiteral("dataVM"), m_dataViewModel);
    context->setContextProperty(QStringLiteral("petVM"), m_petViewModel);
    context->setContextProperty(QStringLiteral("dialogs"), m_dialogs);
}

void Application::setupWindows()
{
    if (!m_windows->loadMainWindow())
        STACKLOG_ERROR() << "main window failed to load";
    m_windows->setPetCount(m_appViewModel->petCount());
    m_windows->loadBubbleWindow();
    m_windows->loadBallWindow();
}

void Application::setupTray()
{
    m_trayMenu = new QMenu();
    QAction *showAction = m_trayMenu->addAction(tr("打开主窗口"));
    QAction *chatAction = m_trayMenu->addAction(tr("打开聊天"));
    QAction *togglePetAction = m_trayMenu->addAction(tr("显示 / 隐藏桌宠"));
    QAction *settingsAction = m_trayMenu->addAction(tr("设置…"));
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(tr("退出"));

    QObject::connect(showAction, &QAction::triggered, this, [this] { m_windows->showMainWindow(); });
    QObject::connect(chatAction, &QAction::triggered, this, [this] {
        m_appViewModel->setPageIndex(0);
        m_windows->showMainWindow();
    });
    QObject::connect(togglePetAction, &QAction::triggered, this, [this] {
        m_appViewModel->setPetVisible(!m_appViewModel->petVisible());
    });
    QObject::connect(settingsAction, &QAction::triggered, this, [this] {
        m_appViewModel->setPageIndex(8);
        m_windows->showMainWindow();
    });
    QObject::connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_tray = new QSystemTrayIcon(QIcon(QStringLiteral(":/pet.png")), this);
    m_tray->setToolTip(tr("桌面桌宠"));
    m_tray->setContextMenu(m_trayMenu);
    m_tray->show();
    QObject::connect(m_tray, &QSystemTrayIcon::activated, this,
                     [this](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::DoubleClick)
                             m_windows->toggleMainWindow();
                     });
}

void Application::setupHotkeys()
{
    m_hotkey = new GlobalHotkey(this);
    m_app.installNativeEventFilter(m_hotkey);
    QString error;
    if (!m_hotkey->registerDefaultHotkeys(&error))
        STACKLOG_WARN() << "hotkey registration: " << error.toStdString();
    QObject::connect(m_hotkey, &GlobalHotkey::togglePetRequested, this, [this] {
        m_appViewModel->setPetVisible(!m_appViewModel->petVisible());
    });
    QObject::connect(m_hotkey, &GlobalHotkey::openChatRequested, this, [this] {
        m_appViewModel->setPageIndex(0);
        m_windows->showMainWindow();
    });
    QObject::connect(m_hotkey, &GlobalHotkey::toggleMuteRequested, this, [this] {
        m_appViewModel->setMode(m_appViewModel->mode() == 2 ? 0 : 2);
    });
}

void Application::setupConnections()
{
    QObject::connect(m_appViewModel, &AppViewModel::mainWindowVisibilityRequested, this,
                     [this](bool visible) {
                         if (visible)
                             m_windows->showMainWindow();
                         else
                             m_windows->hideMainWindow();
                     });
    QObject::connect(m_appViewModel, &AppViewModel::petMenuRequested, this,
                     &Application::showPetMenu);
    QObject::connect(m_chatViewModel, &ChatViewModel::modelSettingsRequested, this, [this] {
        m_appViewModel->setPageIndex(5);
        m_windows->showMainWindow();
    });
    QObject::connect(m_petViewModel, &PetViewModel::doubleTapRequested, this, [this](int) {
        m_appViewModel->setPageIndex(0);
        m_windows->showMainWindow();
    });
    QObject::connect(m_petViewModel, &PetViewModel::contextMenuRequested, this,
                     &Application::showPetMenu);
    QObject::connect(m_petViewModel, &PetViewModel::characterActivated, this,
                     [this](const QString &id) {
                         m_appViewModel->setActivePetCharacter(id);
                         if (m_charactersViewModel != nullptr)
                             m_charactersViewModel->select(id);
                     });
    QObject::connect(m_charactersViewModel, &CharactersViewModel::launchPetRequested, this,
                     [this](const QString &id) {
                         m_appViewModel->setActivePetCharacter(id);
                         m_appViewModel->launchPet();
                     });
    QObject::connect(m_appViewModel, &AppViewModel::petCountChanged, this, [this] {
        m_windows->setPetCount(m_appViewModel->petCount());
        applyPetMasks();
    });
    QObject::connect(m_appViewModel, &AppViewModel::clickThroughChanged, this,
                     &Application::applyPetMasks);
    QObject::connect(m_appViewModel, &AppViewModel::debugHitAreasChanged, this,
                     &Application::applyPetMasks);
    QObject::connect(m_charactersViewModel, &CharactersViewModel::charactersChanged, this,
                     &Application::applyPetMasks);
    QObject::connect(m_memoriesViewModel, &MemoriesViewModel::extractRequested, this, [this] {
        m_chat->extractMemoriesNow();
    });
    QObject::connect(m_appViewModel, &AppViewModel::intimacyDecayChanged, this, [this] {
        const bool enabled = m_appViewModel->intimacyDecay();
        m_intimacy->setDecayEnabled(enabled);
        if (enabled)
            m_intimacy->applyDailyDecay();
    });
    QObject::connect(m_appViewModel, &AppViewModel::toast, this, [](const QString &message) {
        STACKLOG_INFO() << "toast: " << message.toStdString();
    });
}

void Application::setupForegroundWatcher()
{
    m_foreground = new ForegroundWatcher(this);
    QObject::connect(m_foreground, &ForegroundWatcher::categoryChanged, this,
                     [this](int category) {
                         if (!m_appViewModel->autoMode())
                             return;
                         const int target = category == 1 ? 1 : 0;
                         if (m_appViewModel->mode() != target)
                             m_appViewModel->setMode(target);
                     });
    QObject::connect(m_appViewModel, &AppViewModel::autoModeChanged, this, [this] {
        if (m_appViewModel->autoMode())
            m_foreground->start();
        else
            m_foreground->stop();
    });
    if (m_appViewModel->autoMode())
        m_foreground->start();
}

bool Application::initialize()
{
    m_engine = new QQmlApplicationEngine(this);
    const QString bundledQml = QCoreApplication::applicationDirPath() + QStringLiteral("/qml");
    if (QDir(bundledQml).exists())
        m_engine->addImportPath(bundledQml);
    m_engine->addImportPath(QStringLiteral("qrc:/qml"));
    setupServices();
    setupViewModels();
    setupForegroundWatcher();
    setupContextProperties();
    m_windows = new WindowRegistry(m_engine, this);
    setupTray();
    setupHotkeys();
    setupConnections();
    setupWindows();
    STACKLOG_INFO() << "application initialized";
    return true;
}

void Application::showFirstScreen()
{
    m_windows->showMainWindow();
    applyPetMasks();
}

int Application::grabWindows(const QString &dir, bool includeHidden) const
{
    QDir().mkpath(dir);
    if (m_windows == nullptr)
        return 0;
    int saved = 0;
    for (QQuickWindow *window : m_windows->allWindows()) {
        if (window == nullptr)
            continue;
        if (!window->isVisible() && !includeHidden)
            continue;
        const QImage image = window->grabWindow();
        if (image.isNull())
            continue;
        QString name = window->objectName();
        if (name.isEmpty())
            name = QStringLiteral("window");
        if (image.save(dir + QLatin1Char('/') + name + QStringLiteral(".png")))
            ++saved;
        STACKLOG_INFO() << "grab " << name.toStdString() << " visible=" << window->isVisible();
    }
    return saved;
}

void Application::applyPetMasks()
{
    if (m_windows == nullptr)
        return;
    const QVariantList pets = m_petViewModel->pets();
    for (int i = 0; i < m_windows->petWindows().size(); ++i) {
        QQuickWindow *window = m_windows->petWindows().at(i);
        if (window == nullptr)
            continue;
        if (!m_config->values().clickThrough || m_config->values().debugHitAreas) {
            petwindow::clearMask(window);
            continue;
        }
        if (i >= pets.size())
            continue;
        const QString avatar = pets.at(i).toMap().value(QStringLiteral("avatar")).toString();
        const QImage image(avatar);
        if (image.isNull())
            continue;
        if (!petwindow::applyAlphaMask(window, image))
            STACKLOG_WARN() << "apply alpha mask failed";
    }
}

void Application::showPetMenu(int index)
{
    Q_UNUSED(index)
    QMenu menu;
    QAction *chat = menu.addAction(tr("打开聊天"));
    QAction *hide = menu.addAction(tr("隐藏桌宠"));
    QAction *settings = menu.addAction(tr("设置…"));
    menu.addSeparator();
    QAction *quit = menu.addAction(tr("退出"));
    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == chat) {
        m_appViewModel->setPageIndex(0);
        m_windows->showMainWindow();
    } else if (chosen == hide) {
        m_appViewModel->setPetVisible(false);
    } else if (chosen == settings) {
        m_appViewModel->setPageIndex(8);
        m_windows->showMainWindow();
    } else if (chosen == quit) {
        QCoreApplication::quit();
    }
}
