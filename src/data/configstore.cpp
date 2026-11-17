#include "data/configstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include "services/secretstore.h"

namespace
{
QString defaultConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/config.json");
}
} // namespace

ConfigStore::ConfigStore(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath.isEmpty() ? defaultConfigPath() : filePath)
{
    m_config.petPositions = QVector<QPointF>(3, QPointF(-1.0, -1.0));
    m_config.petCharacters = QVector<QString>(3, QString());
    load();
}

void ConfigStore::load()
{
    m_config = AppConfig{};
    m_config.petPositions = QVector<QPointF>(3, QPointF(-1.0, -1.0));
    m_config.petCharacters = QVector<QString>(3, QString());

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QByteArray content = file.readAll();
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(content, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        // 配置损坏：备份原文件后回退默认值，避免下次保存覆盖掉用户数据。
        file.close();
        const QString backup = m_filePath + QStringLiteral(".bak");
        QFile::remove(backup);
        QFile::copy(m_filePath, backup);
        return;
    }

    const QJsonObject root = document.object();
    m_config.preset = root.value(QStringLiteral("preset")).toString(m_config.preset);
    m_config.baseUrl = root.value(QStringLiteral("baseUrl")).toString();
    // API Key 优先读 DPAPI 密文，兼容旧版明文；下次保存时自动迁移。
    const QString protectedKey = root.value(QStringLiteral("apiKeyProtected")).toString();
    if (!protectedKey.isEmpty())
        m_config.apiKey = secrets::unprotect(protectedKey);
    else
        m_config.apiKey = root.value(QStringLiteral("apiKey")).toString();
    m_config.model = root.value(QStringLiteral("model")).toString(m_config.model);
    m_config.temperature = root.value(QStringLiteral("temperature")).toDouble(0.8);
    m_config.maxTokens = root.value(QStringLiteral("maxTokens")).toInt(0);
    m_config.personaId = root.value(QStringLiteral("personaId")).toString();
    m_config.characterId = root.value(QStringLiteral("characterId")).toString();
    m_config.idleMotion = root.value(QStringLiteral("idleMotion")).toBool(true);
    m_config.idleAmplitude = root.value(QStringLiteral("idleAmplitude")).toDouble(1.0);
    m_config.clickThrough = root.value(QStringLiteral("clickThrough")).toBool(true);
    m_config.debugHitAreas = root.value(QStringLiteral("debugHitAreas")).toBool(false);
    m_config.edgeHide = root.value(QStringLiteral("edgeHide")).toBool(true);
    m_config.autoExtractMemories = root.value(QStringLiteral("autoExtractMemories")).toBool(true);
    m_config.proactiveFrequency = root.value(QStringLiteral("proactiveFrequency")).toInt(0);
    m_config.mode = qBound(0, root.value(QStringLiteral("mode")).toInt(0), 2);
    m_config.theme = qBound(0, root.value(QStringLiteral("theme")).toInt(0), 2);
    m_config.language = qBound(0, root.value(QStringLiteral("language")).toInt(2), 2);
    m_config.autoMode = root.value(QStringLiteral("autoMode")).toBool(false);
    m_config.intimacyDecay = root.value(QStringLiteral("intimacyDecay")).toBool(false);
    m_config.modelProfiles.clear();
    const QJsonArray profiles = root.value(QStringLiteral("modelProfiles")).toArray();
    for (const QJsonValue &value : profiles) {
        const QJsonObject object = value.toObject();
        ModelProfile profile;
        profile.name = object.value(QStringLiteral("name")).toString();
        profile.preset = object.value(QStringLiteral("preset")).toString();
        profile.baseUrl = object.value(QStringLiteral("baseUrl")).toString();
        const QString protectedKey = object.value(QStringLiteral("apiKeyProtected")).toString();
        profile.apiKey = protectedKey.isEmpty() ? object.value(QStringLiteral("apiKey")).toString()
                                                : secrets::unprotect(protectedKey);
        profile.model = object.value(QStringLiteral("model")).toString();
        profile.temperature = object.value(QStringLiteral("temperature")).toDouble(0.8);
        profile.maxTokens = object.value(QStringLiteral("maxTokens")).toInt(0);
        m_config.modelProfiles.append(profile);
    }
    m_config.activeProfile = root.value(QStringLiteral("activeProfile")).toInt(-1);
    m_config.ballVisible = root.value(QStringLiteral("ballVisible")).toBool(true);
    m_config.ballX = root.value(QStringLiteral("ballX")).toDouble(-1.0);
    m_config.ballY = root.value(QStringLiteral("ballY")).toDouble(-1.0);
    m_config.onboardingDone = root.value(QStringLiteral("onboardingDone")).toBool(false);
    m_config.updateUrl = root.value(QStringLiteral("updateUrl")).toString();
    m_config.petCount = qBound(1, root.value(QStringLiteral("petCount")).toInt(1), 3);
    m_config.petPositions = QVector<QPointF>(3, QPointF(-1.0, -1.0));
    m_config.petCharacters = QVector<QString>(3, QString());
    const QJsonArray characters = root.value(QStringLiteral("petCharacters")).toArray();
    for (int i = 0; i < characters.size() && i < 3; ++i)
        m_config.petCharacters[i] = characters.at(i).toString();
    const QJsonArray pets = root.value(QStringLiteral("pets")).toArray();
    if (!pets.isEmpty()) {
        for (int i = 0; i < pets.size() && i < 3; ++i) {
            const QJsonObject pet = pets.at(i).toObject();
            m_config.petPositions[i] = QPointF(pet.value(QStringLiteral("x")).toDouble(-1.0),
                                               pet.value(QStringLiteral("y")).toDouble(-1.0));
        }
    } else {
        const double legacyX = root.value(QStringLiteral("petX")).toDouble(-1.0);
        const double legacyY = root.value(QStringLiteral("petY")).toDouble(-1.0);
        if (legacyX >= 0 && legacyY >= 0)
            m_config.petPositions[0] = QPointF(legacyX, legacyY);
    }
}

