/**
 * \file src/main.cpp
 * \brief 程序入口：日志、单实例、翻译、应用外壳与验证模式（smoke/shot/demo）。
 */

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>
#include <QUrl>

#include <singleproc/singleproc.hpp>
#include <stacklog/stacklog.hpp>

#include "app/application.h"
#include "app/windowregistry.h"
#include "data/configstore.h"
#include "viewmodels/appviewmodel.h"
#include "viewmodels/chatviewmodel.h"

namespace
{

/// 把 Qt 的日志转发到 stacklog，保证日志只有一个出口。
void qtMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    const std::string text = message.toStdString();
    switch (type) {
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

/**
 * \brief 按配置安装英文翻译（QML 与 C++ 共享 :/i18n 资源）。
 * \param app 应用对象。
 * \return 翻译器指针；中文/跟随系统时返回空。
 */
QTranslator *installTranslator(QApplication &app)
{
    ConfigStore probe;
    const int language = probe.values().language;
    if (language != 1)
        return nullptr;
    auto *translator = new QTranslator(&app);
    if (translator->load(QStringLiteral(":/i18n/desktop_companion_en.qm"))) {
        app.installTranslator(translator);
        STACKLOG_INFO() << "english translation installed";
        return translator;
    }
    delete translator;
    return nullptr;
}

} // namespace

int main(int argc, char *argv[])
{
    // 统一使用 Material 控件样式，配色由 QML Theme token 控制。
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("desktop_companion"));
    app.setApplicationName(QStringLiteral("desktop_companion"));
    app.setApplicationVersion(QStringLiteral(DESKTOP_COMPANION_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/pet.png")));
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
    QCommandLineOption managerPageOption(
        QStringLiteral("manager-page"),
        QStringLiteral("打开主窗口时定位到指定页（0 聊天 1 角色 2 人设 3 世界观 4 记忆 5 模型 6 记录 7 数据 8 设置）"),
        QStringLiteral("index"));
    parser.addOption(managerPageOption);
    QCommandLineOption hidePetOption(QStringLiteral("hide-pet"),
                                     QStringLiteral("启动后隐藏桌宠（用于验证悬浮球）"));
    parser.addOption(hidePetOption);
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

    const bool smokeMode = parser.isSet(smokeOption);
    const QString smokeDir = smokeMode ? parser.value(smokeOption) : QString();
    const bool shotMode = parser.isSet(shotOption);
    const QString shotDir = shotMode ? parser.value(shotOption) : QString();
    const bool demoMode = parser.isSet(demoOption);
    const QString demoDir = demoMode ? parser.value(demoOption) : QString();
    const int pageIndex = parser.isSet(managerPageOption)
                              ? parser.value(managerPageOption).toInt()
                              : -1;

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
    stacklog::init(logConfig);
    qInstallMessageHandler(qtMessageHandler);
    STACKLOG_INFO() << "desktop_companion " << DESKTOP_COMPANION_VERSION << " starting";

    // 同一时间只允许一个实例；二次启动通过门铃唤醒主实例的主窗口。
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                               + QStringLiteral("/desktop_companion");
    QDir().mkpath(runtimeDir);
    singleproc::Guard guard(QStringLiteral("desktop_companion").toStdString(),
                            runtimeDir.toStdString());
    if (guard.state() == singleproc::Guard::State::AlreadyRunning) {
        // Guard 构造时已自动给主实例发门铃通知。
        STACKLOG_INFO() << "another instance is running, doorbell notified";
        return 0;
    }
    if (guard.state() == singleproc::Guard::State::Error) {
        STACKLOG_ERROR() << "single instance guard error: errno=" << guard.error_code();
        return 2;
    }

    QTranslator *translator = installTranslator(app);
    Q_UNUSED(translator)

    Application application(app);
    if (!application.initialize()) {
        STACKLOG_ERROR() << "application initialize failed";
        return 3;
    }
    guard.set_doorbell_handler([&application] {
        STACKLOG_INFO() << "doorbell received: second launch";
        if (application.windows() != nullptr)
            application.windows()->showMainWindow();
    });
    QTimer doorbellTimer;
    QObject::connect(&doorbellTimer, &QTimer::timeout, &app, [&guard] {
        guard.process_doorbell(0);
    });
    doorbellTimer.start(800);

    if (pageIndex >= 0)
        application.appViewModel()->setPageIndex(pageIndex);
    if (parser.isSet(hidePetOption))
        application.appViewModel()->setPetVisible(false);
    application.showFirstScreen();

    if (smokeMode) {
        QTimer::singleShot(4500, &app, [&application, smokeDir] {
            application.grabWindows(smokeDir, false);
        });
        QTimer::singleShot(5500, &app, [&application, smokeDir] {
            application.grabWindows(smokeDir, true);
            QCoreApplication::quit();
        });
    }

    if (demoMode) {
        const QString demoText = parser.isSet(demoTextOption)
                                     ? parser.value(demoTextOption)
                                     : QStringLiteral("你好，请用一句话确认你能正常工作");
        QTimer::singleShot(1500, &app, [&application, demoText] {
            application.appViewModel()->setPageIndex(0);
            application.chatViewModel()->send(demoText);
        });
        const int holdSeconds = parser.value(demoHoldOption).toInt();
        QTimer::singleShot(9000 + holdSeconds * 1000, &app, [&application, demoDir] {
            application.grabWindows(demoDir, false);
            QCoreApplication::quit();
        });
    }

    if (shotMode) {
        QTimer::singleShot(2500, &app, [shotDir] {
            if (QScreen *screen = QGuiApplication::primaryScreen()) {
                const QPixmap pixmap = screen->grabWindow(0);
                QDir().mkpath(shotDir);
                pixmap.toImage().save(shotDir + QStringLiteral("/desktop.png"));
            }
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
