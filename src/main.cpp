#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include <singleproc/singleproc.hpp>
#include <stacklog/stacklog.hpp>

#include "bridge.h"

namespace
{

/// 把 Qt 的日志转发到 stacklog，保证日志只有一个出口。
void qtMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    const std::string text = message.toStdString();
    switch (type)
    {
    case QtDebugMsg:
        STACKLOG_LOG(stacklog::level::debug) << text;
        break;
    case QtInfoMsg:
        STACKLOG_LOG(stacklog::level::info) << text;
        break;
    case QtWarningMsg:
        STACKLOG_LOG(stacklog::level::warn) << text;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        STACKLOG_LOG(stacklog::level::error) << text;
        break;
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("desktop_companion"));
    app.setApplicationName(QStringLiteral("desktop_companion"));
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption smokeOption(QStringLiteral("smoke"),
                                   QStringLiteral("渲染窗口并截图后退出，仅用于验证"),
                                   QStringLiteral("dir"));
    parser.addOption(smokeOption);
    QCommandLineOption shotOption(QStringLiteral("shot"),
                                  QStringLiteral("运行 2.5 秒后截取全屏并退出，仅用于验证"),
                                  QStringLiteral("dir"));
    parser.addOption(shotOption);
    QCommandLineOption managerPageOption(QStringLiteral("manager-page"),
                                        QStringLiteral("打开控制中心时定位到指定页（0 模型 1 角色 2 人设 3 世界观 4 数据 5 关于）"),
                                        QStringLiteral("index"));
    parser.addOption(managerPageOption);
    QCommandLineOption demoTextOption(QStringLiteral("demo-text"),
                                     QStringLiteral("demo 模式发送的消息内容"),
                                     QStringLiteral("text"));
    parser.addOption(demoTextOption);
    QCommandLineOption demoHoldOption(QStringLiteral("demo-hold"),
                                     QStringLiteral("demo 截图后额外等待秒数（给后台任务留时间）"),
                                     QStringLiteral("seconds"));
    parser.addOption(demoHoldOption);
    QCommandLineOption demoOption(QStringLiteral("demo"),
                                  QStringLiteral("发一条消息，等回复完成后截图并退出，仅用于验证"),
                                  QStringLiteral("dir"));
    parser.addOption(demoOption);
    parser.process(app);
    const int managerPageIndex = parser.isSet(managerPageOption)
                                    ? parser.value(managerPageOption).toInt()
                                    : -1;

    const bool smokeMode = parser.isSet(smokeOption);
    const QString smokeDir = smokeMode ? parser.value(smokeOption) : QString();
    const bool shotMode = parser.isSet(shotOption);
    const QString shotDir = shotMode ? parser.value(shotOption) : QString();
    const bool demoMode = parser.isSet(demoOption);
    const QString demoDir = demoMode ? parser.value(demoOption) : QString();

    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString logDir = appDataDir + QStringLiteral("/logs");
    if (smokeMode)
        logDir = smokeDir + QStringLiteral("/logs");
    else if (shotMode)
        logDir = shotDir + QStringLiteral("/logs");
    else if (demoMode)
        logDir = demoDir + QStringLiteral("/logs");

    stacklog::config logConfig;
    logConfig.dir = logDir.toStdString();
    logConfig.base_name = "desktop_companion";
    logConfig.min_level = stacklog::level::debug;
    const bool loggingReady = stacklog::init(logConfig);
    qInstallMessageHandler(qtMessageHandler);
    if (loggingReady)
        STACKLOG_INFO() << "desktop_companion starting, log dir: " << logConfig.dir;
    else
        STACKLOG_ERROR() << "stacklog init failed, falling back to stderr";

    QQuickWindow::setDefaultAlphaBuffer(true);

    // 单实例：第一次启动成为主实例，后续启动只发门铃通知并退出。
    const QString runtimeDir = appDataDir + QStringLiteral("/runtime");
    QDir().mkpath(runtimeDir);
    singleproc::Guard guard(QStringLiteral("desktop_companion").toStdString(), runtimeDir.toStdString());
    if (guard.state() == singleproc::Guard::State::AlreadyRunning)
    {
        STACKLOG_INFO() << "another instance is running, doorbell sent, runtime=" << runtimeDir.toStdString();
        stacklog::shutdown();
        return 0;
    }
    if (guard.state() == singleproc::Guard::State::Error)
    {
        STACKLOG_ERROR() << "single-instance guard failed, error_code=" << guard.error_code();
        stacklog::shutdown();
        return 1;
    }
    STACKLOG_INFO() << "primary instance acquired, key=desktop_companion, runtime=" << runtimeDir.toStdString();

    Bridge bridge;

    QQmlApplicationEngine engine;
    if (managerPageIndex >= 0)
        bridge.setManagerPage(managerPageIndex);
    engine.rootContext()->setContextProperty(QStringLiteral("bridge"), &bridge);

    const QString bundledQml = QCoreApplication::applicationDirPath() + QStringLiteral("/qml");
    if (QDir(bundledQml).exists())
        engine.addImportPath(bundledQml);

    const auto loadQml = [&engine](const QString &name) {
        const QString path = QStringLiteral(":/qml/%1.qml").arg(name);
        if (!QFile::exists(path)) {
            qWarning("QML 未找到: %s", qPrintable(path));
            return;
        }
        qInfo("QML: %s", qPrintable(path));
        engine.load(QUrl(QStringLiteral("qrc") + path));
    };
    loadQml(QStringLiteral("Pet"));
    loadQml(QStringLiteral("Bubble"));
    loadQml(QStringLiteral("Chat"));
    loadQml(QStringLiteral("Manager"));

