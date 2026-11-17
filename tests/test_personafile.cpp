#include "test_personafile.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "services/personafile.h"

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

void TestPersonaFile::parsesEnglishFrontmatter()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/小柚.md");
    QVERIFY(writeUtf8File(path,
                          QStringLiteral("---\n"
                                         "name: 小柚\n"
                                         "birthday: 2000-01-01\n"
                                         "zodiac: 摩羯座\n"
                                         "hobbies: 看书、晒太阳\n"
                                         "favorite_food: 草莓\n"
                                         "background: 来自桌面的小精灵\n"
                                         "---\n"
                                         "\n"
                                         "你是小柚，说话简短。\n")
                              .toUtf8()));

    PersonaData data;
    QString error;
    QVERIFY2(parsePersonaFile(path, &data, &error), qPrintable(error));
    QCOMPARE(data.id, QStringLiteral("小柚"));
    QCOMPARE(data.name, QStringLiteral("小柚"));
    QCOMPARE(data.birthday, QStringLiteral("2000-01-01"));
    QCOMPARE(data.zodiac, QStringLiteral("摩羯座"));
    QCOMPARE(data.hobbies, QStringLiteral("看书、晒太阳"));
    QCOMPARE(data.favoriteFood, QStringLiteral("草莓"));
    QCOMPARE(data.background, QStringLiteral("来自桌面的小精灵"));
    QCOMPARE(data.body, QStringLiteral("你是小柚，说话简短。"));
}

void TestPersonaFile::parsesChineseKeys()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/阿柚.md");
    QVERIFY(writeUtf8File(path,
                          QStringLiteral("---\n"
                                         "称呼：阿柚\n"
                                         "生日：2月14日\n"
                                         "星座：水瓶座\n"
                                         "爱好：听歌\n"
                                         "最爱的食物：芒果\n"
                                         "背景故事：住在屏幕里\n"
                                         "---\n"
                                         "正文第一行\n"
                                         "正文第二行\n")
                              .toUtf8()));

    PersonaData data;
    QString error;
    QVERIFY2(parsePersonaFile(path, &data, &error), qPrintable(error));
    QCOMPARE(data.name, QStringLiteral("阿柚"));
    QCOMPARE(data.birthday, QStringLiteral("2月14日"));
    QCOMPARE(data.zodiac, QStringLiteral("水瓶座"));
    QCOMPARE(data.hobbies, QStringLiteral("听歌"));
    QCOMPARE(data.favoriteFood, QStringLiteral("芒果"));
    QCOMPARE(data.background, QStringLiteral("住在屏幕里"));
    QCOMPARE(data.body, QStringLiteral("正文第一行\n正文第二行"));
}

void TestPersonaFile::withoutFrontmatterUsesFilename()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/小桃.md");
    QVERIFY(writeUtf8File(path, QStringLiteral("# 小桃\n\n你好呀。\n").toUtf8()));

    PersonaData data;
    QString error;
    QVERIFY2(parsePersonaFile(path, &data, &error), qPrintable(error));
    QCOMPARE(data.name, QStringLiteral("小桃"));
    QCOMPARE(data.body, QStringLiteral("# 小桃\n\n你好呀。"));
}

void TestPersonaFile::frontmatterWithoutNameUsesFilename()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/无名.md");
    QVERIFY(writeUtf8File(path, QStringLiteral("---\nbirthday: 1月1日\n---\n正文\n").toUtf8()));

    PersonaData data;
    QString error;
    QVERIFY2(parsePersonaFile(path, &data, &error), qPrintable(error));
    QCOMPARE(data.name, QStringLiteral("无名"));
    QCOMPARE(data.birthday, QStringLiteral("1月1日"));
    QCOMPARE(data.body, QStringLiteral("正文"));
}

void TestPersonaFile::missingFileFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PersonaData data;
    QString error;
    QVERIFY(!parsePersonaFile(dir.path() + QStringLiteral("/不存在.md"), &data, &error));
    QVERIFY(!error.isEmpty());

    QVERIFY(!parsePersonaFile(QString(), &data, &error));
    QVERIFY(!error.isEmpty());
}

void TestPersonaFile::malformedFrontmatterFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PersonaData data;
    QString error;
    // 缺少结束分隔符。
    const QString noClosing = dir.path() + QStringLiteral("/no-closing.md");
    QVERIFY(writeUtf8File(noClosing, QStringLiteral("---\nname: 小柚\n").toUtf8()));
    QVERIFY(!parsePersonaFile(noClosing, &data, &error));
    QVERIFY(!error.isEmpty());

    // 行内缺少冒号。
    const QString badLine = dir.path() + QStringLiteral("/bad-line.md");
    QVERIFY(writeUtf8File(badLine, QStringLiteral("---\nname: 小柚\n这不是键值\n---\n正文\n").toUtf8()));
    error.clear();
    QVERIFY(!parsePersonaFile(badLine, &data, &error));
    QVERIFY(error.contains(QStringLiteral("冒号")));

    // 无法从文本解析且没有文件名兜底。
    error.clear();
    QVERIFY(!parsePersonaMarkdown(QStringLiteral("正文"), QString(), &data, &error));
    QVERIFY(!error.isEmpty());
}

