#include "characterstore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
const char *const translationContext = "CharacterStore";

QString trText(const char *text)
{
    return QCoreApplication::translate(translationContext, text);
}

void setError(QString *error, const QString &message)
{
    if (error != nullptr)
        *error = message;
}

QString defaultDataDir()
{
    QString location = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!location.isEmpty())
        return location;
    return QDir::homePath() + QStringLiteral("/.desktop_companion");
}

bool isSafeId(const QString &id)
{
    return !id.isEmpty() && CharacterStore::sanitizeId(id) == id;
}

bool isImageFileName(const QString &fileName)
{
    static const QStringList imageSuffixes = {QStringLiteral("png"),
                                              QStringLiteral("jpg"),
                                              QStringLiteral("jpeg"),
                                              QStringLiteral("webp"),
                                              QStringLiteral("gif"),
                                              QStringLiteral("bmp"),
                                              QStringLiteral("svg")};
    return imageSuffixes.contains(QFileInfo(fileName).suffix().toLower());
}

bool writeTextFile(const QString &path, const QString &text, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, trText("无法写入文件：%1").arg(file.errorString()));
        return false;
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        setError(error, trText("写入文件失败：%1").arg(file.errorString()));
        return false;
    }
    return true;
}
} // namespace

CharacterStore::CharacterStore(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir.isEmpty() ? defaultDataDir() : dataDir)
{
}

QString CharacterStore::charactersDir() const
{
    return QDir(m_dataDir).filePath(QStringLiteral("characters"));
}

QString CharacterStore::characterDir(const QString &id) const
{
    return QDir(charactersDir()).filePath(id);
}

QString CharacterStore::cardPath(const QString &id) const
{
    return QDir(characterDir(id)).filePath(QStringLiteral("card.md"));
}

QString CharacterStore::metaPath(const QString &id) const
{
    return QDir(characterDir(id)).filePath(QStringLiteral("meta.json"));
}

QString CharacterStore::avatarsDir(const QString &id) const
{
    return QDir(characterDir(id)).filePath(QStringLiteral("avatars"));
}

QString CharacterStore::personasDir(const QString &id) const
{
    return QDir(characterDir(id)).filePath(QStringLiteral("personas"));
}

QString CharacterStore::sanitizeId(const QString &raw)
{
    return sanitizeFileId(raw);
}

bool CharacterStore::contains(const QString &id) const
{
    return isSafeId(id) && QDir(characterDir(id)).exists();
}

QList<Character> CharacterStore::list() const
{
    QList<Character> characters;
    const QDir root(charactersDir());
    const QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString id = entry.fileName();
        if (!contains(id))
            continue;

        Character character;
        character.id = id;
        PersonaData data;
        if (card(id, &data, nullptr))
            character.name = data.name;
        if (character.name.isEmpty())
            character.name = id;

        CharacterMeta characterMeta;
        if (meta(id, &characterMeta, nullptr)) {
            character.worldId = characterMeta.worldId;
            character.enabled = characterMeta.enabled;
        }
        character.avatarPath = primaryAvatar(id);
        characters.append(character);
    }
    return characters;
}

bool CharacterStore::create(const QString &name, QString *newId, QString *error)
{
    const QString displayName = name.trimmed();
    if (displayName.isEmpty()) {
        setError(error, tr("角色名不能为空"));
        return false;
    }
    const QString id = sanitizeId(displayName);
    if (id.isEmpty()) {
        setError(error, tr("角色名无法用作目录名：%1").arg(displayName));
        return false;
    }
    const QString directory = characterDir(id);
    if (QDir(directory).exists()) {
        setError(error, tr("角色已存在：%1").arg(id));
        return false;
    }
    if (!QDir().mkpath(avatarsDir(id)) || !QDir().mkpath(personasDir(id))) {
        setError(error, tr("无法创建角色目录：%1").arg(directory));
        return false;
    }

    PersonaData data;
    data.id = id;
    data.name = displayName;
    if (!writeCard(id, data, error) || !writeMetaJson(id, CharacterMeta{}, error)) {
        QDir(directory).removeRecursively();
        return false;
    }
    if (newId != nullptr)
        *newId = id;
    emit changed();
    return true;
}

bool CharacterStore::rename(const QString &id, const QString &newName, QString *error)
{
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    const QString displayName = newName.trimmed();
    if (displayName.isEmpty()) {
        setError(error, tr("角色名不能为空"));
        return false;
    }

    PersonaData data;
    if (!card(id, &data, error))
        return false;
    data.name = displayName;
    if (!writeCard(id, data, error))
        return false;
    emit changed();
    return true;
}

bool CharacterStore::remove(const QString &id, QString *error)
{
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    const QString directory = characterDir(id);
    if (!QDir(directory).removeRecursively()) {
        setError(error, tr("无法删除角色目录：%1").arg(directory));
        return false;
    }
    emit changed();
    return true;
}

