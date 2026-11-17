#ifndef TEST_CHARACTERPACK_H
#define TEST_CHARACTERPACK_H

#include <QObject>

class TestCharacterPack : public QObject
{
    Q_OBJECT

private slots:
    void exportImportRoundTrip();
    void rejectsIncompletePack();
    void rejectsExistingCharacter();
};

#endif // TEST_CHARACTERPACK_H
