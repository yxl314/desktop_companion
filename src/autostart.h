#ifndef AUTOSTART_H
#define AUTOSTART_H

#include <QString>

/// Windows 当前用户的开机自启项（HKCU\...\Run）。
namespace autostart
{
bool enabled();
void setEnabled(bool on, const QString &exePath = QString());
} // namespace autostart

#endif // AUTOSTART_H