bool CharacterStore::card(const QString &id, PersonaData *data, QString *error) const
{
    if (data == nullptr) {
        setError(error, tr("人设输出指针为空"));
        return false;
    }
    *data = PersonaData{};
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    const QString path = cardPath(id);
    if (!QFileInfo::exists(path)) {
        setError(error, tr("角色缺少 card.md：%1").arg(id));
        return false;
    }
    if (!parsePersonaFile(path, data, error))
        return false;
    data->id = id;
    return true;
}

bool CharacterStore::saveCard(const QString &id, const PersonaData &data, QString *error)
{
    if (!writeCard(id, data, error))
        return false;
    emit changed();
    return true;
}

bool CharacterStore::meta(const QString &id, CharacterMeta *meta, QString *error) const
{
    if (meta == nullptr) {
        setError(error, tr("元数据输出指针为空"));
        return false;
    }
    *meta = CharacterMeta{};
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }

    QFile file(metaPath(id));
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, tr("无法读取 meta.json：%1").arg(file.errorString()));
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(error, tr("meta.json 损坏：%1").arg(parseError.errorString()));
        return false;
    }
    if (!document.isObject()) {
        setError(error, tr("meta.json 损坏：顶层不是对象"));
        return false;
    }

    const QJsonObject root = document.object();
    meta->scale = root.value(QStringLiteral("scale")).toDouble(1.0);
    meta->enabled = root.value(QStringLiteral("enabled")).toBool(true);
    meta->worldId = root.value(QStringLiteral("worldId")).toString();
    const QJsonValue hitAreasValue = root.value(QStringLiteral("hitAreas"));
    if (hitAreasValue.isArray()) {
        // 兼容角色包 meta.json 的数组写法（见 HitAreaMeta），内部转成 id → 对象。
        QJsonObject converted;
        for (const QJsonValue &value : hitAreasValue.toArray()) {
            const QJsonObject area = value.toObject();
            const QString id = area.value(QStringLiteral("id")).toString();
            if (!id.isEmpty())
                converted.insert(id, area);
        }
        meta->hitAreas = converted;
    } else {
        meta->hitAreas = hitAreasValue.toObject();
    }
    meta->motions = root.value(QStringLiteral("motions")).toObject();
    const QJsonObject anchor = root.value(QStringLiteral("anchor")).toObject();
    meta->anchor =
        QPointF(anchor.value(QStringLiteral("x")).toDouble(0.5), anchor.value(QStringLiteral("y")).toDouble(1.0));
    return true;
}

bool CharacterStore::saveMeta(const QString &id, const CharacterMeta &meta, QString *error)
{
    if (!writeMetaJson(id, meta, error))
        return false;
    emit changed();
    return true;
}

bool CharacterStore::setWorldId(const QString &id, const QString &newWorldId, QString *error)
{
    CharacterMeta characterMeta;
    if (!meta(id, &characterMeta, error))
        return false;
    characterMeta.worldId = newWorldId;
    return saveMeta(id, characterMeta, error);
}

QString CharacterStore::worldId(const QString &id) const
{
    CharacterMeta characterMeta;
    if (!meta(id, &characterMeta, nullptr))
        return {};
    return characterMeta.worldId;
}

bool CharacterStore::importAvatar(const QString &id, const QString &sourcePath, QString *storedPath, QString *error)
{
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    const QFileInfo source(sourcePath);
    if (!source.isFile()) {
        setError(error, tr("形象文件不存在：%1").arg(sourcePath));
        return false;
    }
    if (!isImageFileName(source.fileName())) {
        setError(error, tr("形象文件格式不支持：%1").arg(source.fileName()));
        return false;
    }

    const QString target = QDir(avatarsDir(id)).filePath(source.fileName());
    if (source.absoluteFilePath() != QFileInfo(target).absoluteFilePath()) {
        if (!QDir().mkpath(avatarsDir(id))) {
            setError(error, tr("无法创建形象目录：%1").arg(avatarsDir(id)));
            return false;
        }
        if (QFileInfo::exists(target) && !QFile::remove(target)) {
            setError(error, tr("无法覆盖形象文件：%1").arg(target));
            return false;
        }
        if (!QFile::copy(source.absoluteFilePath(), target)) {
            setError(error, tr("复制形象文件失败：%1").arg(source.fileName()));
            return false;
        }
    }
    if (storedPath != nullptr)
        *storedPath = target;
    emit changed();
    return true;
}

QStringList CharacterStore::avatars(const QString &id) const
{
    QStringList result;
    if (!contains(id))
        return result;
    const QDir directory(avatarsDir(id));
    const QFileInfoList entries = directory.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo &entry : entries) {
        if (isImageFileName(entry.fileName()))
            result.append(entry.absoluteFilePath());
    }
    return result;
}

QString CharacterStore::primaryAvatar(const QString &id) const
{
    const QStringList files = avatars(id);
    return files.isEmpty() ? QString() : files.first();
}

QList<PersonaData> CharacterStore::personas(const QString &characterId) const
{
    QList<PersonaData> result;
    if (!contains(characterId))
        return result;

    const QDir directory(personasDir(characterId));
    const QFileInfoList entries = directory.entryInfoList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    for (const QFileInfo &entry : entries) {
        PersonaData data;
        if (!parsePersonaFile(entry.absoluteFilePath(), &data, nullptr))
            continue;
        data.id = entry.completeBaseName();
        result.append(data);
    }
    return result;
}

