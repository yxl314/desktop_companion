#ifndef TEST_INTIMACY_H
#define TEST_INTIMACY_H

#include <QObject>

class TestIntimacy : public QObject
{
    Q_OBJECT

private slots:
    void defaultStateAndLevelThresholds();
    void addClampsAndRecordsEvents();
    void persistsAcrossServices();
    void corruptFileFallsBackWithBackup();
    void decayDisabledByDefault();
    void dailyDecayAppliesPerDay();
    void jsonRoundTrip();
    void eventCapEnforced();
};

#endif // TEST_INTIMACY_H
