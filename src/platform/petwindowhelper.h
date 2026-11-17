/**
 * \file src/platform/petwindowhelper.h
 * \brief 桌宠窗口的 alpha 遮罩工具（Windows SetWindowRgn）。
 */

#ifndef PETWINDOWHELPER_H
#define PETWINDOWHELPER_H

#include <QImage>
#include <QString>

class QWindow;

/**
 * \brief 桌宠窗口的 alpha 遮罩工具：透明区域点击穿透（Windows 用 SetWindowRgn）。
 */
namespace petwindow
{
/**
 * \brief 根据图片 alpha 生成窗口区域。
 * @param window 目标窗口。
 * @param image 源图片；为空表示清除区域。
 * @param alphaThreshold alpha 阈值。
 * @return 是否应用成功。
 */
bool applyAlphaMask(QWindow *window, const QImage &image, int alphaThreshold = 16);

/**
 * \brief 清除窗口区域，恢复整窗可点击。
 * @param window 目标窗口。
 */
void clearMask(QWindow *window);
} // namespace petwindow

#endif // PETWINDOWHELPER_H
