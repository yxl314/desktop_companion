#include "test_chatengine.h"

#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "services/chatengine.h"

void TestChatEngine::replyContainsExpectedLine()
{
    ChatEngine engine;
    QSignalSpy deltaSpy(&engine, &ChatEngine::delta);
    QSignalSpy doneSpy(&engine, &ChatEngine::done);

    engine.send(QStringLiteral("你好"));

    QTRY_COMPARE_WITH_TIMEOUT(doneSpy.count(), 1, 5000);

    QString text;
    for (const QList<QVariant> &arguments : deltaSpy)
        text += arguments.at(0).toString();
    QVERIFY(text.contains(QStringLiteral("小柚")));
}

void TestChatEngine::thinkingThenStreamingThenDone()
{
    ChatEngine engine;
    QSignalSpy stateSpy(&engine, &ChatEngine::stateChanged);
    QSignalSpy doneSpy(&engine, &ChatEngine::done);

    engine.send(QStringLiteral("随便聊聊"));

    QTRY_COMPARE_WITH_TIMEOUT(doneSpy.count(), 1, 5000);
    QVERIFY(stateSpy.count() >= 2);
    QCOMPARE(stateSpy.at(0).at(0).toString(), QStringLiteral("thinking"));
    QCOMPARE(stateSpy.at(stateSpy.count() - 1).at(0).toString(), QStringLiteral("idle"));
}

QTEST_GUILESS_MAIN(TestChatEngine)
