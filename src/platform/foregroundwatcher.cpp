#include "platform/foregroundwatcher.h"

#include <QStringList>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

#include <stacklog/stacklog.hpp>

ForegroundWatcher::ForegroundWatcher(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &ForegroundWatcher::poll);
}

void ForegroundWatcher::start(int intervalMs)
{
    m_timer->setInterval(intervalMs);
    m_timer->start();
    poll();
}

void ForegroundWatcher::stop()
{
    m_timer->stop();
}

int ForegroundWatcher::classify(const QString &processName)
{
    const QString name = processName.toLower();
    static const QStringList workApps = {
        QStringLiteral("code.exe"),      QStringLiteral("code-insiders.exe"),
        QStringLiteral("devenv.exe"),    QStringLiteral("idea64.exe"),
        QStringLiteral("pycharm64.exe"), QStringLiteral("clion64.exe"),
        QStringLiteral("goland64.exe"),  QStringLiteral("rider64.exe"),
        QStringLiteral("sublime_text.exe"), QStringLiteral("notepad++.exe"),
        QStringLiteral("notepad.exe"),   QStringLiteral("cursor.exe"),
        QStringLiteral("excel.exe"),     QStringLiteral("winword.exe"),
        QStringLiteral("powerpnt.exe"),  QStringLiteral("outlook.exe"),
        QStringLiteral("wps.exe"),       QStringLiteral("et.exe"),
        QStringLiteral("wpp.exe"),       QStringLiteral("zed.exe"),
    };
    static const QStringList gameApps = {
        QStringLiteral("steam.exe"),     QStringLiteral("steamwebhelper.exe"),
        QStringLiteral("leagueclient.exe"), QStringLiteral("league of legends.exe"),
        QStringLiteral("valorant.exe"),  QStringLiteral("cs2.exe"),
        QStringLiteral("dota2.exe"),     QStringLiteral("genshinimpact.exe"),
        QStringLiteral("yuanshen.exe"),  QStringLiteral("starrail.exe"),
        QStringLiteral("minecraft.exe"), QStringLiteral("javaw.exe"),
        QStringLiteral("epicgameslauncher.exe"),
    };
    if (workApps.contains(name))
        return 1;
    if (gameApps.contains(name))
        return 2;
    return 0;
}

void ForegroundWatcher::poll()
{
#ifdef Q_OS_WIN
    const HWND hwnd = GetForegroundWindow();
    if (hwnd == nullptr)
        return;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process == nullptr)
        return;
    wchar_t buffer[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QString name;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        const QString path = QString::fromWCharArray(buffer, int(size));
        const int slash = path.lastIndexOf(QLatin1Char('\\'));
        name = slash >= 0 ? path.mid(slash + 1) : path;
    }
    CloseHandle(process);
    if (name.isEmpty() || name == m_process)
        return;
    m_process = name;
    const int category = classify(name);
    if (category != m_category) {
        m_category = category;
        STACKLOG_INFO() << "foreground category: " << category << " (" << name.toStdString() << ")";
        emit categoryChanged(category);
    }
#else
    Q_UNUSED(m_process)
#endif
}
