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
    parser.process(app);

    const bool smokeMode = parser.isSet(smokeOption);
    const QString smokeDir = smokeMode ? parser.value(smokeOption) : QString();
    const bool shotMode = parser.isSet(shotOption);
    const QString shotDir = shotMode ? parser.value(shotOption) : QString();

    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString logDir = appDataDir + QStringLiteral("/logs");
    if (smokeMode)
        logDir = smokeDir + QStringLiteral("/logs");
    else if (shotMode)
        logDir = shotDir + QStringLiteral("/logs");

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
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction(QStringLiteral("退出"));
    QObject::connect(toggleAction, &QAction::triggered, &bridge, &Bridge::togglePet);
    QObject::connect(chatAction, &QAction::triggered, &bridge, &Bridge::openChat);
    QObject::connect(quitAction, &QAction::triggered, &bridge, &Bridge::quitApp);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app, [&bridge](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick)
            bridge.openChat();
    });
    tray.show();

    if (smokeMode) {
        const QString &dir = smokeDir;
        qInfo("loaded windows: %d", int(engine.rootObjects().size()));
        for (QObject *obj : engine.rootObjects())
            qInfo("window: %s", qPrintable(obj->objectName()));
        QTimer::singleShot(1200, &app, [&bridge] {
            bridge.openChat();
            bridge.petClicked(420, 420);
        });
        QTimer::singleShot(1500, &app, [&bridge] { bridge.sendChat(QStringLiteral("你好")); });
        const auto grabAll = [&engine, dir](bool onlyMissing) {
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
        QTimer::singleShot(2600, &app, [grabAll] { grabAll(false); });
        QTimer::singleShot(3400, &app, [grabAll] { grabAll(true); });
        QTimer::singleShot(3800, &app, [] { QCoreApplication::quit(); });
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