void TestPersonaFile::rendersRoundTrip()
{
    PersonaData source;
    source.id = QStringLiteral("小柚");
    source.name = QStringLiteral("小柚");
    source.birthday = QStringLiteral("2000-01-01");
    source.zodiac = QStringLiteral("摩羯座");
    source.hobbies = QStringLiteral("看书");
    source.favoriteFood = QStringLiteral("草莓");
    source.background = QStringLiteral("\"稀有\"背景\n第二行");
    source.body = QStringLiteral("第一行\n\n第二行");

    const QString markdown = renderPersonaMarkdown(source);
    QVERIFY(markdown.startsWith(QStringLiteral("---\n")));
    QVERIFY(markdown.contains(QStringLiteral("favorite_food: 草莓")));
    QVERIFY(markdown.endsWith(QStringLiteral("\n")));

    PersonaData parsed;
    QString error;
    QVERIFY2(parsePersonaMarkdown(markdown, QStringLiteral("小柚"), &parsed, &error), qPrintable(error));
    QCOMPARE(parsed.name, source.name);
    QCOMPARE(parsed.birthday, source.birthday);
    QCOMPARE(parsed.zodiac, source.zodiac);
    QCOMPARE(parsed.hobbies, source.hobbies);
    QCOMPARE(parsed.favoriteFood, source.favoriteFood);
    QCOMPARE(parsed.background, source.background);
    QCOMPARE(parsed.body, source.body);

    // 只有正文时也要能渲染并解析回来。
    PersonaData bodyOnly;
    bodyOnly.body = QStringLiteral("只有正文");
    const QString bodyMarkdown = renderPersonaMarkdown(bodyOnly);
    PersonaData bodyParsed;
    QVERIFY2(parsePersonaMarkdown(bodyMarkdown, QStringLiteral("名字"), &bodyParsed, &error), qPrintable(error));
    QCOMPARE(bodyParsed.name, QStringLiteral("名字"));
    QCOMPARE(bodyParsed.body, QStringLiteral("只有正文"));
}

void TestPersonaFile::sanitizesFileIds()
{
    QCOMPARE(sanitizeFileId(QStringLiteral(" 小柚 ")), QStringLiteral("小柚"));
    QCOMPARE(sanitizeFileId(QStringLiteral("a/b:c*?")), QStringLiteral("a_b_c"));
    QCOMPARE(sanitizeFileId(QStringLiteral("阿·尔")), QStringLiteral("阿_尔"));
    QCOMPARE(sanitizeFileId(QString()), QString());
    QCOMPARE(sanitizeFileId(QStringLiteral("   ")), QString());
    QCOMPARE(sanitizeFileId(QStringLiteral("...")), QString());
    QCOMPARE(sanitizeFileId(QStringLiteral("CON")), QStringLiteral("_CON"));
    QCOMPARE(sanitizeFileId(QStringLiteral("ok-name_1")), QStringLiteral("ok-name_1"));
}


void TestPersonaFile::asyncParseMatchesSync()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/async.md");
    QVERIFY(writeUtf8File(path, QStringLiteral("---\nname: 异步人设\n---\n\n正文。\n").toUtf8()));

    PersonaData syncData;
    QString syncError;
    QVERIFY2(parsePersonaFile(path, &syncData, &syncError), qPrintable(syncError));

    QFuture<PersonaParseResult> future = parsePersonaFileAsync(path);
    future.waitForFinished();
    const PersonaParseResult result = future.result();
    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.data.name, syncData.name);
    QCOMPARE(result.data.body, syncData.body);
    QCOMPARE(result.data.id, syncData.id);

    // 失败分支文案与同步版一致。
    PersonaData ignored;
    QString missingError;
    QVERIFY(!parsePersonaFile(dir.path() + QStringLiteral("/missing.md"), &ignored, &missingError));
    QFuture<PersonaParseResult> missing = parsePersonaFileAsync(dir.path() + QStringLiteral("/missing.md"));
    missing.waitForFinished();
    QVERIFY(!missing.result().ok);
    QCOMPARE(missing.result().error, missingError);
}

QTEST_GUILESS_MAIN(TestPersonaFile)
