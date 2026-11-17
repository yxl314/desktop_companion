#include "test_intimacy.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include "data/intimacyrepository.h"

void TestIntimacy::defaultStateAndLevelThresholds()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());

    QString error;
    QVERIFY2(service.load(QStringLiteral("builtin-default"), &error), qPrintable(error));
    QVERIFY(service.loaded());
    QCOMPARE(service.characterId(), QStringLiteral("builtin-default"));
    QCOMPARE(service.score(), 0);
    QVERIFY(service.level() == IntimacyService::Level::Stranger);
    QCOMPARE(service.levelName(), QStringLiteral("陌生"));
    QVERIFY(!service.decayEnabled());
    QVERIFY(service.events().isEmpty());
    QVERIFY(!QFile::exists(service.filePath(QStringLiteral("builtin-default"))));

    QCOMPARE(IntimacyService::levelNameForScore(0), QStringLiteral("陌生"));
    QCOMPARE(IntimacyService::levelNameForScore(19), QStringLiteral("陌生"));
    QCOMPARE(IntimacyService::levelNameForScore(20), QStringLiteral("熟悉"));
    QCOMPARE(IntimacyService::levelNameForScore(49), QStringLiteral("熟悉"));
    QCOMPARE(IntimacyService::levelNameForScore(50), QStringLiteral("亲近"));
    QCOMPARE(IntimacyService::levelNameForScore(79), QStringLiteral("亲近"));
    QCOMPARE(IntimacyService::levelNameForScore(80), QStringLiteral("信赖"));
    QCOMPARE(IntimacyService::levelNameForScore(100), QStringLiteral("信赖"));
}

void TestIntimacy::addClampsAndRecordsEvents()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    QString error;
    QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));

    QVERIFY2(service.add(60, QStringLiteral("对话"), &error), qPrintable(error));
    QCOMPARE(service.score(), 60);
    QVERIFY(service.level() == IntimacyService::Level::Close);
    QCOMPARE(service.levelName(), QStringLiteral("亲近"));
    QCOMPARE(static_cast<int>(service.events().size()), 1);
    QCOMPARE(service.events().first().delta, 60);
    QCOMPARE(service.events().first().score, 60);
    QCOMPARE(service.events().first().reason, QStringLiteral("对话"));

    QVERIFY2(service.add(60, QStringLiteral("点击"), &error), qPrintable(error));
    QCOMPARE(service.score(), 100);
    QVERIFY(service.level() == IntimacyService::Level::Trusted);
    QCOMPARE(static_cast<int>(service.events().size()), 2);
    // 事件记录的是实际生效的变化量（被 100 上限截断后为 +40）。
    QCOMPARE(service.events().last().delta, 40);

    QVERIFY2(service.add(-1000, QStringLiteral("重置"), &error), qPrintable(error));
    QCOMPARE(service.score(), 0);
    QCOMPARE(static_cast<int>(service.events().size()), 3);
    QCOMPARE(service.events().last().delta, -100);

    // 无实际变化时不产生事件。
    QVERIFY2(service.add(0, QStringLiteral("无变化"), &error), qPrintable(error));
    QCOMPARE(static_cast<int>(service.events().size()), 3);
}

void TestIntimacy::persistsAcrossServices()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    {
        IntimacyService service(dir.path());
        QString error;
        QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));
        QVERIFY2(service.add(7, QStringLiteral("测试"), &error), qPrintable(error));
        QVERIFY2(service.setDecayEnabled(true, &error), qPrintable(error));
    }

    IntimacyService reloaded(dir.path());
    QString error;
    QVERIFY2(reloaded.load(QStringLiteral("c1"), &error), qPrintable(error));
    QCOMPARE(reloaded.score(), 7);
    QVERIFY(reloaded.decayEnabled());
    QCOMPARE(static_cast<int>(reloaded.events().size()), 1);
    QCOMPARE(reloaded.events().first().reason, QStringLiteral("测试"));

    QFile file(reloaded.filePath(QStringLiteral("c1")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("score")).toInt(), 7);
    QCOMPARE(document.object().value(QStringLiteral("characterId")).toString(), QStringLiteral("c1"));
}

