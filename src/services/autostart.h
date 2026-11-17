/**
 * \file src/services/autostart.h
 * \brief 开机自启：Windows 当前用户的 Run 注册表项。
 */

#ifndef AUTOSTART_H
#define AUTOSTART_H

#include <QString>

/**
 * \brief Windows 当前用户的开机自启项（HKCU\...\Run）。
 *
 * 非 Windows 平台为空实现。
 */
namespace autostart
{
/**
 * \brief 查询是否已开启开机自启。
 * @return 注册表中是否存在自启项。
 */
bool enabled();
/**
 * \brief 开启或关闭开机自启。
 * @param on 是否开启。
 * @param exePath 自启程序路径；为空时使用当前可执行文件。
 */
void setEnabled(bool on, const QString &exePath = QString());
} // namespace autostart

#endif // AUTOSTART_H
