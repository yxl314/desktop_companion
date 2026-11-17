#include "test_hitmask.h"

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QtTest>

#include "hitmask.h"

namespace
{
constexpr int imageWidth = 12;
constexpr int imageHeight = 10;
const QRect solidRect(2, 3, 4, 5);

void fillRect(QImage *image, const QRect &rect, const QColor &color)
{
    for (int y = rect.y(); y < rect.y() + rect.height(); ++y) {
        for (int x = rect.x(); x < rect.x() + rect.width(); ++x)
            image->setPixelColor(x, y, color);
    }
}

QImage maskedImage()
{
    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    fillRect(&image, solidRect, QColor(255, 0, 0, 255));
    return image;
}
} // namespace

void TestHitMask::defaultsToInvalid()
{
    HitMask mask;
    QVERIFY(!mask.valid());
    QVERIFY(!mask.contains(QPoint(0, 0)));
    QVERIFY(!mask.scaledContains(QPoint(0, 0), QSize(10, 10)));
    QVERIFY(mask.region().isEmpty());
    QVERIFY(mask.imageSize().isEmpty());
    QCOMPARE(mask.alphaThreshold(), HitMask::defaultAlphaThreshold);
}

void TestHitMask::generatesMaskFromAlpha()
{
    HitMask mask;
    QString error;
    QVERIFY2(mask.setImage(maskedImage(), HitMask::defaultAlphaThreshold, &error), qPrintable(error));
    QVERIFY(mask.valid());
    QCOMPARE(mask.imageSize(), QSize(imageWidth, imageHeight));

    QVERIFY(mask.contains(QPoint(2, 3)));
    QVERIFY(mask.contains(QPoint(5, 7)));
    QVERIFY(!mask.contains(QPoint(1, 3)));
    QVERIFY(!mask.contains(QPoint(6, 3)));
    QVERIFY(!mask.contains(QPoint(2, 2)));
    QVERIFY(!mask.contains(QPoint(5, 8)));
    QVERIFY(!mask.contains(QPoint(-1, 3)));
    QVERIFY(!mask.contains(QPoint(2, imageHeight)));

    const QRegion region = mask.region();
    QVERIFY(!region.isEmpty());
    QCOMPARE(region.boundingRect(), solidRect);
    QVERIFY(region.contains(QPoint(2, 3)));
    QVERIFY(region.contains(QPoint(5, 7)));
    QVERIFY(!region.contains(QPoint(0, 0)));
    QVERIFY(!region.contains(QPoint(6, 3)));
    // 区域缓存：重复调用结果一致。
    QVERIFY(mask.region() == region);
}

void TestHitMask::thresholdControlsSemiTransparentPixels()
{
    QImage image(3, 1, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    image.setPixelColor(0, 0, QColor(255, 0, 0, 8));
    image.setPixelColor(1, 0, QColor(255, 0, 0, 16));

    HitMask mask;
    QString error;
    QVERIFY2(mask.setImage(image, HitMask::defaultAlphaThreshold, &error), qPrintable(error));
    QVERIFY(!mask.contains(QPoint(0, 0))); // alpha 8 < 16
    QVERIFY(mask.contains(QPoint(1, 0)));  // alpha 16 >= 16
    QVERIFY(!mask.contains(QPoint(2, 0))); // 全透明像素永不命中

    QVERIFY2(mask.setImage(image, 8, &error), qPrintable(error));
    QVERIFY(mask.contains(QPoint(0, 0)));
    QVERIFY(mask.contains(QPoint(1, 0)));
    QVERIFY(!mask.contains(QPoint(2, 0)));

    // 非法阈值：失败且保留旧遮罩。
    error.clear();
    QVERIFY(!mask.setImage(image, 0, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(mask.valid());
    QVERIFY(mask.contains(QPoint(0, 0)));
}

void TestHitMask::rejectsInvalidInput()
{
    HitMask mask;
    QString error;
    QVERIFY(!mask.setImage(QImage(), HitMask::defaultAlphaThreshold, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!mask.valid());

    error.clear();
    QVERIFY(!mask.setImage(maskedImage(), -1, &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!mask.setImage(maskedImage(), 256, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!mask.valid());
}

void TestHitMask::regionMatchesOpaqueArea()
{
    QImage image(10, 8, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    fillRect(&image, QRect(1, 1, 3, 2), QColor(0, 255, 0, 255));
    fillRect(&image, QRect(6, 4, 3, 3), QColor(0, 0, 255, 255));

    HitMask mask;
    QVERIFY(mask.setImage(image));
    const QRegion region = mask.region();
    QVERIFY(region.contains(QPoint(1, 1)));
    QVERIFY(region.contains(QPoint(8, 6)));
    QVERIFY(!region.contains(QPoint(5, 3)));
    QCOMPARE(region.boundingRect(), QRect(1, 1, 8, 6));
}

void TestHitMask::scaledContainsMapsWindowCoordinates()
{
    HitMask mask;
    QVERIFY(mask.setImage(maskedImage()));

    // 窗口逻辑尺寸是图片像素尺寸的一半（DPR=2）。
    const QSize windowSize(imageWidth / 2, imageHeight / 2);
    QVERIFY(mask.scaledContains(QPoint(1, 2), windowSize)); // -> (2, 4)，命中
    QVERIFY(mask.scaledContains(QPoint(2, 3), windowSize)); // -> (4, 6)，命中
    QVERIFY(!mask.scaledContains(QPoint(0, 0), windowSize)); // -> (0, 0)，透明
    QVERIFY(!mask.scaledContains(QPoint(5, 3), windowSize)); // -> (10, 6)，透明
    QVERIFY(!mask.scaledContains(QPoint(windowSize.width(), 0), windowSize));
    QVERIFY(!mask.scaledContains(QPoint(-1, 0), windowSize));

    // 窗口与图片同尺寸时退化为直接判断。
    QVERIFY(mask.scaledContains(QPoint(2, 3), QSize(imageWidth, imageHeight)));
    QVERIFY(!mask.scaledContains(QPoint(0, 0), QSize(imageWidth, imageHeight)));

    // 窗口比图片大时按比例放大取样。
    const QSize largeSize(imageWidth * 2, imageHeight * 2);
    QVERIFY(mask.scaledContains(QPoint(4, 6), largeSize)); // -> (2, 3)，命中
    QVERIFY(!mask.scaledContains(QPoint(0, 0), largeSize));
}

void TestHitMask::rgbImagesAreFullySolid()
{
    QImage image(4, 3, QImage::Format_RGB32);
    image.fill(Qt::red);

    HitMask mask;
    QVERIFY(mask.setImage(image));
    QVERIFY(mask.contains(QPoint(0, 0)));
    QVERIFY(mask.contains(QPoint(3, 2)));
    QCOMPARE(mask.region().boundingRect(), QRect(0, 0, 4, 3));
}

void TestHitMask::clearInvalidatesMask()
{
    HitMask mask;
    QVERIFY(mask.setImage(maskedImage()));
    QVERIFY(!mask.region().isEmpty());

    mask.clear();
    QVERIFY(!mask.valid());
    QVERIFY(mask.imageSize().isEmpty());
    QVERIFY(!mask.contains(QPoint(2, 3)));
    QVERIFY(mask.region().isEmpty());
}

QTEST_GUILESS_MAIN(TestHitMask)