bool ConfigStore::save(QString *error)
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonObject root;
    root.insert(QStringLiteral("version"), 5);
    root.insert(QStringLiteral("preset"), m_config.preset);
    root.insert(QStringLiteral("baseUrl"), m_config.baseUrl);
    const QString protectedKey = secrets::protect(m_config.apiKey);
    if (!m_config.apiKey.isEmpty() && !protectedKey.isEmpty())
        root.insert(QStringLiteral("apiKeyProtected"), protectedKey);
    else if (!m_config.apiKey.isEmpty())
        root.insert(QStringLiteral("apiKey"), m_config.apiKey);
    root.insert(QStringLiteral("model"), m_config.model);
    root.insert(QStringLiteral("temperature"), m_config.temperature);
    root.insert(QStringLiteral("maxTokens"), m_config.maxTokens);
    root.insert(QStringLiteral("personaId"), m_config.personaId);
    root.insert(QStringLiteral("characterId"), m_config.characterId);
    root.insert(QStringLiteral("idleMotion"), m_config.idleMotion);
    root.insert(QStringLiteral("idleAmplitude"), m_config.idleAmplitude);
    root.insert(QStringLiteral("clickThrough"), m_config.clickThrough);
    root.insert(QStringLiteral("debugHitAreas"), m_config.debugHitAreas);
    root.insert(QStringLiteral("edgeHide"), m_config.edgeHide);
    root.insert(QStringLiteral("autoExtractMemories"), m_config.autoExtractMemories);
    root.insert(QStringLiteral("proactiveFrequency"), m_config.proactiveFrequency);
    root.insert(QStringLiteral("mode"), m_config.mode);
    root.insert(QStringLiteral("theme"), m_config.theme);
    root.insert(QStringLiteral("language"), m_config.language);
    root.insert(QStringLiteral("autoMode"), m_config.autoMode);
    root.insert(QStringLiteral("intimacyDecay"), m_config.intimacyDecay);
    QJsonArray profiles;
    for (const ModelProfile &profile : m_config.modelProfiles) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), profile.name);
        object.insert(QStringLiteral("preset"), profile.preset);
        object.insert(QStringLiteral("baseUrl"), profile.baseUrl);
        const QString protectedKey = secrets::protect(profile.apiKey);
        if (!protectedKey.isEmpty())
            object.insert(QStringLiteral("apiKeyProtected"), protectedKey);
        object.insert(QStringLiteral("model"), profile.model);
        object.insert(QStringLiteral("temperature"), profile.temperature);
        object.insert(QStringLiteral("maxTokens"), profile.maxTokens);
        profiles.append(object);
    }
    root.insert(QStringLiteral("modelProfiles"), profiles);
    root.insert(QStringLiteral("activeProfile"), m_config.activeProfile);
    root.insert(QStringLiteral("ballVisible"), m_config.ballVisible);
    root.insert(QStringLiteral("ballX"), m_config.ballX);
    root.insert(QStringLiteral("ballY"), m_config.ballY);
    root.insert(QStringLiteral("onboardingDone"), m_config.onboardingDone);
    root.insert(QStringLiteral("updateUrl"), m_config.updateUrl);
    root.insert(QStringLiteral("petCount"), m_config.petCount);
    QJsonArray pets;
    for (const QPointF &position : m_config.petPositions) {
        pets.append(QJsonObject{{QStringLiteral("x"), position.x()}, {QStringLiteral("y"), position.y()}});
    }
    root.insert(QStringLiteral("pets"), pets);
    QJsonArray petCharacters;
    for (const QString &characterId : m_config.petCharacters)
        petCharacters.append(characterId);
    root.insert(QStringLiteral("petCharacters"), petCharacters);

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error != nullptr)
            *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error != nullptr)
            *error = file.errorString();
        return false;
    }
    return true;
}

