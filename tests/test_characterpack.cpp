#include "test_characterpack.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "characterpack.h"

namespace
{
void writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
    file.close();
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}
} // namespace

void TestCharacterPack::exportImportRoundTrip()
{
    QTemporaryDir sourceDir;
    QVERIFY(sourceDir.isValid());
    const QString characterDir = sourceDir.path() + QStringLiteral("/yuki");
    writeFile(characterDir + QStringLiteral("/card.md"), QByteArray("# 小雪\n你是小雪。"));
    writeFile(characterDir + QStringLiteral("/meta.json"), QByteArray("{\"scale\":1.0}"));
    writeFile(characterDir + QStringLiteral("/avatars/yuki.png"), QByteArray("\x89PNG\r\n\x1a\n"));

    QTemporaryDir workDir;
    QVERIFY(workDir.isValid());
    const QString zipPath = workDir.path() + QStringLiteral("/pack.zip");
    QString error;
    QVERIFY2(characterpack::exportPack(characterDir, zipPath, &error), qPrintable(error));

    const QString targetDir = workDir.path() + QStringLiteral("/characters");
    QDir().mkpath(targetDir);
    QString newId;
    QVERIFY2(characterpack::importPack(zipPath, targetDir, &newId, &error), qPrintable(error));
    QCOMPARE(newId, QStringLiteral("yuki"));
    QCOMPARE(readFile(targetDir + QStringLiteral("/yuki/card.md")), QByteArray("# 小雪\n你是小雪。"));
    QCOMPARE(readFile(targetDir + QStringLiteral("/yuki/avatars/yuki.png")),
             QByteArray("\x89PNG\r\n\x1a\n"));
}

void TestCharacterPack::rejectsIncompletePack()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString characterDir = dir.path() + QStringLiteral("/broken");
    writeFile(characterDir + QStringLiteral("/card.md"), QByteArray("# 无形象"));

    const QString zipPath = dir.path() + QStringLiteral("/broken.zip");
    QString error;
    QVERIFY(characterpack::exportPack(characterDir, zipPath, &error));
    QVERIFY(!characterpack::importPack(zipPath, dir.path() + QStringLiteral("/characters"),
                                       nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterPack::rejectsExistingCharacter()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString source = dir.path() + QStringLiteral("/yuki");
    writeFile(source + QStringLiteral("/card.md"), QByteArray("# 小雪"));
    writeFile(source + QStringLiteral("/avatars/a.png"), QByteArray("png"));

    const QString zipPath = dir.path() + QStringLiteral("/pack.zip");
    QString error;
    QVERIFY(characterpack::exportPack(source, zipPath, &error));

    const QString characters = dir.path() + QStringLiteral("/characters");
    QString newId;
    QVERIFY(characterpack::importPack(zipPath, characters, &newId, &error));
    QVERIFY(!characterpack::importPack(zipPath, characters, nullptr, &error));
}

QTEST_GUILESS_MAIN(TestCharacterPack)
