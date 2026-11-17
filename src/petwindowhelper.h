#ifndef PETWINDOWHELPER_H
#define PETWINDOWHELPER_H

#include <QImage>
#include <QString>

class QWindow;

/// 桌宠窗口的 alpha 遮罩工具：透明区域点击穿透（Windows 用 SetWindowRgn）。
namespace petwindow
{
/// 根据图片 alpha 生成窗口区域；失败返回 false。image 为空表示清除区域。
bool applyAlphaMask(QWindow *window, const QImage &image, int alphaThreshold = 16);

/// 清除窗口区域，恢复整窗可点击。
void clearMask(QWindow *window);
} // namespace petwindow

#endif // PETWINDOWHELPER_H
