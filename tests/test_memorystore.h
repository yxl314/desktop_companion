#ifndef TEST_MEMORYSTORE_H
#define TEST_MEMORYSTORE_H

#include <QObject>

class TestMemoryStore : public QObject
{
    Q_OBJECT

private slots:
    void addListAndCount();
    void updateAndPin();
    void rejectsInvalidEntries();
    void enforcesMaxEntries();
    void searchRanksKeywordThenFreshness();
    void searchRanksFreshnessForEmptyQuery();
    void pinnedAlwaysReturned();
    void searchUpdatesLastUsedAt();
    void resetForPersonaAndRemove();
    void persistsAcrossReopen();
};

#endif // TEST_MEMORYSTORE_H
