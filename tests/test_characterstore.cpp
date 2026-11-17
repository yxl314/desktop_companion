#include "test_characterstore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include "characterstore.h"

namespace
{
bool writeBytesFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(content) == content.size();
}
} // namespace

void TestCharacterStore::createsAndListsCharacters()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));
    QCOMPARE(id, QStringLiteral("小柚"));
    QVERIFY(store.contains(id));
    QVERIFY(QFile::exists(store.cardPath(id)));
    QVERIFY(QFile::exists(store.metaPath(id)));
    QVERIFY(QDir(store.avatarsDir(id)).exists());
    QVERIFY(QDir(store.personasDir(id)).exists());

    QString secondId;
    QVERIFY2(store.create(QStringLiteral("阿桃"), &secondId, &error), qPrintable(error));
    QCOMPARE(secondId, QStringLiteral("阿桃"));

    const QList<Character> characters = store.list();
    QCOMPARE(characters.size(), 2);
    QStringList ids;
    for (const Character &character : characters) {
        ids.append(character.id);
        QVERIFY(character.enabled);
        QVERIFY(character.worldId.isEmpty());
        QVERIFY(character.avatarPath.isEmpty());
    }
    QVERIFY(ids.contains(id));
    QVERIFY(ids.contains(secondId));

    // 列表显示名来自 card.md。
    for (const Character &character : characters) {
        if (character.id == id)
            QCOMPARE(character.name, QStringLiteral("小柚"));
        else
            QCOMPARE(character.name, QStringLiteral("阿桃"));
    }
}

void TestCharacterStore::rejectsInvalidOrDuplicateNames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY(!store.create(QString(), &id, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.create(QStringLiteral("   "), &id, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));
    error.clear();
    QVERIFY(!store.create(QStringLiteral("小柚"), &id, &error));
    QVERIFY(error.contains(QStringLiteral("已存在")));

    // 含非法字符的名字要规范化成安全 id。
    error.clear();
    QString slashId;
    QVERIFY2(store.create(QStringLiteral("a/b"), &slashId, &error), qPrintable(error));
    QCOMPARE(slashId, QStringLiteral("a_b"));

    // 不安全的 id 不能访问已有角色。
    QVERIFY(!store.contains(QStringLiteral("a/b")));
    QVERIFY(!store.contains(QString()));
    PersonaData data;
    error.clear();
    QVERIFY(!store.card(QStringLiteral("a/b"), &data, &error));
    QVERIFY(!error.isEmpty());

    // 工具函数本身。
    QCOMPARE(CharacterStore::sanitizeId(QStringLiteral(" 小柚 ")), QStringLiteral("小柚"));
    QCOMPARE(CharacterStore::sanitizeId(QStringLiteral("x:y")), QStringLiteral("x_y"));
    QCOMPARE(CharacterStore::sanitizeId(QStringLiteral("  ")), QString());
    QCOMPARE(CharacterStore::sanitizeId(QStringLiteral("NUL")), QStringLiteral("_NUL"));
}

void TestCharacterStore::persistsAcrossInstances()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString id;
    QString error;
    {
        CharacterStore store(dir.path());
        QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));

        PersonaData card;
        card.name = QStringLiteral("小柚");
        card.body = QStringLiteral("旧正文");
        QVERIFY2(store.saveCard(id, card, &error), qPrintable(error));

        CharacterMeta meta;
        meta.scale = 1.5;
        meta.worldId = QStringLiteral("魔法学院");
        QVERIFY2(store.saveMeta(id, meta, &error), qPrintable(error));
    }

    CharacterStore reopened(dir.path());
    const QList<Character> characters = reopened.list();
    QCOMPARE(characters.size(), 1);
    QCOMPARE(characters.first().id, id);
    QCOMPARE(characters.first().name, QStringLiteral("小柚"));
    QCOMPARE(characters.first().worldId, QStringLiteral("魔法学院"));
    QCOMPARE(reopened.worldId(id), QStringLiteral("魔法学院"));

    PersonaData card;
    QVERIFY(reopened.card(id, &card));
    QCOMPARE(card.body, QStringLiteral("旧正文"));

    CharacterMeta meta;
    QVERIFY(reopened.meta(id, &meta));
    QCOMPARE(meta.scale, 1.5);
    QCOMPARE(meta.worldId, QStringLiteral("魔法学院"));
}

