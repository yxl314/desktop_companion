#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QTimer>

#include "bridge.h"

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
    parser.process(app);

    // 透明窗口需要默认 alpha buffer
    QQuickWindow::setDefaultAlphaBuffer(true);

    Bridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("bridge"), &bridge);

    engine.loadFromModule(QStringLiteral("DesktopCompanion"), QStringLiteral("Pet"));
    engine.loadFromModule(QStringLiteral("DesktopCompanion"), QStringLiteral("Bubble"));
    engine.loadFromModule(QStringLiteral("DesktopCompanion"), QStringLiteral("Chat"));

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

    if (parser.isSet(smokeOption)) {
        const QString dir = parser.value(smokeOption);
        QDir().mkpath(dir);
        QTimer::singleShot(1200, &app, [&bridge] {
            bridge.openChat();
            bridge.petClicked(420, 420);
        });
        QTimer::singleShot(2400, &app, [&engine, dir] {
            for (QObject *obj : engine.rootObjects()) {
                auto *window = qobject_cast<QQuickWindow *>(obj);
                if (!window)
                    continue;
                const QString name = window->objectName().isEmpty()
                                         ? QStringLiteral("window")
                                         : window->objectName();
                const QString path = dir + QStringLiteral("/") + name + QStringLiteral(".png");
                const QImage image = window->grabWindow();
                if (image.isNull())
                    qWarning("截图失败: %s", qPrintable(path));
                else
                    image.save(path);
            }
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