void TestIntimacy::corruptFileFallsBackWithBackup()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    const QString path = service.filePath(QStringLiteral("c1"));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ 这不是 JSON");
    file.close();

    QString error;
    QVERIFY(!service.load(QStringLiteral("c1"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains(QStringLiteral("损坏")));
    QCOMPARE(service.score(), 0);
    QVERIFY(QFile::exists(path + QStringLiteral(".bak")));

    // 回退默认值后仍能正常写入新文件。
    QVERIFY2(service.add(5, QStringLiteral("恢复"), &error), qPrintable(error));
    QVERIFY(QFile::exists(path));

    QFile recovered(path);
    QVERIFY(recovered.open(QIODevice::ReadOnly));
    const QJsonDocument document = QJsonDocument::fromJson(recovered.readAll());
    recovered.close();
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("score")).toInt(), 5);
}

void TestIntimacy::decayDisabledByDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    QString error;
    QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));
    QVERIFY2(service.add(30, QStringLiteral("对话"), &error), qPrintable(error));
    QVERIFY(!service.decayEnabled());

    const QDate today = QDate::currentDate();
    QVERIFY2(service.applyDailyDecay(10, today.addDays(5), &error), qPrintable(error));
    QCOMPARE(service.score(), 30);
}

void TestIntimacy::dailyDecayAppliesPerDay()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    const QDate today = QDate::currentDate();

    QString error;
    QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));
    QVERIFY2(service.setDecayEnabled(true, &error), qPrintable(error));
    QVERIFY(service.decayEnabled());
    QVERIFY2(service.add(50, QStringLiteral("对话"), &error), qPrintable(error));

    QVERIFY2(service.applyDailyDecay(2, today.addDays(2), &error), qPrintable(error));
    QCOMPARE(service.score(), 46);
    // 同一天重复调用不重复扣分。
    QVERIFY2(service.applyDailyDecay(2, today.addDays(2), &error), qPrintable(error));
    QCOMPARE(service.score(), 46);
    QVERIFY2(service.applyDailyDecay(1, today.addDays(3), &error), qPrintable(error));
    QCOMPARE(service.score(), 45);
    // 不会扣到负数。
    QVERIFY2(service.applyDailyDecay(1000, today.addDays(400), &error), qPrintable(error));
    QCOMPARE(service.score(), 0);

    bool hasDecayEvent = false;
    for (const IntimacyEvent &event : service.events()) {
        if (event.reason.contains(QStringLiteral("衰减")))
            hasDecayEvent = true;
    }
    QVERIFY(hasDecayEvent);

    IntimacyService reloaded(dir.path());
    QVERIFY2(reloaded.load(QStringLiteral("c1"), &error), qPrintable(error));
    QVERIFY(reloaded.decayEnabled());
    QCOMPARE(reloaded.score(), 0);
}

void TestIntimacy::jsonRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    QString error;
    QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));
    QVERIFY2(service.add(33, QStringLiteral("对话"), &error), qPrintable(error));
    QVERIFY2(service.setDecayEnabled(true, &error), qPrintable(error));

    const QJsonObject object = service.toJson();
    QCOMPARE(object.value(QStringLiteral("score")).toInt(), 33);
    QCOMPARE(object.value(QStringLiteral("characterId")).toString(), QStringLiteral("c1"));
    QVERIFY(object.value(QStringLiteral("decayEnabled")).toBool());

    IntimacyService copy(dir.path());
    QVERIFY2(copy.fromJson(object, &error), qPrintable(error));
    QCOMPARE(copy.characterId(), QStringLiteral("c1"));
    QCOMPARE(copy.score(), 33);
    QVERIFY(copy.decayEnabled());
    QCOMPARE(static_cast<int>(copy.events().size()), 1);
    QCOMPARE(copy.events().first().delta, 33);

    QJsonObject broken = object;
    broken.insert(QStringLiteral("score"), QStringLiteral("不是数字"));
    IntimacyService invalid(dir.path());
    QVERIFY(!invalid.fromJson(broken, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(invalid.score(), 0);
}

void TestIntimacy::eventCapEnforced()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    IntimacyService service(dir.path());
    QString error;
    QVERIFY2(service.load(QStringLiteral("c1"), &error), qPrintable(error));

    for (int index = 0; index < 55; ++index) {
        QVERIFY2(service.add(1, QStringLiteral("事件%1").arg(index), &error), qPrintable(error));
    }
    QCOMPARE(service.score(), 55);
    QCOMPARE(static_cast<int>(service.events().size()), IntimacyService::maxEvents);
    QCOMPARE(service.events().first().reason, QStringLiteral("事件5"));
    QCOMPARE(service.events().last().reason, QStringLiteral("事件54"));
}

QTEST_GUILESS_MAIN(TestIntimacy)
