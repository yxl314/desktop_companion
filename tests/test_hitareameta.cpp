#include "test_hitareameta.h"

#include <QFile>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>
#include <QtTest>

#include "platform/hitareameta.h"

namespace
{
QByteArray validMetaJson()
{
    return QByteArrayLiteral(R"({
        "scale": 1.5,
        "anchor": [0.5, 0.9],
        "extra": "kept",
        "hitAreas": [
            {"id": "head", "name": "头", "rect": [0.1, 0.0, 0.8, 0.35], "lines": ["别摸头！", "喵"]},
            {"id": "body", "name": "身体", "rect": [0.2, 0.35, 0.6, 0.4], "lines": ["好痒"]},
            {"id": "feet", "rect": [0.25, 0.75, 0.5, 0.25]}
        ]
    })");
}
} // namespace

void TestHitAreaMeta::defaultsToEmpty()
{
    HitAreaMeta meta;
    QVERIFY(meta.isEmpty());
    QCOMPARE(meta.areaCount(), 0);
    QVERIFY(meta.hitAreaAt(QPointF(0.5, 0.5)).isEmpty());
    QCOMPARE(meta.scale(), 1.0);
    QVERIFY(meta.anchor() == QPointF(0.5, 0.5));
    QVERIFY(meta.meta().isEmpty());
    QVERIFY(meta.find(QStringLiteral("head")) == nullptr);

    // 空对象是合法 meta：空表 + 默认 scale/anchor。
    QVERIFY(meta.loadFromJson(QByteArrayLiteral("{}")));
    QVERIFY(meta.isEmpty());
    QCOMPARE(meta.scale(), 1.0);
    QVERIFY(meta.anchor() == QPointF(0.5, 0.5));
}

void TestHitAreaMeta::loadsValidMeta()
{
    HitAreaMeta meta;
    QString error;
    QVERIFY2(meta.loadFromJson(validMetaJson(), &error), qPrintable(error));
    QVERIFY(!meta.isEmpty());
    QCOMPARE(meta.areaCount(), 3);

    const QList<HitArea> &areas = meta.areas();
    const QStringList headLines = {QStringLiteral("别摸头！"), QStringLiteral("喵")};
    const QStringList bodyLines = {QStringLiteral("好痒")};
    QCOMPARE(areas.at(0).id, QStringLiteral("head"));
    QCOMPARE(areas.at(0).name, QStringLiteral("头"));
    QVERIFY(areas.at(0).rect == QRectF(0.1, 0.0, 0.8, 0.35));
    QVERIFY(areas.at(0).lines == headLines);

    QCOMPARE(areas.at(1).id, QStringLiteral("body"));
    QVERIFY(areas.at(1).lines == bodyLines);

    // name 缺省时回退为 id，lines 缺省为空。
    QCOMPARE(areas.at(2).id, QStringLiteral("feet"));
    QCOMPARE(areas.at(2).name, QStringLiteral("feet"));
    QVERIFY(areas.at(2).lines.isEmpty());

    QCOMPARE(meta.scale(), 1.5);
    QVERIFY(meta.anchor() == QPointF(0.5, 0.9));

    // QVariantMap 原样保留未识别字段。
    const QVariantMap raw = meta.meta();
    QCOMPARE(raw.value(QStringLiteral("extra")).toString(), QStringLiteral("kept"));
    QCOMPARE(raw.value(QStringLiteral("hitAreas")).toList().size(), 3);
}

void TestHitAreaMeta::hitAreaAtReturnsExpectedId()
{
    HitAreaMeta meta;
    QVERIFY(meta.loadFromJson(validMetaJson()));

    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.1)), QStringLiteral("head"));
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.34)), QStringLiteral("head"));
    // 边界：head 区下边界 0.35 属于 body（左闭右开）。
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.35)), QStringLiteral("body"));
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.5)), QStringLiteral("body"));
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.9)), QStringLiteral("feet"));
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.999)), QStringLiteral("feet"));

    // 未命中与越界。
    QVERIFY(meta.hitAreaAt(QPointF(0.05, 0.05)).isEmpty());
    QVERIFY(meta.hitAreaAt(QPointF(0.99, 0.5)).isEmpty());
    QVERIFY(meta.hitAreaAt(QPointF(-0.1, 0.5)).isEmpty());
    QVERIFY(meta.hitAreaAt(QPointF(0.5, 1.5)).isEmpty());

    const HitArea *feet = meta.find(QStringLiteral("feet"));
    QVERIFY(feet != nullptr);
    QCOMPARE(feet->id, QStringLiteral("feet"));
    QVERIFY(meta.find(QStringLiteral("missing")) == nullptr);
}

