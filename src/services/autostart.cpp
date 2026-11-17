#include "services/autostart.h"

#include <QCoreApplication>
#include <QSettings>

namespace
{
const char *runKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const char *valueName = "desktop_companion";
} // namespace

namespace autostart
{
bool enabled()
{
    QSettings settings(QString::fromLatin1(runKey), QSettings::NativeFormat);
    return settings.contains(QString::fromLatin1(valueName));
}

void setEnabled(bool on, const QString &exePath)
{
    QSettings settings(QString::fromLatin1(runKey), QSettings::NativeFormat);
    if (on) {
        QString path = exePath.isEmpty() ? QCoreApplication::applicationFilePath() : exePath;
        path.replace(QLatin1Char('/'), QLatin1Char('\\'));
        settings.setValue(QString::fromLatin1(valueName), QStringLiteral("\"%1\"").arg(path));
    } else {
        settings.remove(QString::fromLatin1(valueName));
    }
}
} // namespace autostart