void ConfigStore::setCharacterId(const QString &characterId)
{
    m_config.characterId = characterId;
    emit changed();
}

void ConfigStore::setLlm(const QString &preset, const QString &baseUrl, const QString &apiKey, const QString &model)
{
    m_config.preset = preset;
    m_config.baseUrl = baseUrl;
    m_config.apiKey = apiKey;
    m_config.model = model;
    emit changed();
}

void ConfigStore::setPersonaId(const QString &personaId)
{
    m_config.personaId = personaId;
    emit changed();
}

void ConfigStore::setMemoryPrefs(bool autoExtract, int proactiveFrequency)
{
    m_config.autoExtractMemories = autoExtract;
    m_config.proactiveFrequency = proactiveFrequency;
    emit changed();
}

void ConfigStore::setPetBehavior(bool idleMotion, double idleAmplitude, bool clickThrough, bool debugHitAreas)
{
    m_config.idleMotion = idleMotion;
    m_config.idleAmplitude = idleAmplitude;
    m_config.clickThrough = clickThrough;
    m_config.debugHitAreas = debugHitAreas;
    emit changed();
}

void ConfigStore::setPetPosition(int index, double x, double y)
{
    if (index < 0 || index >= m_config.petPositions.size())
        return;
    m_config.petPositions[index] = QPointF(x, y);
    emit changed();
}

void ConfigStore::setEdgeHide(bool enabled)
{
    m_config.edgeHide = enabled;
    emit changed();
}

void ConfigStore::setMode(int mode)
{
    m_config.mode = qBound(0, mode, 2);
    emit changed();
}

void ConfigStore::setTheme(int theme)
{
    m_config.theme = qBound(0, theme, 2);
    emit changed();
}

void ConfigStore::setGeneration(double temperature, int maxTokens)
{
    m_config.temperature = temperature;
    m_config.maxTokens = maxTokens;
    emit changed();
}

void ConfigStore::setModelProfiles(const QVector<ModelProfile> &profiles, int activeIndex)
{
    m_config.modelProfiles = profiles;
    m_config.activeProfile = activeIndex;
    emit changed();
}

void ConfigStore::setIntimacyDecay(bool enabled)
{
    m_config.intimacyDecay = enabled;
    emit changed();
}

void ConfigStore::setAutoMode(bool enabled)
{
    m_config.autoMode = enabled;
    emit changed();
}

void ConfigStore::setLanguage(int language)
{
    m_config.language = qBound(0, language, 2);
    emit changed();
}

void ConfigStore::setBall(bool visible, double x, double y)
{
    m_config.ballVisible = visible;
    m_config.ballX = x;
    m_config.ballY = y;
    emit changed();
}

void ConfigStore::setOnboardingDone(bool done)
{
    m_config.onboardingDone = done;
    emit changed();
}

void ConfigStore::setUpdateUrl(const QString &url)
{
    m_config.updateUrl = url;
    emit changed();
}

void ConfigStore::setPetCharacter(int index, const QString &characterId)
{
    if (index < 0 || index >= m_config.petCharacters.size())
        return;
    m_config.petCharacters[index] = characterId;
    emit changed();
}

void ConfigStore::setPetCount(int count)
{
    m_config.petCount = qBound(1, count, 3);
    emit changed();
}
