#include "test_memorystore.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QtTest>

#include "data/memorystore.h"

void TestMemoryStore::addListAndCount()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY2(store.ready(), qPrintable(store.lastError()));
    QCOMPARE(store.schemaVersion(), 1);
    QCOMPARE(store.maxEntries(), MemoryStore::defaultMaxEntries);

    MemoryEntry first;
    first.characterId = QStringLiteral("builtin-default");
    first.personaId = QStringLiteral("default");
    first.content = QStringLiteral("  用户喜欢猫  ");
    first.tags = QStringList{QStringLiteral("爱好"), QStringLiteral("宠物")};
    QString error;
    QVERIFY2(store.add(&first, &error), qPrintable(error));
    QVERIFY(first.id > 0);
    QCOMPARE(first.content, QStringLiteral("用户喜欢猫"));

    MemoryEntry second;
    second.characterId = QStringLiteral("builtin-default");
    second.personaId = QStringLiteral("default");
    second.content = QStringLiteral("用户住在杭州");
    QVERIFY2(store.add(&second, &error), qPrintable(error));

    MemoryEntry other;
    other.characterId = QStringLiteral("c2");
    other.personaId = QStringLiteral("default");
    other.content = QStringLiteral("别的角色");
    QVERIFY2(store.add(&other, &error), qPrintable(error));

    QCOMPARE(store.count(QStringLiteral("builtin-default"), QStringLiteral("default")), 2);
    QCOMPARE(store.count(QStringLiteral("c2"), QStringLiteral("default")), 1);

    const QVector<MemoryEntry> entries =
        store.list(QStringLiteral("builtin-default"), QStringLiteral("default"));
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).id, second.id);
    QCOMPARE(entries.at(1).content, QStringLiteral("用户喜欢猫"));
    QCOMPARE(entries.at(1).tags.size(), 2);
    QCOMPARE(entries.at(1).characterId, QStringLiteral("builtin-default"));
}

void TestMemoryStore::updateAndPin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    MemoryEntry entry;
    entry.characterId = QStringLiteral("c1");
    entry.personaId = QStringLiteral("p1");
    entry.content = QStringLiteral("旧内容");
    QString error;
    QVERIFY2(store.add(&entry, &error), qPrintable(error));

    entry.content = QStringLiteral("新内容");
    entry.pinned = true;
    entry.tags = QStringList{QStringLiteral("重要")};
    QVERIFY2(store.update(entry, &error), qPrintable(error));

    QVector<MemoryEntry> entries = store.list(QStringLiteral("c1"), QStringLiteral("p1"));
    QCOMPARE(entries.size(), 1);
    QVERIFY(entries.first().pinned);
    QCOMPARE(entries.first().content, QStringLiteral("新内容"));
    QCOMPARE(entries.first().tags, QStringList{QStringLiteral("重要")});

    QVERIFY2(store.setPinned(entry.id, false, &error), qPrintable(error));
    entries = store.list(QStringLiteral("c1"), QStringLiteral("p1"));
    QVERIFY(!entries.first().pinned);
}

void TestMemoryStore::rejectsInvalidEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());
    QString error;

    MemoryEntry emptyContent;
    emptyContent.characterId = QStringLiteral("c1");
    emptyContent.content = QStringLiteral("   ");
    QVERIFY(!store.add(&emptyContent, &error));
    QVERIFY(!error.isEmpty());

    MemoryEntry noCharacter;
    noCharacter.content = QStringLiteral("内容");
    QVERIFY(!store.add(&noCharacter, &error));
    QVERIFY(!error.isEmpty());

    QVERIFY(!store.add(nullptr, &error));

    MemoryEntry missing;
    missing.id = 999;
    missing.characterId = QStringLiteral("c1");
    missing.content = QStringLiteral("不存在");
    QVERIFY(!store.update(missing, &error));
    QVERIFY(!store.remove(999, &error));
    QVERIFY(!store.setPinned(999, true, &error));
}

void TestMemoryStore::enforcesMaxEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    store.setMaxEntries(0);
    QCOMPARE(store.maxEntries(), 1);
    store.setMaxEntries(3);
    QCOMPARE(store.maxEntries(), 3);

    QString error;
    for (int index = 0; index < 3; ++index) {
        MemoryEntry entry;
        entry.characterId = QStringLiteral("c1");
        entry.personaId = QStringLiteral("p1");
        entry.content = QStringLiteral("记忆 %1").arg(index);
        QVERIFY2(store.add(&entry, &error), qPrintable(error));
    }

    MemoryEntry overflow;
    overflow.characterId = QStringLiteral("c1");
    overflow.personaId = QStringLiteral("p1");
    overflow.content = QStringLiteral("超出上限");
    QVERIFY(!store.add(&overflow, &error));
    QVERIFY(error.contains(QStringLiteral("上限")));
    QCOMPARE(store.count(QStringLiteral("c1"), QStringLiteral("p1")), 3);

    // 上限按分区计算，其他分区仍可写入。
    MemoryEntry otherPartition;
    otherPartition.characterId = QStringLiteral("c1");
    otherPartition.personaId = QStringLiteral("p2");
    otherPartition.content = QStringLiteral("另一个分区");
    QVERIFY2(store.add(&otherPartition, &error), qPrintable(error));
}

