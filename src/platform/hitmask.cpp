#include "platform/hitmask.h"

#include <utility>

namespace
{
constexpr int minAlphaThreshold = 1;
constexpr int maxAlphaThreshold = 255;
constexpr int bitsPerWord = 64;
constexpr int wordShift = 6; // 2^6 = bitsPerWord
constexpr int wordMask = bitsPerWord - 1;

void setError(QString *error, const QString &message)
{
    if (error != nullptr)
        *error = message;
}
} // namespace

bool HitMask::setImage(const QImage &image, int alphaThreshold, QString *error)
{
    if (image.isNull()) {
        setError(error, QStringLiteral("图片为空，无法生成命中遮罩"));
        return false;
    }
    if (alphaThreshold < minAlphaThreshold || alphaThreshold > maxAlphaThreshold) {
        setError(error,
                 QStringLiteral("alpha 阈值必须在 %1~%2 之间").arg(minAlphaThreshold).arg(maxAlphaThreshold));
        return false;
    }

    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    if (source.isNull()) {
        setError(error, QStringLiteral("图片转换为 ARGB32 失败"));
        return false;
    }

    const int width = source.width();
    const int height = source.height();
    if (width <= 0 || height <= 0) {
        setError(error, QStringLiteral("图片尺寸无效：%1x%2").arg(width).arg(height));
        return false;
    }

    const qint64 pixelCount = qint64(width) * qint64(height);
    if (pixelCount > maxPixelCount) {
        setError(error, QStringLiteral("图片过大：%1x%2，命中遮罩最多支持 %3 像素")
                            .arg(width)
                            .arg(height)
                            .arg(maxPixelCount));
        return false;
    }

    const int wordsPerRow = (width + wordMask) / bitsPerWord;
    std::vector<std::uint64_t> words((std::size_t(wordsPerRow) * std::size_t(height)), 0);
    for (int y = 0; y < height; ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        std::uint64_t *row = words.data() + (std::size_t(y) * std::size_t(wordsPerRow));
        for (int x = 0; x < width; ++x) {
            if (qAlpha(line[x]) >= alphaThreshold)
                row[x >> wordShift] |= (std::uint64_t(1) << (x & wordMask));
        }
    }

    m_words = std::move(words);
    m_size = QSize(width, height);
    m_wordsPerRow = wordsPerRow;
    m_alphaThreshold = alphaThreshold;
    m_valid = true;
    m_region = QRegion();
    m_regionReady = false;
    return true;
}

void HitMask::clear()
{
    m_words.clear();
    m_size = QSize();
    m_wordsPerRow = 0;
    m_valid = false;
    m_region = QRegion();
    m_regionReady = false;
}

bool HitMask::contains(const QPoint &imagePixel) const
{
    if (!m_valid)
        return false;
    if (imagePixel.x() < 0 || imagePixel.y() < 0 || imagePixel.x() >= m_size.width()
        || imagePixel.y() >= m_size.height())
        return false;
    return bitAt(imagePixel.x(), imagePixel.y());
}

bool HitMask::scaledContains(const QPoint &windowPixel, const QSize &windowSize) const
{
    if (!m_valid || windowSize.width() <= 0 || windowSize.height() <= 0)
        return false;
    if (windowPixel.x() < 0 || windowPixel.y() < 0 || windowPixel.x() >= windowSize.width()
        || windowPixel.y() >= windowSize.height())
        return false;

    const int imageX = int(qint64(windowPixel.x()) * m_size.width() / windowSize.width());
    const int imageY = int(qint64(windowPixel.y()) * m_size.height() / windowSize.height());
    return contains(QPoint(imageX, imageY));
}

QRegion HitMask::region() const
{
    if (!m_regionReady) {
        QRegion region;
        if (m_valid) {
            const QList<QRect> rects = mergedRects();
            for (const QRect &rect : rects)
                region += rect;
        }
        m_region = std::move(region);
        m_regionReady = true;
    }
    return m_region;
}

bool HitMask::bitAt(int x, int y) const
{
    const std::size_t index = (std::size_t(y) * std::size_t(m_wordsPerRow)) + std::size_t(x >> wordShift);
    return ((m_words[index] >> (x & wordMask)) & std::uint64_t(1)) != 0;
}

QList<QRect> HitMask::mergedRects() const
{
    QList<QRect> rects;
    if (!m_valid)
        return rects;

    struct Run
    {
        int x;
        int width;
        int startY;
    };

    const int width = m_size.width();
    const int height = m_size.height();
    std::vector<Run> active;
    std::vector<Run> current;
    std::vector<Run> merged;

    for (int y = 0; y < height; ++y) {
        current.clear();
        int x = 0;
        while (x < width) {
            while (x < width && !bitAt(x, y))
                ++x;
            const int start = x;
            while (x < width && bitAt(x, y))
                ++x;
            if (x > start)
                current.push_back(Run{.x = start, .width = x - start, .startY = y});
        }

        // 相邻行中 x/宽度完全相同的连续段合并为同一矩形，矩形数量接近命中区轮廓数。
        merged.clear();
        std::size_t oldIndex = 0;
        std::size_t newIndex = 0;
        while (oldIndex < active.size() || newIndex < current.size()) {
            if (oldIndex >= active.size()) {
                merged.push_back(current[newIndex]);
                ++newIndex;
                continue;
            }
            if (newIndex >= current.size()) {
                const Run &oldRun = active[oldIndex];
                rects.append(QRect(oldRun.x, oldRun.startY, oldRun.width, y - oldRun.startY));
                ++oldIndex;
                continue;
            }

            const Run &oldRun = active[oldIndex];
            const Run &newRun = current[newIndex];
            if (oldRun.x < newRun.x) {
                rects.append(QRect(oldRun.x, oldRun.startY, oldRun.width, y - oldRun.startY));
                ++oldIndex;
            } else if (newRun.x < oldRun.x) {
                merged.push_back(newRun);
                ++newIndex;
            } else if (oldRun.width == newRun.width) {
                merged.push_back(oldRun);
                ++oldIndex;
                ++newIndex;
            } else {
                rects.append(QRect(oldRun.x, oldRun.startY, oldRun.width, y - oldRun.startY));
                merged.push_back(newRun);
                ++oldIndex;
                ++newIndex;
            }
        }
        active = std::move(merged);
    }

    for (const Run &run : active)
        rects.append(QRect(run.x, run.startY, run.width, height - run.startY));
    return rects;
}
