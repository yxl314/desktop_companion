#ifndef TEST_ERRORCODES_H
#define TEST_ERRORCODES_H

#include <QObject>

class TestErrorCodes : public QObject
{
    Q_OBJECT

private slots:
    void formatsCode();
    void mapsHttpStatus();
    void mapsNetworkError();
    void buildsErrorObject();
};

#endif // TEST_ERRORCODES_H