void TestMemoryStore::searchRanksKeywordThenFreshness()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    const QString characterId = QStringLiteral("c1");
    const QString personaId = QStringLiteral("p1");
    const QDateTime now = QDateTime::currentDateTimeUtc();

    const auto insert = [&store, &characterId, &personaId](const QString &content, const QDateTime &usedAt) {
        MemoryEntry entry;
        entry.characterId = characterId;
        entry.personaId = personaId;
        entry.content = content;
        entry.createdAt = usedAt.toString(Qt::ISODate);
        entry.lastUsedAt = entry.createdAt;
        QString error;
        const bool ok = store.add(&entry, &error);
        return ok ? entry.id : -1;
    };

    QVERIFY(insert(QStringLiteral("我喜欢吃苹果"), now.addDays(-30)) > 0);
    QVERIFY(insert(QStringLiteral("苹果和香蕉都好吃"), now.addDays(-60)) > 0);
    QVERIFY(insert(QStringLiteral("苹果很甜"), now.addDays(-90)) > 0);
    QVERIFY(insert(QStringLiteral("今天下雨了"), now) > 0);

    QString error;
    const QVector<MemoryEntry> results =
        store.search(characterId, personaId, QStringLiteral("苹果 香蕉"), 5, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(results.size(), 4);
    // 关键词命中数优先：2 命中 > 1 命中；同命中数时 last_used_at 越新越靠前。
    QCOMPARE(results.at(0).content, QStringLiteral("苹果和香蕉都好吃"));
    QCOMPARE(results.at(1).content, QStringLiteral("我喜欢吃苹果"));
    QCOMPARE(results.at(2).content, QStringLiteral("苹果很甜"));
    QCOMPARE(results.at(3).content, QStringLiteral("今天下雨了"));

    const QVector<MemoryEntry> limited =
        store.search(characterId, personaId, QStringLiteral("苹果 香蕉"), 2, &error);
    QCOMPARE(limited.size(), 2);
}

void TestMemoryStore::searchRanksFreshnessForEmptyQuery()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    const QString characterId = QStringLiteral("c1");
    const QString personaId = QStringLiteral("p1");
    const QDateTime now = QDateTime::currentDateTimeUtc();

    MemoryEntry older;
    older.characterId = characterId;
    older.personaId = personaId;
    older.content = QStringLiteral("旧记忆");
    older.createdAt = now.addDays(-10).toString(Qt::ISODate);
    older.lastUsedAt = older.createdAt;
    QString error;
    QVERIFY2(store.add(&older, &error), qPrintable(error));

    MemoryEntry newer;
    newer.characterId = characterId;
    newer.personaId = personaId;
    newer.content = QStringLiteral("新记忆");
    newer.createdAt = now.toString(Qt::ISODate);
    newer.lastUsedAt = newer.createdAt;
    QVERIFY2(store.add(&newer, &error), qPrintable(error));

    const QVector<MemoryEntry> results = store.search(characterId, personaId, QString(), 5, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).content, QStringLiteral("新记忆"));
    QCOMPARE(results.at(1).content, QStringLiteral("旧记忆"));
}

void TestMemoryStore::pinnedAlwaysReturned()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    const QString characterId = QStringLiteral("c1");
    const QString personaId = QStringLiteral("p1");
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QString error;

    const auto insert = [&store, &characterId, &personaId](const QString &content, bool pinned,
                                                           const QDateTime &usedAt) {
        MemoryEntry entry;
        entry.characterId = characterId;
        entry.personaId = personaId;
        entry.content = content;
        entry.pinned = pinned;
        entry.createdAt = usedAt.toString(Qt::ISODate);
        entry.lastUsedAt = entry.createdAt;
        QString insertError;
        const bool ok = store.add(&entry, &insertError);
        return ok ? entry.id : -1;
    };

    const qint64 pinnedOld = insert(QStringLiteral("置顶旧"), true, now.addDays(-2));
    const qint64 pinnedNew = insert(QStringLiteral("置顶新"), true, now);
    QVERIFY(pinnedOld > 0);
    QVERIFY(pinnedNew > 0);
    for (int index = 0; index < 6; ++index) {
        QVERIFY(insert(QStringLiteral("猫 %1").arg(index), false,
                      now.addSecs(-static_cast<qint64>(index) * 60)) > 0);
    }

    const QVector<MemoryEntry> results =
        store.search(characterId, personaId, QStringLiteral("猫"), 2, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    // 2 条置顶全部返回，另加 2 条非置顶（limit 只约束非置顶条目）。
    QCOMPARE(results.size(), 4);
    QVERIFY(results.at(0).pinned);
    QVERIFY(results.at(1).pinned);
    QCOMPARE(results.at(0).id, pinnedNew);
    QCOMPARE(results.at(1).id, pinnedOld);
    QVERIFY(!results.at(2).pinned);
    QVERIFY(!results.at(3).pinned);
}

