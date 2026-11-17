#ifndef HITMASK_H
#define HITMASK_H

#include <QImage>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include <cstdint>
#include <vector>

/// 基于图片 alpha 通道的点击命中遮罩（PET-H-05）。
///
/// - setImage() 把 alpha >= 阈值的像素标记为命中，内部按位图（1 bit/像素）存储，
///   contains() 为 O(1) 位查询，不会在每次命中判断时重复遍历原图。
/// - region() 返回按行合并后的 QRegion，仅供命中区可视化调试（PET-L-02）使用，
///   首次调用时构建并缓存。
/// - 坐标以图片左上角为原点；窗口逻辑坐标向图片像素坐标的换算见 scaledContains()。
///
/// 命中判定为 alpha >= 阈值，阈值范围 1~255（全透明像素永远不命中）。
class HitMask
{
public:
    static constexpr int defaultAlphaThreshold = 16;
    /// 遮罩允许的最大像素数（约 8192x8192），防止异常大图耗尽内存。
    static constexpr qint64 maxPixelCount = 64LL * 1024 * 1024;

    HitMask() = default;

    /// 从图片生成遮罩。失败时保留原有遮罩不变，error 写入原因。
    bool setImage(const QImage &image, int alphaThreshold = defaultAlphaThreshold, QString *error = nullptr);

    /// 清空遮罩，之后 valid() 为 false；alphaThreshold 保持不变。
    void clear();

    bool valid() const { return m_valid; }
    /// 图片像素尺寸（不是窗口逻辑尺寸）。
    QSize imageSize() const { return m_size; }
    int alphaThreshold() const { return m_alphaThreshold; }

    /// 图片像素坐标是否命中；未设置图片或坐标越界时返回 false。
    bool contains(const QPoint &imagePixel) const;

    /// 窗口逻辑坐标 → 图片像素坐标后判断命中。
    /// windowSize 为窗口逻辑尺寸：高 DPI 下窗口逻辑尺寸可能只有图片像素尺寸的一半，
    /// 由本函数按比例换算，调用方无需自行处理 DPR。
    /// windowPixel 超出 windowSize 范围时返回 false。
    bool scaledContains(const QPoint &windowPixel, const QSize &windowSize) const;

    /// 命中区域（合并矩形），用于命中区可视化调试；结果缓存，重复调用不再扫描。
    QRegion region() const;

private:
    bool bitAt(int x, int y) const;
    QList<QRect> mergedRects() const;

    std::vector<std::uint64_t> m_words;
    QSize m_size;
    int m_wordsPerRow = 0;
    int m_alphaThreshold = defaultAlphaThreshold;
    bool m_valid = false;
    mutable QRegion m_region;
    mutable bool m_regionReady = false;
};

#endif // HITMASK_H
