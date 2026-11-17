#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <QObject>

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    void savesAndLoads();
    void personaImportAndRemove();
    void apiKeyIsEncrypted();
    void autoStartRoundTrip();
    void corruptConfigFallsBack();
    void themeRoundTrip();
};

#endif // TEST_CONFIG_H
