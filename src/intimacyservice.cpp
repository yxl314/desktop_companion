#include "intimacyservice.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
constexpr int fileVersion = 1;

QString defaultIntimacyDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/intimacy");
}

QString nowIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

/// 角色 id → 文件名：保留字母、数字、'-'、'_'，其余替换为 '_'。
QString safeFileName(const QString &characterId)
{
    QString result;
    result.reserve(characterId.size());
    for (const QChar character : characterId) {
        const bool safe = character.isLetterOrNumber() || character == QLatin1Char('-')
                          || character == QLatin1Char('_');
        result.append(safe ? character : QLatin1Char('_'));
    }
    if (result.isEmpty())
        result = QStringLiteral("default");
    return result;
}
} // namespace

IntimacyService::IntimacyService(const QString &dirPath, QObject *parent)
    : QObject(parent)
    , m_dirPath(dirPath.isEmpty() ? defaultIntimacyDir() : dirPath)
{
}

QString IntimacyService::absoluteFilePath(const QString &characterId) const
{
    return m_dirPath + QLatin1Char('/') + safeFileName(characterId) + QStringLiteral(".json");
}

QString IntimacyService::filePath(const QString &characterId) const
{
    return absoluteFilePath(characterId);
}

IntimacyService::Level IntimacyService::levelForScore(int score)
{
    if (score >= trustedThreshold)
        return Level::Trusted;
    if (score >= closeThreshold)
        return Level::Close;
    if (score >= familiarThreshold)
        return Level::Familiar;
    return Level::Stranger;
}

QString IntimacyService::levelName(Level level)
{
    switch (level) {
    case Level::Stranger:
        return tr("陌生");
    case Level::Familiar:
        return tr("熟悉");
    case Level::Close:
        return tr("亲近");
    case Level::Trusted:
        return tr("信赖");
    }
    return tr("陌生");
}

QString IntimacyService::levelNameForScore(int score)
{
    return levelName(levelForScore(score));
}

void IntimacyService::setErrorText(const QString &text)
{
    m_lastError = text;
}

bool IntimacyService::fail(const QString &message, QString *error)
{
    setErrorText(message);
    if (error != nullptr)
        *error = message;
    return false;
}

void IntimacyService::resetToDefault()
{
    m_characterId.clear();
    m_score = 0;
    m_decayEnabled = false;
    m_lastDecayDate = QDate();
    m_updatedAt.clear();
    m_events.clear();
}

bool IntimacyService::backupCorruptFile(const QString &path)
{
    const QString backupPath = path + QStringLiteral(".bak");
    QFile::remove(backupPath);
    if (QFile::rename(path, backupPath))
        return true;
    return QFile::copy(path, backupPath);
}

void IntimacyService::appendEvent(int appliedDelta, const QString &reason)
{
    IntimacyEvent event;
    event.at = nowIso();
    event.delta = appliedDelta;
    event.score = m_score;
    event.reason = reason;
    m_events.append(event);
    while (m_events.size() > maxEvents)
        m_events.removeFirst();
}

bool IntimacyService::load(const QString &characterId, QString *error)
{
    if (error != nullptr)
        error->clear();

    const QString id = characterId.trimmed();
    if (id.isEmpty())
        return fail(QStringLiteral("角色 id 不能为空"), error);

    resetToDefault();
    m_characterId = id;
    m_lastDecayDate = QDate::currentDate();

    const QString path = absoluteFilePath(id);
    QFile file(path);
    if (!file.exists()) {
        m_lastError.clear();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("读取好感度文件失败：%1").arg(file.errorString()), error);
    }
    const QByteArray payload = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const bool backedUp = backupCorruptFile(path);
        resetToDefault();
        m_characterId = id;
        m_lastDecayDate = QDate::currentDate();
        if (!backedUp) {
            return fail(QStringLiteral("好感度文件损坏且备份失败，已回退默认值"), error);
        }
        return fail(QStringLiteral("好感度文件损坏，已备份为 %1").arg(path + QStringLiteral(".bak")), error);
    }

    QString parseMessage;
    if (!fromJson(document.object(), &parseMessage)) {
        const bool backedUp = backupCorruptFile(path);
        resetToDefault();
        m_characterId = id;
        m_lastDecayDate = QDate::currentDate();
        if (!backedUp) {
            return fail(QStringLiteral("好感度文件损坏（%1）且备份失败，已回退默认值").arg(parseMessage), error);
        }
        return fail(QStringLiteral("好感度文件损坏（%1），已备份为 %2")
                        .arg(parseMessage, path + QStringLiteral(".bak")),
                    error);
    }

    m_characterId = id;
    if (!m_lastDecayDate.isValid())
        m_lastDecayDate = QDate::currentDate();
    emit changed();
    return true;
}

bool IntimacyService::save(QString *error)
{
    if (error != nullptr)
        error->clear();
    if (m_characterId.isEmpty())
        return fail(QStringLiteral("尚未加载角色好感度"), error);

    if (!QDir().mkpath(m_dirPath))
        return fail(QStringLiteral("创建好感度目录失败：%1").arg(m_dirPath), error);

    QSaveFile file(absoluteFilePath(m_characterId));
    if (!file.open(QIODevice::WriteOnly))
        return fail(QStringLiteral("写入好感度文件失败：%1").arg(file.errorString()), error);

    m_updatedAt = nowIso();
    file.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    if (!file.commit())
        return fail(QStringLiteral("保存好感度文件失败：%1").arg(file.errorString()), error);
    return true;
}

