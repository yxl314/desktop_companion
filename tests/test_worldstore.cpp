#include "test_worldstore.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "worldstore.h"

namespace
{
bool writeUtf8File(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(content) == content.size();
}
} // namespace

void TestWorldStore::createsAndListsWorlds()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
    QCOMPARE(id, QStringLiteral("魔法学院"));
    QVERIFY(store.contains(id));
    QVERIFY(QFile::exists(store.worldPath(id)));

    QCOMPARE(store.list().size(), 1);
    QCOMPARE(store.list().first().id, id);
    QCOMPARE(store.list().first().name, QStringLiteral("魔法学院"));
    QVERIFY(store.list().first().content.isEmpty());

    QString secondId;
    QVERIFY2(store.create(QStringLiteral("深海遗迹"), &secondId, &error), qPrintable(error));
    QCOMPARE(secondId, QStringLiteral("深海遗迹"));
    QCOMPARE(store.list().size(), 2);
}

void TestWorldStore::rejectsInvalidOrDuplicateNames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY(!store.create(QString(), &id, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.create(QStringLiteral("   "), &id, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
    error.clear();
    QVERIFY(!store.create(QStringLiteral("魔法学院"), &id, &error));
    QVERIFY(error.contains(QStringLiteral("已存在")));

    error.clear();
    QString slashId;
    QVERIFY2(store.create(QStringLiteral("a/b"), &slashId, &error), qPrintable(error));
    QCOMPARE(slashId, QStringLiteral("a_b"));

    QVERIFY(!store.contains(QStringLiteral("a/b")));
    QVERIFY(!store.contains(QString()));
    World world;
    error.clear();
    QVERIFY(!store.read(QStringLiteral("a/b"), &world, &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::readsAndWritesContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));

    World world;
    QVERIFY2(store.read(id, &world, &error), qPrintable(error));
    QCOMPARE(world.id, id);
    QCOMPARE(world.name, QStringLiteral("魔法学院"));
    QVERIFY(world.content.isEmpty());

    world.name = QStringLiteral("魔法学院·新版");
    world.content = QStringLiteral("# 世界设定\n\n这里有魔法。");
    QVERIFY2(store.write(id, world, &error), qPrintable(error));

    World reloaded;
    QVERIFY2(store.read(id, &reloaded, &error), qPrintable(error));
    QCOMPARE(reloaded.name, QStringLiteral("魔法学院·新版"));
    QCOMPARE(reloaded.content, QStringLiteral("# 世界设定\n\n这里有魔法。"));

    // name 为空时用 id 兜底。
    World unnamed;
    unnamed.content = QStringLiteral("只有正文");
    QVERIFY2(store.write(id, unnamed, &error), qPrintable(error));
    World fallback;
    QVERIFY(store.read(id, &fallback));
    QCOMPARE(fallback.name, id);
    QCOMPARE(fallback.content, QStringLiteral("只有正文"));

    // 写入一个新 id 会创建世界观目录。
    World fresh;
    fresh.id = QStringLiteral("新世界");
    fresh.name = QStringLiteral("新世界");
    fresh.content = QStringLiteral("正文");
    QVERIFY2(store.write(QStringLiteral("新世界"), fresh, &error), qPrintable(error));
    QVERIFY(store.contains(QStringLiteral("新世界")));

    // 非法 id 要拒绝。
    error.clear();
    QVERIFY(!store.write(QStringLiteral("a/b"), fresh, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.read(QStringLiteral("不存在"), &world, &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::importsExternalWorld()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    const QString path = dir.path() + QStringLiteral("/奇幻大陆.md");
    QVERIFY(writeUtf8File(path, QStringLiteral("---\nname: 奇幻大陆\n---\n\n这里有龙。\n").toUtf8()));

    QString id;
    QString error;
    QVERIFY2(store.importFrom(path, &id, &error), qPrintable(error));
    QCOMPARE(id, QStringLiteral("奇幻大陆"));
    World world;
    QVERIFY2(store.read(id, &world, &error), qPrintable(error));
    QCOMPARE(world.name, QStringLiteral("奇幻大陆"));
    QCOMPARE(world.content, QStringLiteral("这里有龙。"));

    // 同一个世界观不能重复导入。
    error.clear();
    QVERIFY(!store.importFrom(path, &id, &error));
    QVERIFY(error.contains(QStringLiteral("已存在")));

    // 无 frontmatter：文件名作名字，整文件作正文。
    const QString plainPath = dir.path() + QStringLiteral("/深海.md");
    QVERIFY(writeUtf8File(plainPath, QStringLiteral("# 深海\n\n海底遗迹。\n").toUtf8()));
    QString plainId;
    error.clear();
    QVERIFY2(store.importFrom(plainPath, &plainId, &error), qPrintable(error));
    QCOMPARE(plainId, QStringLiteral("深海"));
    World plain;
    QVERIFY(store.read(plainId, &plain));
    QCOMPARE(plain.name, QStringLiteral("深海"));
    QCOMPARE(plain.content, QStringLiteral("# 深海\n\n海底遗迹。"));

    // 文件不存在。
    error.clear();
    QVERIFY(!store.importFrom(dir.path() + QStringLiteral("/missing.md"), &id, &error));
    QVERIFY(!error.isEmpty());

    // frontmatter 损坏。
    const QString badPath = dir.path() + QStringLiteral("/损坏.md");
    QVERIFY(writeUtf8File(badPath, QStringLiteral("---\nname: x\n").toUtf8()));
    error.clear();
    QVERIFY(!store.importFrom(badPath, &id, &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::renamesWorld()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
    World world;
    world.name = QStringLiteral("魔法学院");
    world.content = QStringLiteral("设定正文");
    QVERIFY2(store.write(id, world, &error), qPrintable(error));

    QVERIFY2(store.rename(id, QStringLiteral("星辰学院"), &error), qPrintable(error));
    World renamed;
    QVERIFY(store.read(id, &renamed));
    QCOMPARE(renamed.name, QStringLiteral("星辰学院"));
    QCOMPARE(renamed.content, QStringLiteral("设定正文"));
    QCOMPARE(store.list().first().name, QStringLiteral("星辰学院"));

    error.clear();
    QVERIFY(!store.rename(QStringLiteral("不存在"), QStringLiteral("名字"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.rename(id, QStringLiteral("   "), &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::removesWorld()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
    QVERIFY(QDir(store.worldDir(id)).exists());

    QVERIFY2(store.remove(id, &error), qPrintable(error));
    QVERIFY(!store.contains(id));
    QVERIFY(!QDir(store.worldDir(id)).exists());
    QVERIFY(store.list().isEmpty());

    error.clear();
    QVERIFY(!store.remove(id, &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::corruptWorldSkipped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorldStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
    QVERIFY(writeUtf8File(store.worldPath(id), QStringLiteral("---\nname: 损坏\n").toUtf8()));

    QVERIFY(store.list().isEmpty());
    World world;
    error.clear();
    QVERIFY(!store.read(id, &world, &error));
    QVERIFY(!error.isEmpty());
}

void TestWorldStore::persistsAcrossInstances()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString id;
    QString error;
    {
        WorldStore store(dir.path());
        QVERIFY2(store.create(QStringLiteral("魔法学院"), &id, &error), qPrintable(error));
        World world;
        world.name = QStringLiteral("魔法学院");
        world.content = QStringLiteral("跨越实例");
        QVERIFY2(store.write(id, world, &error), qPrintable(error));
    }

    WorldStore reopened(dir.path());
    QCOMPARE(reopened.list().size(), 1);
    QCOMPARE(reopened.list().first().id, id);
    World world;
    QVERIFY(reopened.read(id, &world));
    QCOMPARE(world.name, QStringLiteral("魔法学院"));
    QCOMPARE(world.content, QStringLiteral("跨越实例"));
}

QTEST_GUILESS_MAIN(TestWorldStore)