bool CharacterStore::persona(const QString &characterId,
                             const QString &personaId,
                             PersonaData *data,
                             QString *error) const
{
    if (data == nullptr) {
        setError(error, tr("人设输出指针为空"));
        return false;
    }
    *data = PersonaData{};
    if (!contains(characterId)) {
        setError(error, tr("角色不存在：%1").arg(characterId));
        return false;
    }
    if (!isSafeId(personaId)) {
        setError(error, tr("人设 id 无效：%1").arg(personaId));
        return false;
    }
    const QString path = QDir(personasDir(characterId)).filePath(personaId + QStringLiteral(".md"));
    if (!QFileInfo::exists(path)) {
        setError(error, tr("人设不存在：%1").arg(personaId));
        return false;
    }
    if (!parsePersonaFile(path, data, error))
        return false;
    data->id = personaId;
    return true;
}

bool CharacterStore::savePersona(const QString &characterId,
                                 const PersonaData &data,
                                 QString *personaId,
                                 QString *error)
{
    if (!contains(characterId)) {
        setError(error, tr("角色不存在：%1").arg(characterId));
        return false;
    }
    QString id = data.id.trimmed();
    if (id.isEmpty())
        id = data.name.trimmed();
    id = sanitizeId(id);
    if (id.isEmpty()) {
        setError(error, tr("人设 id 无效"));
        return false;
    }

    PersonaData copy = data;
    copy.id = id;
    if (copy.name.trimmed().isEmpty())
        copy.name = id;
    if (!QDir().mkpath(personasDir(characterId))) {
        setError(error, tr("无法创建人设目录：%1").arg(personasDir(characterId)));
        return false;
    }
    const QString path = QDir(personasDir(characterId)).filePath(id + QStringLiteral(".md"));
    if (!writeTextFile(path, renderPersonaMarkdown(copy), error))
        return false;
    if (personaId != nullptr)
        *personaId = id;
    emit changed();
    return true;
}

bool CharacterStore::renamePersona(const QString &characterId,
                                   const QString &personaId,
                                   const QString &newName,
                                   QString *error)
{
    PersonaData data;
    if (!persona(characterId, personaId, &data, error))
        return false;
    const QString displayName = newName.trimmed();
    if (displayName.isEmpty()) {
        setError(error, tr("人设名称不能为空"));
        return false;
    }
    data.name = displayName;
    return savePersona(characterId, data, nullptr, error);
}

bool CharacterStore::removePersona(const QString &characterId, const QString &personaId, QString *error)
{
    if (!contains(characterId)) {
        setError(error, tr("角色不存在：%1").arg(characterId));
        return false;
    }
    if (!isSafeId(personaId)) {
        setError(error, tr("人设 id 无效：%1").arg(personaId));
        return false;
    }
    const QString path = QDir(personasDir(characterId)).filePath(personaId + QStringLiteral(".md"));
    if (!QFileInfo::exists(path)) {
        setError(error, tr("人设不存在：%1").arg(personaId));
        return false;
    }
    if (!QFile::remove(path)) {
        setError(error, tr("无法删除人设文件：%1").arg(path));
        return false;
    }
    emit changed();
    return true;
}

bool CharacterStore::writeCard(const QString &id, const PersonaData &data, QString *error) const
{
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    PersonaData copy = data;
    copy.id = id;
    if (copy.name.trimmed().isEmpty())
        copy.name = id;
    return writeTextFile(cardPath(id), renderPersonaMarkdown(copy), error);
}

bool CharacterStore::writeMetaJson(const QString &id, const CharacterMeta &meta, QString *error) const
{
    if (!contains(id)) {
        setError(error, tr("角色不存在：%1").arg(id));
        return false;
    }
    if (meta.scale <= 0.0) {
        setError(error, tr("缩放比例无效：%1").arg(meta.scale));
        return false;
    }

    // 保留已有文件中未识别的字段，避免覆盖命中/动效模块写入的自定义元数据。
    QJsonObject root;
    {
        QFile existing(metaPath(id));
        if (existing.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError{};
            const QJsonDocument document = QJsonDocument::fromJson(existing.readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && document.isObject())
                root = document.object();
        }
    } // QSaveFile 提交前必须释放旧文件句柄，否则 Windows 下替换会报拒绝访问

    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("scale"), meta.scale);
    QJsonObject anchor;
    anchor.insert(QStringLiteral("x"), meta.anchor.x());
    anchor.insert(QStringLiteral("y"), meta.anchor.y());
    root.insert(QStringLiteral("anchor"), anchor);
    root.insert(QStringLiteral("hitAreas"), meta.hitAreas);
    root.insert(QStringLiteral("motions"), meta.motions);
    root.insert(QStringLiteral("worldId"), meta.worldId);
    root.insert(QStringLiteral("enabled"), meta.enabled);

    const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return writeTextFile(metaPath(id), json, error);
}
