#ifndef TEST_WORLDSTORE_H
#define TEST_WORLDSTORE_H

#include <QObject>

class TestWorldStore : public QObject
{
    Q_OBJECT

private slots:
    void createsAndListsWorlds();
    void rejectsInvalidOrDuplicateNames();
    void readsAndWritesContent();
    void importsExternalWorld();
    void renamesWorld();
    void removesWorld();
    void corruptWorldSkipped();
    void persistsAcrossInstances();
};

#endif // TEST_WORLDSTORE_H
