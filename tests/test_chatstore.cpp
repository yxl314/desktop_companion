#include "test_chatstore.h"

#include <QTemporaryDir>
#include <QtTest>

#include "chatstore.h"

void TestChatStore::persistsMessages()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/chat.db");

    {
        ChatStore store(path);
        QVERIFY(store.ready());
        QVERIFY(store.ensureConversation(QStringLiteral("builtin-default")) > 0);
        QVERIFY(store.appendMessage(QStringLiteral("user"), QStringLiteral("你好")));
        QVERIFY(store.appendMessage(QStringLiteral("assistant"), QStringLiteral("你好呀～")));
    }

    ChatStore reopened(path);
    QVERIFY(reopened.ready());
    QVERIFY(reopened.ensureConversation(QStringLiteral("builtin-default")) > 0);
    const QVector<StoredMessage> messages = reopened.recentMessages(10);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.at(0).role, QStringLiteral("user"));
    QCOMPARE(messages.at(0).content, QStringLiteral("你好"));
    QCOMPARE(messages.at(1).role, QStringLiteral("assistant"));
    QCOMPARE(messages.at(1).content, QStringLiteral("你好呀～"));
}

void TestChatStore::separatesConversationsByPersona()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ChatStore store(dir.path() + QStringLiteral("/chat.db"));
    QVERIFY(store.ready());

    QVERIFY(store.ensureConversation(QStringLiteral("builtin-default")) > 0);
    QVERIFY(store.appendMessage(QStringLiteral("user"), QStringLiteral("旧会话")));

    QVERIFY(store.ensureConversation(QStringLiteral("cat")) > 0);
    QVERIFY(store.recentMessages(10).isEmpty());
    QVERIFY(store.appendMessage(QStringLiteral("user"), QStringLiteral("新会话")));
    QCOMPARE(store.recentMessages(10).size(), 1);

    QVERIFY(store.ensureConversation(QStringLiteral("cat")) > 0);
    QCOMPARE(store.recentMessages(10).size(), 1);
}

void TestChatStore::clearRemovesMessages()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ChatStore store(dir.path() + QStringLiteral("/chat.db"));
    QVERIFY(store.ready());
    QVERIFY(store.ensureConversation(QStringLiteral("builtin-default")) > 0);
    QVERIFY(store.appendMessage(QStringLiteral("user"), QStringLiteral("hi")));
    QVERIFY(store.clearCurrentMessages());
    QVERIFY(store.recentMessages(10).isEmpty());
}

QTEST_GUILESS_MAIN(TestChatStore)
