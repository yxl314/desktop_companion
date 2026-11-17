#include "test_characterpack.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "services/characterpack.h"

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


void TestCharacterPack::asyncImportExportRoundTrip()
{
    QTemporaryDir sourceDir;
    QVERIFY(sourceDir.isValid());
    const QString characterDir = sourceDir.path() + QStringLiteral("/yuki");
    writeFile(characterDir + QStringLiteral("/card.md"), QByteArray("# 小雪\n你是小雪。"));
    writeFile(characterDir + QStringLiteral("/avatars/yuki.png"), QByteArray("png"));

    QTemporaryDir workDir;
    QVERIFY(workDir.isValid());
    const QString zipPath = workDir.path() + QStringLiteral("/async-pack.zip");

    // 异步导出：结果与同步版一致，错误文案保留。
    QFuture<characterpack::AsyncExportResult> exportFuture =
        characterpack::exportPackAsync(characterDir, zipPath);
    exportFuture.waitForFinished();
    const characterpack::AsyncExportResult exported = exportFuture.result();
    QVERIFY2(exported.ok, qPrintable(exported.error));
    QVERIFY(QFile::exists(zipPath));

    // 异步导入。
    const QString characters = workDir.path() + QStringLiteral("/characters");
    QDir().mkpath(characters);
    QFuture<characterpack::AsyncImportResult> importFuture =
        characterpack::importPackAsync(zipPath, characters);
    importFuture.waitForFinished();
    const characterpack::AsyncImportResult imported = importFuture.result();
    QVERIFY2(imported.ok, qPrintable(imported.error));
    QCOMPARE(imported.characterId, QStringLiteral("yuki"));
    QCOMPARE(readFile(characters + QStringLiteral("/yuki/card.md")), QByteArray("# 小雪\n你是小雪。"));

    // 失败分支同样返回可读错误文案。
    QFuture<characterpack::AsyncImportResult> missingFuture =
        characterpack::importPackAsync(workDir.path() + QStringLiteral("/missing.zip"), characters);
    missingFuture.waitForFinished();
    QVERIFY(!missingFuture.result().ok);
    QVERIFY(!missingFuture.result().error.isEmpty());
}

QTEST_GUILESS_MAIN(TestCharacterPack)
