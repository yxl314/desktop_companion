#ifndef TEST_HITAREAMETA_H
#define TEST_HITAREAMETA_H

#include <QObject>

class TestHitAreaMeta : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToEmpty();
    void loadsValidMeta();
    void hitAreaAtReturnsExpectedId();
    void rejectsInvalidRect();
    void rejectsInvalidStructure();
    void loadsFromFile();
    void failureKeepsPreviousState();
    void acceptsObjectForms();
};

#endif // TEST_HITAREAMETA_H
