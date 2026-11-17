#ifndef TEST_SSE_H
#define TEST_SSE_H

#include <QObject>

class TestSse : public QObject
{
    Q_OBJECT

private slots:
    void parsesDeltas();
    void handlesPartialLines();
    void ignoresNonDataLines();
    void detectsDone();
};

#endif // TEST_SSE_H