void TestHitAreaMeta::rejectsInvalidRect()
{
    HitAreaMeta meta;
    QString error;
    const auto makeMeta = [](const QString &rect) {
        return QStringLiteral(R"({"hitAreas":[{"id":"bad","rect":%1}]})").arg(rect).toUtf8();
    };

    QVERIFY(!meta.loadFromJson(makeMeta(QStringLiteral("[0.0, 0.0, 1.2, 1.0]")), &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!meta.loadFromJson(makeMeta(QStringLiteral("[-0.1, 0.0, 0.5, 1.0]")), &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!meta.loadFromJson(makeMeta(QStringLiteral("[0.0, 0.0, 0.0, 1.0]")), &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!meta.loadFromJson(makeMeta(QStringLiteral("[0.0, 0.0, 0.5, 1.1]")), &error));
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(!meta.loadFromJson(makeMeta(QStringLiteral("null")), &error));
    QVERIFY(!error.isEmpty());

    QVERIFY(meta.isEmpty());
}

void TestHitAreaMeta::rejectsInvalidStructure()
{
    HitAreaMeta meta;
    QString error;

    QVERIFY(!meta.loadFromJson(QByteArrayLiteral("not json"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral("[]"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral(R"({"hitAreas":{}})"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral(R"({"hitAreas":[{"rect":[0,0,1,1]}]})"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(
        QByteArrayLiteral(R"({"hitAreas":[{"id":"a","rect":[0,0,1,1]},{"id":"a","rect":[0,0,1,1]}]})"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(
        QByteArrayLiteral(R"({"hitAreas":[{"id":"a","rect":[0,0,0.5,0.5],"lines":"oops"}]})"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral(R"({"scale":0})"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral(R"({"anchor":"center"})"), &error));
    QVERIFY(!error.isEmpty());

    QVERIFY(meta.isEmpty());
}

void TestHitAreaMeta::loadsFromFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/meta.json");
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(validMetaJson()) > 0);
    }

    HitAreaMeta meta;
    QString error;
    QVERIFY2(meta.loadFromFile(path, &error), qPrintable(error));
    QCOMPARE(meta.areaCount(), 3);
    QCOMPARE(meta.hitAreaAt(QPointF(0.5, 0.1)), QStringLiteral("head"));

    // 文件不存在：失败且保留旧数据。
    error.clear();
    QVERIFY(!meta.loadFromFile(dir.path() + QStringLiteral("/missing.json"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!meta.isEmpty());

    error.clear();
    QVERIFY(!meta.loadFromFile(QString(), &error));
    QVERIFY(!error.isEmpty());
}

void TestHitAreaMeta::failureKeepsPreviousState()
{
    HitAreaMeta meta;
    QVERIFY(meta.loadFromJson(validMetaJson()));
    QCOMPARE(meta.areaCount(), 3);
    QCOMPARE(meta.scale(), 1.5);

    QString error;
    QVERIFY(!meta.loadFromJson(QByteArrayLiteral("{"), &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(meta.areaCount(), 3);
    QCOMPARE(meta.scale(), 1.5);
    QVERIFY(meta.anchor() == QPointF(0.5, 0.9));

    meta.clear();
    QVERIFY(meta.isEmpty());
    QCOMPARE(meta.scale(), 1.0);
    QVERIFY(meta.anchor() == QPointF(0.5, 0.5));
    QVERIFY(meta.meta().isEmpty());
}

void TestHitAreaMeta::acceptsObjectForms()
{
    const QByteArray json = QByteArrayLiteral(R"({
        "anchor": {"x": 0.25, "y": 0.75},
        "hitAreas": [
            {"id": "body", "rect": {"x": 0.1, "y": 0.2, "width": 0.4, "height": 0.5}}
        ]
    })");

    HitAreaMeta meta;
    QString error;
    QVERIFY2(meta.loadFromJson(json, &error), qPrintable(error));
    QCOMPARE(meta.areaCount(), 1);
    QVERIFY(meta.areas().at(0).rect == QRectF(0.1, 0.2, 0.4, 0.5));
    QVERIFY(meta.anchor() == QPointF(0.25, 0.75));
    QCOMPARE(meta.hitAreaAt(QPointF(0.2, 0.3)), QStringLiteral("body"));
}

QTEST_GUILESS_MAIN(TestHitAreaMeta)
