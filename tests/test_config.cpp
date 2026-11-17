#include "test_config.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "services/autostart.h"
#include "data/configstore.h"
#include "data/personastore.h"

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
        store.setPetPosition(0, 120.0, 240.0);
        QString error;
        QVERIFY2(store.save(&error), qPrintable(error));
    }

    ConfigStore loaded(path);
    QCOMPARE(loaded.values().baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
    QCOMPARE(loaded.values().apiKey, QStringLiteral("secret"));
    QCOMPARE(loaded.values().model, QStringLiteral("qwen2.5:7b"));
    QCOMPARE(loaded.values().personaId, QStringLiteral("cat"));
    QVERIFY(loaded.values().petPositions.size() >= 1);
    QCOMPARE(loaded.values().petPositions.at(0).x(), 120.0);
    QCOMPARE(loaded.values().petPositions.at(0).y(), 240.0);
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


void TestConfig::personaImportAsync()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PersonaStore store(dir.path());

    QDir().mkpath(dir.path() + QStringLiteral("/import"));
    const QString source = dir.path() + QStringLiteral("/import/dog.md");
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral("# 小狗\n你是小狗，说话要汪汪叫。").toUtf8());
    file.close();

    QSignalSpy spy(&store, &PersonaStore::changed);
    QFuture<PersonaImportResult> future = store.importFromAsync(source);
    future.waitForFinished();
    const PersonaImportResult result = future.result();
    QVERIFY2(result.ok, qPrintable(result.error));

    // changed() 由异步入口在对象所属线程发出后，列表才更新。
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
    QCOMPARE(store.personas().size(), 2);
    QCOMPARE(store.find(QStringLiteral("dog")).name, QStringLiteral("小狗"));

    // 同步入口仍然可用。
    QString error;
    QDir().mkpath(dir.path() + QStringLiteral("/import2"));
    const QString second = dir.path() + QStringLiteral("/import2/fox.md");
    QFile secondFile(second);
    QVERIFY(secondFile.open(QIODevice::WriteOnly));
    secondFile.write(QStringLiteral("# 狐狸").toUtf8());
    secondFile.close();
    QVERIFY2(store.importFrom(second, &error), qPrintable(error));
    QCOMPARE(store.personas().size(), 3);
}

void TestConfig::apiKeyIsEncrypted()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/config.json");

    ConfigStore store(path);
    store.setLlm(QStringLiteral("OpenAI"), QStringLiteral("https://api.example.com/v1"),
                 QStringLiteral("super-secret-key"), QStringLiteral("gpt-test"));
    QString error;
    QVERIFY2(store.save(&error), qPrintable(error));

    QFile raw(path);
    QVERIFY(raw.open(QIODevice::ReadOnly));
    const QByteArray content = raw.readAll();
    raw.close();

#ifdef Q_OS_WIN
    // DPAPI 密文不应包含明文 key。
    QVERIFY(!content.contains("super-secret-key"));
    QVERIFY(content.contains("apiKeyProtected"));
#else
    QVERIFY(content.contains("super-secret-key"));
#endif

    ConfigStore reloaded(path);
    QCOMPARE(reloaded.values().apiKey, QStringLiteral("super-secret-key"));
}

void TestConfig::autoStartRoundTrip()
{
#ifdef Q_OS_WIN
    const bool initial = autostart::enabled();
    autostart::setEnabled(true, QStringLiteral("C:/tmp/desktop_companion.exe"));
    QVERIFY(autostart::enabled());
    autostart::setEnabled(false);
    QVERIFY(!autostart::enabled());
    if (initial)
        autostart::setEnabled(true, QStringLiteral("C:/tmp/desktop_companion.exe"));
#endif
}

void TestConfig::corruptConfigFallsBack()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/config.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ this is not json");
    file.close();

    ConfigStore store(path);
    QCOMPARE(store.values().model, QStringLiteral("qwen2.5:7b"));
    QCOMPARE(store.values().theme, 0);
    QVERIFY(QFile::exists(path + QStringLiteral(".bak")));
}

void TestConfig::themeRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/config.json");
    {
        ConfigStore store(path);
        store.setTheme(1);
        QVERIFY(store.save());
    }
    ConfigStore loaded(path);
    QCOMPARE(loaded.values().theme, 1);
}

QTEST_GUILESS_MAIN(TestConfig)