bool IntimacyService::add(int delta, const QString &reason, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (m_characterId.isEmpty())
        return fail(QStringLiteral("尚未加载角色好感度"), error);
    if (delta == 0)
        return true;

    const int previous = m_score;
    m_score = qBound(minScore, m_score + delta, maxScore);
    const int applied = m_score - previous;
    if (applied == 0)
        return true;

    appendEvent(applied, reason);
    m_updatedAt = nowIso();
    if (!save(error))
        return false;
    emit changed();
    return true;
}

bool IntimacyService::setDecayEnabled(bool enabled, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (m_characterId.isEmpty())
        return fail(QStringLiteral("尚未加载角色好感度"), error);
    if (m_decayEnabled == enabled) {
        if (enabled && !m_lastDecayDate.isValid())
            m_lastDecayDate = QDate::currentDate();
        return true;
    }

    m_decayEnabled = enabled;
    if (enabled && !m_lastDecayDate.isValid())
        m_lastDecayDate = QDate::currentDate();
    if (!save(error))
        return false;
    emit changed();
    return true;
}

bool IntimacyService::applyDailyDecay(int pointsPerDay, const QDate &today, QString *error)
{
    if (error != nullptr)
        error->clear();
    if (m_characterId.isEmpty())
        return fail(QStringLiteral("尚未加载角色好感度"), error);
    if (!m_decayEnabled)
        return true;
    if (!today.isValid())
        return fail(QStringLiteral("衰减日期无效"), error);

    if (!m_lastDecayDate.isValid()) {
        m_lastDecayDate = today;
        return save(error);
    }
    const qint64 days = m_lastDecayDate.daysTo(today);
    if (days <= 0)
        return true;

    const int perDay = qMax(0, pointsPerDay);
    const qint64 deduction = qMin<qint64>(days * static_cast<qint64>(perDay), maxScore);
    const int previous = m_score;
    m_score = qMax(minScore, m_score - static_cast<int>(deduction));
    const int applied = m_score - previous;
    m_lastDecayDate = today;
    if (applied != 0) {
        appendEvent(applied, QStringLiteral("跨天衰减（%1 天）").arg(days));
        m_updatedAt = nowIso();
    }
    if (!save(error))
        return false;
    if (applied != 0)
        emit changed();
    return true;
}

QJsonObject IntimacyService::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), fileVersion);
    object.insert(QStringLiteral("characterId"), m_characterId);
    object.insert(QStringLiteral("score"), m_score);
    object.insert(QStringLiteral("decayEnabled"), m_decayEnabled);
    if (m_lastDecayDate.isValid())
        object.insert(QStringLiteral("lastDecayDate"), m_lastDecayDate.toString(Qt::ISODate));
    object.insert(QStringLiteral("updatedAt"), m_updatedAt.isEmpty() ? nowIso() : m_updatedAt);

    QJsonArray events;
    for (const IntimacyEvent &event : m_events) {
        events.append(QJsonObject{{QStringLiteral("at"), event.at},
                                  {QStringLiteral("delta"), event.delta},
                                  {QStringLiteral("score"), event.score},
                                  {QStringLiteral("reason"), event.reason}});
    }
    object.insert(QStringLiteral("events"), events);
    return object;
}

bool IntimacyService::fromJson(const QJsonObject &object, QString *error)
{
    if (error != nullptr)
        error->clear();

    const int version = object.value(QStringLiteral("version")).toInt(fileVersion);
    if (version > fileVersion)
        return fail(QStringLiteral("好感度文件版本 %1 高于程序支持的 %2").arg(version).arg(fileVersion), error);

    const QJsonValue scoreValue = object.value(QStringLiteral("score"));
    if (!scoreValue.isUndefined() && !scoreValue.isDouble())
        return fail(QStringLiteral("score 字段不是数字"), error);

    const QJsonValue decayValue = object.value(QStringLiteral("decayEnabled"));
    if (!decayValue.isUndefined() && !decayValue.isBool())
        return fail(QStringLiteral("decayEnabled 字段不是布尔值"), error);

    QDate lastDecayDate;
    const QString lastDecayText = object.value(QStringLiteral("lastDecayDate")).toString();
    if (!lastDecayText.isEmpty()) {
        lastDecayDate = QDate::fromString(lastDecayText, Qt::ISODate);
        if (!lastDecayDate.isValid())
            return fail(QStringLiteral("lastDecayDate 字段不是合法日期"), error);
    }

    const QJsonValue eventsValue = object.value(QStringLiteral("events"));
    if (!eventsValue.isUndefined() && !eventsValue.isArray())
        return fail(QStringLiteral("events 字段不是数组"), error);

    QVector<IntimacyEvent> events;
    const QJsonArray eventArray = eventsValue.toArray();
    for (const QJsonValue value : eventArray) {
        if (!value.isObject())
            return fail(QStringLiteral("events 条目不是对象"), error);
        const QJsonObject item = value.toObject();
        IntimacyEvent event;
        event.at = item.value(QStringLiteral("at")).toString();
        event.delta = item.value(QStringLiteral("delta")).toInt();
        event.score = item.value(QStringLiteral("score")).toInt();
        event.reason = item.value(QStringLiteral("reason")).toString();
        events.append(event);
    }
    while (events.size() > maxEvents)
        events.removeFirst();

    // 校验全部通过后再提交，失败时保持原状态不变。
    m_characterId = object.value(QStringLiteral("characterId")).toString(m_characterId);
    m_score = qBound(minScore, scoreValue.toInt(minScore), maxScore);
    m_decayEnabled = decayValue.toBool(false);
    m_lastDecayDate = lastDecayDate;
    m_updatedAt = object.value(QStringLiteral("updatedAt")).toString();
    m_events = events;
    return true;
}
