#include "test_sse.h"

#include <QtTest>

#include "sseparser.h"

void TestSse::parsesDeltas()
{
    SseParser parser;
    const QList<QString> deltas = parser.feed(
        "data: {\"choices\":[{\"delta\":{\"content\":\"hello\"}}]}\n\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\" world\"}}]}\n\n");

    QCOMPARE(deltas.size(), 2);
    QCOMPARE(deltas.at(0), QStringLiteral("hello"));
    QCOMPARE(deltas.at(1), QStringLiteral(" world"));
}

void TestSse::handlesPartialLines()
{
    SseParser parser;
    QVERIFY(parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"part\"}}]}").isEmpty());

    const QList<QString> deltas = parser.feed("\ndata: [DONE]\n");
    QCOMPARE(deltas.size(), 1);
    QCOMPARE(deltas.at(0), QStringLiteral("part"));
    QVERIFY(parser.done());
}

void TestSse::ignoresNonDataLines()
{
    SseParser parser;
    const QList<QString> deltas = parser.feed(": keep-alive\n\n");
    QVERIFY(deltas.isEmpty());
    QVERIFY(!parser.done());
}

void TestSse::detectsDone()
{
    SseParser parser;
    parser.feed("data: [DONE]\n\n");
    QVERIFY(parser.done());
}

QTEST_GUILESS_MAIN(TestSse)