    if (engine.rootObjects().isEmpty()) {
        qCritical("QML 加载失败");
        stacklog::shutdown();
        return 1;
    }

    // 二次启动：门铃触发的提示，由主实例气泡展示。
    guard.set_doorbell_handler([&bridge] {
        STACKLOG_INFO() << "doorbell received: second launch";
        bridge.notifyAlreadyRunning();
    });
    QTimer doorbellTimer;
    doorbellTimer.setInterval(250);
    QObject::connect(&doorbellTimer, &QTimer::timeout, &app, [&guard] { guard.process_doorbell(0); });
    doorbellTimer.start();

    QSystemTrayIcon tray(QIcon(QStringLiteral(":/pet.png")));
    tray.setToolTip(QStringLiteral("桌面桌宠（原型）"));
    QMenu trayMenu;
    QAction *toggleAction = trayMenu.addAction(QStringLiteral("显示 / 隐藏桌宠"));
    QAction *chatAction = trayMenu.addAction(QStringLiteral("打开聊天"));
    QAction *settingsAction = trayMenu.addAction(QStringLiteral("设置…"));
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction(QStringLiteral("退出"));
    QObject::connect(toggleAction, &QAction::triggered, &bridge, &Bridge::togglePet);
    QObject::connect(chatAction, &QAction::triggered, &bridge, &Bridge::openChat);
    QObject::connect(settingsAction, &QAction::triggered, &bridge, &Bridge::openManager);
    QObject::connect(quitAction, &QAction::triggered, &bridge, &Bridge::quitApp);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app, [&bridge](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick)
            bridge.openChat();
    });
    tray.show();

    const auto grabAll = [&engine](const QString &dir, bool onlyMissing) {
        int saved = 0;
        for (QObject *obj : engine.rootObjects()) {
            auto *window = qobject_cast<QQuickWindow *>(obj);
            if (!window)
                continue;
            const QString name = window->objectName().isEmpty()
                                     ? QStringLiteral("window")
                                     : window->objectName();
            const QString path = dir + QStringLiteral("/") + name + QStringLiteral(".png");
            if (onlyMissing && QFile::exists(path))
                continue;
            qInfo("grab %s visible=%d", qPrintable(name), int(window->isVisible()));
            const QImage image = window->grabWindow();
            if (image.isNull()) {
                qWarning("截图失败: %s", qPrintable(path));
                continue;
            }
            if (image.save(path))
                ++saved;
        }
        return saved;
    };

    if (smokeMode) {
        qInfo("loaded windows: %d", int(engine.rootObjects().size()));
        for (QObject *obj : engine.rootObjects())
            qInfo("window: %s", qPrintable(obj->objectName()));
        QTimer::singleShot(1200, &app, [&bridge] {
            bridge.openChat();
            bridge.petClicked(420, 420, 320);
            bridge.openManager();
        });
        QTimer::singleShot(1500, &app, [&bridge] { bridge.sendChat(QStringLiteral("你好")); });
        QTimer::singleShot(4500, &app, [&] { grabAll(smokeDir, false); });
        QTimer::singleShot(5500, &app, [&] { grabAll(smokeDir, true); });
        QTimer::singleShot(6000, &app, [] { QCoreApplication::quit(); });
    }

    if (demoMode) {
        qInfo("demo mode: dir=%s", qPrintable(demoDir));
        QTimer::singleShot(1200, &app, [&bridge] {
            bridge.openChat();
            bridge.petClicked(420, 420, 320);
        });
        const QString demoText = parser.isSet(demoTextOption)
                                     ? parser.value(demoTextOption)
                                     : QStringLiteral("你好，请用一句话确认你能正常工作");
        QTimer::singleShot(1500, &app, [&bridge, demoText] { bridge.sendChat(demoText); });
        const int holdSeconds = parser.value(demoHoldOption).toInt();
        auto finishDemo = [&app, &grabAll, demoDir, holdSeconds] {
            QTimer::singleShot(600, &app, [&app, &grabAll, demoDir, holdSeconds] {
                grabAll(demoDir, false);
                if (holdSeconds > 0) {
                    QTimer::singleShot(holdSeconds * 1000, &app, [] { QCoreApplication::quit(); });
                } else {
                    QCoreApplication::quit();
                }
            });
        };
        QObject::connect(&bridge, &Bridge::chatDone, &app, finishDemo);
        QObject::connect(&bridge, &Bridge::chatError, &app, finishDemo);
        QTimer::singleShot(60000, &app, [] { QCoreApplication::quit(); });
    }

    if (shotMode) {
        QTimer::singleShot(2500, &app, [&engine, shotDir] {
            QScreen *screen = QGuiApplication::primaryScreen();
            if (screen) {
                qInfo("screen geo=%dx%d avail=%dx%d size=%dx%d dpr=%.2f",
                      screen->geometry().width(), screen->geometry().height(),
                      screen->availableGeometry().width(), screen->availableGeometry().height(),
                      screen->size().width(), screen->size().height(), screen->devicePixelRatio());
                for (QObject *obj : engine.rootObjects()) {
                    auto *window = qobject_cast<QQuickWindow *>(obj);
                    if (!window)
                        continue;
                    qInfo("win %s pos=%d,%d size=%d,%d visible=%d", qPrintable(window->objectName()),
                          window->x(), window->y(), window->width(), window->height(),
                          int(window->isVisible()));
                }
                const QPixmap pixmap = screen->grabWindow(0);
                pixmap.toImage().save(shotDir + QStringLiteral("/desktop.png"));
            }
            QCoreApplication::quit();
        });
    }

    const int exitCode = app.exec();
    STACKLOG_INFO() << "desktop_companion exiting, code=" << exitCode;
    stacklog::shutdown();
    return exitCode;
}
