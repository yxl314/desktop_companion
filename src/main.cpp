#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMutex>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include "bridge.h"

namespace {
QMutex g_logMutex;
QFile g_logFile;

void fileMessageHandler(QtMsgType, const QMessageLogContext &, const QString &message)
{
    QMutexLocker locker(&g_logMutex);
    if (!g_logFile.isOpen())
        return;
    g_logFile.write(message.toUtf8());
    g_logFile.write("\n");
    g_logFile.flush();
}
}

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
    if (smokeMode || shotMode) {
        const QString logDir = smokeMode ? smokeDir : shotDir;
        QDir().mkpath(logDir);
        g_logFile.setFileName(logDir + QStringLiteral("/run.log"));
        if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return 1;
        qInstallMessageHandler(fileMessageHandler);
    }

    // 透明窗口需要默认 alpha buffer
    QQuickWindow::setDefaultAlphaBuffer(true);

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
        return 1;
    }

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

    return app.exec();
}
