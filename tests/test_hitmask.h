#ifndef TEST_HITMASK_H
#define TEST_HITMASK_H

#include <QObject>

class TestHitMask : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToInvalid();
    void generatesMaskFromAlpha();
    void thresholdControlsSemiTransparentPixels();
    void rejectsInvalidInput();
    void regionMatchesOpaqueArea();
    void scaledContainsMapsWindowCoordinates();
    void rgbImagesAreFullySolid();
    void clearInvalidatesMask();
};

#endif // TEST_HITMASK_H
