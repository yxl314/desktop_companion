#include "platform/petwindowhelper.h"

#include <QRegion>
#include <QWindow>
#include <QTransform>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "platform/hitmask.h"

namespace petwindow
{
bool applyAlphaMask(QWindow *window, const QImage &image, int alphaThreshold)
{
    if (window == nullptr)
        return false;
#ifdef Q_OS_WIN
    if (image.isNull())
        return false;

    HitMask mask;
    QString error;
    if (!mask.setImage(image, alphaThreshold, &error))
        return false;

    const QRegion region = mask.region();
    if (region.isEmpty())
        return false;

    // 图片像素区域 → 窗口设备像素区域（考虑 DPR 与控件缩放）。
    const QSize windowDeviceSize(int(window->width() * window->devicePixelRatio()),
                                 int(window->height() * window->devicePixelRatio()));
    const qreal scaleX = image.width() > 0 ? qreal(windowDeviceSize.width()) / image.width() : 1.0;
    const qreal scaleY = image.height() > 0 ? qreal(windowDeviceSize.height()) / image.height() : 1.0;
    const QRegion scaled = QTransform::fromScale(scaleX, scaleY).map(region);

    HRGN combined = nullptr;
    for (const QRect &rect : scaled) {
        const HRGN part = CreateRectRgn(rect.left(), rect.top(), rect.right() + 1, rect.bottom() + 1);
        if (combined == nullptr)
            combined = part;
        else {
            CombineRgn(combined, combined, part, RGN_OR);
            DeleteObject(part);
        }
    }
    if (combined == nullptr)
        return false;

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (SetWindowRgn(hwnd, combined, TRUE) == 0) {
        DeleteObject(combined);
        return false;
    }
    return true;
#else
    Q_UNUSED(image)
    Q_UNUSED(alphaThreshold)
    return false;
#endif
}

void clearMask(QWindow *window)
{
    if (window == nullptr)
        return;
#ifdef Q_OS_WIN
    SetWindowRgn(reinterpret_cast<HWND>(window->winId()), nullptr, TRUE);
#else
    Q_UNUSED(window)
#endif
}
} // namespace petwindow
