#include "configstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include "secretstore.h"

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
    load();
}

void ConfigStore::load()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return;

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
    m_config.personaId = root.value(QStringLiteral("personaId")).toString();
    m_config.characterId = root.value(QStringLiteral("characterId")).toString();
    m_config.idleMotion = root.value(QStringLiteral("idleMotion")).toBool(true);
    m_config.idleAmplitude = root.value(QStringLiteral("idleAmplitude")).toDouble(1.0);
    m_config.clickThrough = root.value(QStringLiteral("clickThrough")).toBool(true);
    m_config.debugHitAreas = root.value(QStringLiteral("debugHitAreas")).toBool(false);
    m_config.autoExtractMemories = root.value(QStringLiteral("autoExtractMemories")).toBool(true);
    m_config.proactiveFrequency = root.value(QStringLiteral("proactiveFrequency")).toInt(0);
    m_config.petX = root.value(QStringLiteral("petX")).toDouble(-1.0);
    m_config.petY = root.value(QStringLiteral("petY")).toDouble(-1.0);
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
    root.insert(QStringLiteral("personaId"), m_config.personaId);
    root.insert(QStringLiteral("characterId"), m_config.characterId);
    root.insert(QStringLiteral("idleMotion"), m_config.idleMotion);
    root.insert(QStringLiteral("idleAmplitude"), m_config.idleAmplitude);
    root.insert(QStringLiteral("clickThrough"), m_config.clickThrough);
    root.insert(QStringLiteral("debugHitAreas"), m_config.debugHitAreas);
    root.insert(QStringLiteral("autoExtractMemories"), m_config.autoExtractMemories);
    root.insert(QStringLiteral("proactiveFrequency"), m_config.proactiveFrequency);
    root.insert(QStringLiteral("petX"), m_config.petX);
    root.insert(QStringLiteral("petY"), m_config.petY);

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

void ConfigStore::setPetPosition(double x, double y)
{
    m_config.petX = x;
    m_config.petY = y;
    emit changed();
}