void TestMemoryStore::searchUpdatesLastUsedAt()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());

    const QDateTime now = QDateTime::currentDateTimeUtc();
    MemoryEntry entry;
    entry.characterId = QStringLiteral("c1");
    entry.personaId = QStringLiteral("p1");
    entry.content = QStringLiteral("用户喜欢喝咖啡");
    entry.createdAt = now.addDays(-20).toString(Qt::ISODate);
    entry.lastUsedAt = entry.createdAt;
    QString error;
    QVERIFY2(store.add(&entry, &error), qPrintable(error));

    const QString oldUsedAt = entry.lastUsedAt;
    const QVector<MemoryEntry> results =
        store.search(QStringLiteral("c1"), QStringLiteral("p1"), QStringLiteral("咖啡"), 5, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(results.size(), 1);
    QVERIFY(!results.first().lastUsedAt.isEmpty());
    QVERIFY(results.first().lastUsedAt != oldUsedAt);

    const QVector<MemoryEntry> reloaded = store.list(QStringLiteral("c1"), QStringLiteral("p1"));
    QCOMPARE(reloaded.size(), 1);
    QVERIFY(reloaded.first().lastUsedAt != oldUsedAt);
}

void TestMemoryStore::resetForPersonaAndRemove()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MemoryStore store(dir.path() + QStringLiteral("/memory.db"));
    QVERIFY(store.ready());
    QString error;

    const auto insert = [&store](const QString &characterId, const QString &personaId, const QString &content) {
        MemoryEntry entry;
        entry.characterId = characterId;
        entry.personaId = personaId;
        entry.content = content;
        QString insertError;
        const bool ok = store.add(&entry, &insertError);
        return ok ? entry.id : -1;
    };

    QVERIFY(insert(QStringLiteral("c1"), QStringLiteral("p1"), QStringLiteral("a")) > 0);
    QVERIFY(insert(QStringLiteral("c1"), QStringLiteral("p1"), QStringLiteral("b")) > 0);
    const qint64 keepPersona = insert(QStringLiteral("c1"), QStringLiteral("p2"), QStringLiteral("c"));
    const qint64 keepCharacter = insert(QStringLiteral("c2"), QStringLiteral("p1"), QStringLiteral("d"));
    QVERIFY(keepPersona > 0);
    QVERIFY(keepCharacter > 0);

    QVERIFY2(store.resetForPersona(QStringLiteral("c1"), QStringLiteral("p1"), &error), qPrintable(error));
    QCOMPARE(store.count(QStringLiteral("c1"), QStringLiteral("p1")), 0);
    QCOMPARE(store.count(QStringLiteral("c1"), QStringLiteral("p2")), 1);
    QCOMPARE(store.count(QStringLiteral("c2"), QStringLiteral("p1")), 1);

    QVERIFY2(store.remove(keepPersona, &error), qPrintable(error));
    QCOMPARE(store.count(QStringLiteral("c1"), QStringLiteral("p2")), 0);
    QVERIFY(!store.remove(keepPersona, &error));
}

void TestMemoryStore::persistsAcrossReopen()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/memory.db");

    qint64 pinnedId = -1;
    {
        MemoryStore store(path);
        QVERIFY(store.ready());
        MemoryEntry entry;
        entry.characterId = QStringLiteral("c1");
        entry.personaId = QStringLiteral("p1");
        entry.content = QStringLiteral("重启后仍记得");
        entry.pinned = true;
        QString error;
        QVERIFY2(store.add(&entry, &error), qPrintable(error));
        pinnedId = entry.id;
    }

    MemoryStore reopened(path);
    QVERIFY2(reopened.ready(), qPrintable(reopened.lastError()));
    QCOMPARE(reopened.count(QStringLiteral("c1"), QStringLiteral("p1")), 1);
    const QVector<MemoryEntry> entries = reopened.list(QStringLiteral("c1"), QStringLiteral("p1"));
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().id, pinnedId);
    QVERIFY(entries.first().pinned);

    QString error;
    const QVector<MemoryEntry> results =
        reopened.search(QStringLiteral("c1"), QStringLiteral("p1"), QStringLiteral("记得"), 5, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(results.size(), 1);
}

QTEST_GUILESS_MAIN(TestMemoryStore)