void TestCharacterStore::renamesCharacter()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));

    QVERIFY2(store.rename(id, QStringLiteral("大柚"), &error), qPrintable(error));
    QCOMPARE(store.list().size(), 1);
    QCOMPARE(store.list().first().name, QStringLiteral("大柚"));
    QCOMPARE(store.list().first().id, id);

    PersonaData card;
    QVERIFY(store.card(id, &card));
    QCOMPARE(card.name, QStringLiteral("大柚"));

    error.clear();
    QVERIFY(!store.rename(QStringLiteral("不存在"), QStringLiteral("名字"), &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.rename(id, QStringLiteral("   "), &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterStore::removesCharacter()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));
    QVERIFY(QDir(store.characterDir(id)).exists());

    QVERIFY2(store.remove(id, &error), qPrintable(error));
    QVERIFY(!store.contains(id));
    QVERIFY(!QDir(store.characterDir(id)).exists());
    QVERIFY(store.list().isEmpty());

    error.clear();
    QVERIFY(!store.remove(id, &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterStore::cardAndMetaRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));

    PersonaData card;
    card.name = QStringLiteral("小柚");
    card.birthday = QStringLiteral("2000-01-01");
    card.zodiac = QStringLiteral("摩羯座");
    card.hobbies = QStringLiteral("看书");
    card.favoriteFood = QStringLiteral("草莓");
    card.background = QStringLiteral("来自桌面");
    card.body = QStringLiteral("人设正文");
    QVERIFY2(store.saveCard(id, card, &error), qPrintable(error));

    PersonaData loaded;
    QVERIFY2(store.card(id, &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.id, id); // 不能变成 card.md 的 “card”
    QCOMPARE(loaded.name, card.name);
    QCOMPARE(loaded.birthday, card.birthday);
    QCOMPARE(loaded.zodiac, card.zodiac);
    QCOMPARE(loaded.hobbies, card.hobbies);
    QCOMPARE(loaded.favoriteFood, card.favoriteFood);
    QCOMPARE(loaded.background, card.background);
    QCOMPARE(loaded.body, card.body);

    CharacterMeta meta;
    meta.scale = 1.25;
    meta.anchor = QPointF(0.2, 0.8);
    meta.worldId = QStringLiteral("魔法学院");
    meta.enabled = false;
    QJsonObject head;
    head.insert(QStringLiteral("x"), 0.1);
    head.insert(QStringLiteral("y"), 0.2);
    meta.hitAreas.insert(QStringLiteral("head"), head);
    meta.motions.insert(QStringLiteral("idle"), QStringLiteral("breathe"));
    QVERIFY2(store.saveMeta(id, meta, &error), qPrintable(error));

    CharacterMeta loadedMeta;
    QVERIFY2(store.meta(id, &loadedMeta, &error), qPrintable(error));
    QCOMPARE(loadedMeta.scale, 1.25);
    QVERIFY(qFuzzyCompare(loadedMeta.anchor.x(), 0.2));
    QVERIFY(qFuzzyCompare(loadedMeta.anchor.y(), 0.8));
    QCOMPARE(loadedMeta.worldId, QStringLiteral("魔法学院"));
    QVERIFY(!loadedMeta.enabled);
    QVERIFY(qFuzzyCompare(
        loadedMeta.hitAreas.value(QStringLiteral("head")).toObject().value(QStringLiteral("x")).toDouble(), 0.1));
    QCOMPARE(loadedMeta.motions.value(QStringLiteral("idle")).toString(), QStringLiteral("breathe"));

    QCOMPARE(store.worldId(id), QStringLiteral("魔法学院"));
    QVERIFY2(store.setWorldId(id, QStringLiteral("新世界"), &error), qPrintable(error));
    QCOMPARE(store.worldId(id), QStringLiteral("新世界"));
    QCOMPARE(store.list().first().worldId, QStringLiteral("新世界"));

    // meta.json 缺失时返回默认值。
    QVERIFY(QFile::remove(store.metaPath(id)));
    CharacterMeta defaults;
    QVERIFY(store.meta(id, &defaults));
    QCOMPARE(defaults.scale, 1.0);
    QVERIFY(qFuzzyCompare(defaults.anchor.x(), 0.5));
    QVERIFY(qFuzzyCompare(defaults.anchor.y(), 1.0));
    QVERIFY(defaults.enabled);

    // 非法缩放比例要拒绝。
    CharacterMeta invalid;
    invalid.scale = 0.0;
    error.clear();
    QVERIFY(!store.saveMeta(id, invalid, &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterStore::preservesUnknownMetaFields()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));

    // 模拟其它模块往 meta.json 写自定义字段。
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("scale"), 1.0);
    root.insert(QStringLiteral("customFlag"), QStringLiteral("keep-me"));
    QVERIFY(writeBytesFile(store.metaPath(id), QJsonDocument(root).toJson()));

    CharacterMeta meta;
    QVERIFY2(store.meta(id, &meta, &error), qPrintable(error));
    meta.scale = 2.0;
    QVERIFY2(store.saveMeta(id, meta, &error), qPrintable(error));

    QFile file(store.metaPath(id));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject saved = QJsonDocument::fromJson(file.readAll()).object();
    QCOMPARE(saved.value(QStringLiteral("customFlag")).toString(), QStringLiteral("keep-me"));
    QCOMPARE(saved.value(QStringLiteral("scale")).toDouble(), 2.0);
}

