#ifndef TEST_PROMPTBUILDER_H
#define TEST_PROMPTBUILDER_H

#include <QObject>

class TestPromptBuilder : public QObject
{
    Q_OBJECT

private slots:
    void sectionsInFixedOrder();
    void worldPeersRenderedAfterWorld();
    void emptySectionsSkipped();
    void identityFallsBackToCharacterId();
    void memoriesRendered();
    void buildMessagesShape();
    void defaultConstraintsAvailable();
};

#endif // TEST_PROMPTBUILDER_H
