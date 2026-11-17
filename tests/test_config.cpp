#include "test_config.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "configstore.h"
#include "personastore.h"

void TestConfig::savesAndLoads()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/config.json");

    {
        ConfigStore store(path);
        store.setLlm(QStringLiteral("Ollama"), QStringLiteral("http://127.0.0.1:11434/v1"),
                     QStringLiteral("secret"), QStringLiteral("qwen2.5:7b"));
        store.setPersonaId(QStringLiteral("cat"));
        store.setPetPosition(120.0, 240.0);
        QString error;
        QVERIFY2(store.save(&error), qPrintable(error));
    }

    ConfigStore loaded(path);
    QCOMPARE(loaded.values().baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
    QCOMPARE(loaded.values().apiKey, QStringLiteral("secret"));
    QCOMPARE(loaded.values().model, QStringLiteral("qwen2.5:7b"));
    QCOMPARE(loaded.values().personaId, QStringLiteral("cat"));
    QCOMPARE(loaded.values().petX, 120.0);
    QCOMPARE(loaded.values().petY, 240.0);
}

void TestConfig::personaImportAndRemove()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PersonaStore store(dir.path());

    QCOMPARE(store.personas().size(), 1);
    QVERIFY(store.personas().first().builtin);

    QDir().mkpath(dir.path() + QStringLiteral("/import"));
    const QString source = dir.path() + QStringLiteral("/import/cat.md");
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral("# 小猫\n你是小猫，说话要喵喵叫。").toUtf8());
    file.close();

    QString error;
    QVERIFY2(store.importFrom(source, &error), qPrintable(error));
    QCOMPARE(store.personas().size(), 2);
    QCOMPARE(store.find(QStringLiteral("cat")).name, QStringLiteral("小猫"));
    QVERIFY(store.find(QStringLiteral("cat")).prompt.contains(QStringLiteral("喵喵叫")));

    QVERIFY(store.remove(QStringLiteral("cat")));
    QCOMPARE(store.personas().size(), 1);
    QVERIFY(!store.remove(QStringLiteral("builtin-default")));
}

QTEST_GUILESS_MAIN(TestConfig)