void TestCharacterStore::corruptMetaFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));
    QVERIFY(writeBytesFile(store.metaPath(id), QByteArray("not-a-json")));

    CharacterMeta meta;
    error.clear();
    QVERIFY(!store.meta(id, &meta, &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterStore::importsAvatars()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString id;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &id, &error), qPrintable(error));

    const QString source = dir.path() + QStringLiteral("/pet.png");
    QVERIFY(writeBytesFile(source, QByteArray("\x89PNG\r\n\x1a\n", 8)));
    QString stored;
    QVERIFY2(store.importAvatar(id, source, &stored, &error), qPrintable(error));
    QVERIFY(QFile::exists(stored));
    QVERIFY(stored.startsWith(store.avatarsDir(id)));
    QCOMPARE(store.avatars(id).size(), 1);
    QCOMPARE(store.primaryAvatar(id), stored);
    QCOMPARE(store.list().first().avatarPath, stored);

    const QString secondSource = dir.path() + QStringLiteral("/pet2.jpg");
    QVERIFY(writeBytesFile(secondSource, QByteArray("jpeg", 4)));
    QVERIFY2(store.importAvatar(id, secondSource, &stored, &error), qPrintable(error));
    QCOMPARE(store.avatars(id).size(), 2);

    const QString textSource = dir.path() + QStringLiteral("/note.txt");
    QVERIFY(writeBytesFile(textSource, QByteArray("text")));
    error.clear();
    QVERIFY(!store.importAvatar(id, textSource, &stored, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.importAvatar(id, dir.path() + QStringLiteral("/missing.png"), &stored, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.importAvatar(QStringLiteral("不存在"), source, &stored, &error));
    QVERIFY(!error.isEmpty());
}

void TestCharacterStore::managesPersonas()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CharacterStore store(dir.path());

    QString characterId;
    QString error;
    QVERIFY2(store.create(QStringLiteral("小柚"), &characterId, &error), qPrintable(error));

    PersonaData gentle;
    gentle.name = QStringLiteral("温柔版");
    gentle.hobbies = QStringLiteral("画画");
    gentle.body = QStringLiteral("温柔地说话");
    QString personaId;
    QVERIFY2(store.savePersona(characterId, gentle, &personaId, &error), qPrintable(error));
    QCOMPARE(personaId, QStringLiteral("温柔版"));
    QVERIFY(QFile::exists(QDir(store.personasDir(characterId)).filePath(QStringLiteral("温柔版.md"))));

    PersonaData sharp;
    sharp.name = QStringLiteral("毒舌版");
    sharp.body = QStringLiteral("毒舌地说话");
    QString secondPersonaId;
    QVERIFY2(store.savePersona(characterId, sharp, &secondPersonaId, &error), qPrintable(error));
    QCOMPARE(secondPersonaId, QStringLiteral("毒舌版"));
    QCOMPARE(store.personas(characterId).size(), 2);

    PersonaData loaded;
    QVERIFY2(store.persona(characterId, personaId, &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.id, personaId);
    QCOMPARE(loaded.name, QStringLiteral("温柔版"));
    QCOMPARE(loaded.body, QStringLiteral("温柔地说话"));
    QCOMPARE(loaded.hobbies, QStringLiteral("画画"));

    // 修改正文：id 不变，文件数不变。
    loaded.body = QStringLiteral("更温柔地说话");
    QString savedId;
    QVERIFY2(store.savePersona(characterId, loaded, &savedId, &error), qPrintable(error));
    QCOMPARE(savedId, personaId);
    QCOMPARE(store.personas(characterId).size(), 2);
    PersonaData updated;
    QVERIFY(store.persona(characterId, personaId, &updated));
    QCOMPARE(updated.body, QStringLiteral("更温柔地说话"));

    // 重命名显示名。
    QVERIFY2(store.renamePersona(characterId, personaId, QStringLiteral("超温柔版"), &error), qPrintable(error));
    PersonaData renamed;
    QVERIFY(store.persona(characterId, personaId, &renamed));
    QCOMPARE(renamed.name, QStringLiteral("超温柔版"));
    QCOMPARE(renamed.id, personaId);
    QCOMPARE(store.personas(characterId).size(), 2);

    // 删除。
    QVERIFY2(store.removePersona(characterId, secondPersonaId, &error), qPrintable(error));
    QCOMPARE(store.personas(characterId).size(), 1);
    error.clear();
    QVERIFY(!store.removePersona(characterId, secondPersonaId, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.persona(characterId, QStringLiteral("不存在"), &loaded, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!store.savePersona(QStringLiteral("不存在"), gentle, &savedId, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(store.personas(QStringLiteral("不存在")).isEmpty());
}

QTEST_GUILESS_MAIN(TestCharacterStore)
