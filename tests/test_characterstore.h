#ifndef TEST_CHARACTERSTORE_H
#define TEST_CHARACTERSTORE_H

#include <QObject>

class TestCharacterStore : public QObject
{
    Q_OBJECT

private slots:
    void createsAndListsCharacters();
    void rejectsInvalidOrDuplicateNames();
    void persistsAcrossInstances();
    void renamesCharacter();
    void removesCharacter();
    void cardAndMetaRoundTrip();
    void preservesUnknownMetaFields();
    void corruptMetaFails();
    void importsAvatars();
    void managesPersonas();
};

#endif // TEST_CHARACTERSTORE_H
