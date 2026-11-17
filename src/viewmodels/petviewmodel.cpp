/**
 * \file src/viewmodels/petviewmodel.cpp
 * \brief PetViewModel 实现：桌宠布局、点击命中区台词、气泡与好感度。
 */

#include "petviewmodel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QUrl>

#include "data/characterstore.h"
#include "data/configstore.h"
#include "data/intimacyrepository.h"
#include "platform/hitareameta.h"
#include "services/errorcodes.h"

namespace
{
/// 气泡自动隐藏时间（毫秒）。
constexpr int bubbleLifetimeMs = 4200;
/// 桌宠默认落点（未保存位置时）。
constexpr double defaultX = 0.0;
constexpr double defaultY = 0.0;

/// 默认点击台词（按区域）。
QStringList headLines()
{
    return {
        QStringLiteral("别摸头啦～"),
        QStringLiteral("头好痒…"),
        QStringLiteral("摸头会长高的！"),
    };
}

QStringList bodyLines()
{
    return {
        QStringLiteral("嗯？"),
        QStringLiteral("摸摸头～"),
        QStringLiteral("今天也要加油呀！"),
        QStringLiteral("要不要聊聊天？"),
        QStringLiteral("别戳啦，痒～"),
        QStringLiteral("我在呢。"),
    };
}

QStringList feetLines()
{
    return {
        QStringLiteral("别戳脚，站不稳啦！"),
        QStringLiteral("要摔倒了——"),
        QStringLiteral("脚底痒痒的～"),
    };
}

/// CharacterMeta::hitAreas 以 id→定义的对象保存，转成 HitAreaMeta 需要的数组。
QJsonArray hitAreasToArray(const QJsonObject &map)
{
    QJsonArray array;
    for (const QJsonValue &value : map)
        array.append(value);
    return array;
}
} // namespace

PetViewModel::PetViewModel(ConfigStore *config, CharacterStore *characters,
                           IntimacyService *intimacy, QObject *parent)
    : ViewModelBase(parent)
    , m_config(config)
    , m_characters(characters)
    , m_intimacy(intimacy)
{
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_characters != nullptr);
    Q_ASSERT(m_intimacy != nullptr);

    m_bubbleHideTimer.setSingleShot(true);
    connect(&m_bubbleHideTimer, &QTimer::timeout, this, &PetViewModel::hideBubble);

    connect(m_config, &ConfigStore::changed, this, [this] {
        syncFromConfig();
        rebuild();
    });
    connect(m_characters, &CharacterStore::changed, this, &PetViewModel::rebuild);

    syncFromConfig();
    rebuild();
}

PetViewModel::~PetViewModel() = default;

QVariantList PetViewModel::pets() const
{
    return m_pets;
}

int PetViewModel::petCount() const
{
    return m_petCount;
}

int PetViewModel::activePetIndex() const
{
    return m_activePetIndex;
}

bool PetViewModel::idleMotion() const
{
    return m_idleMotion;
}

double PetViewModel::idleAmplitude() const
{
    return m_idleAmplitude;
}

bool PetViewModel::clickThrough() const
{
    return m_clickThrough;
}

bool PetViewModel::edgeHide() const
{
    return m_edgeHide;
}

bool PetViewModel::debugHitAreas() const
{
    return m_debugHitAreas;
}

QString PetViewModel::bubbleText() const
{
    return m_bubbleText;
}

bool PetViewModel::bubbleVisible() const
{
    return m_bubbleVisible;
}

void PetViewModel::movePet(int index, double x, double y)
{
    if (index < 0 || index >= qBound(1, m_config->values().petCount, 3))
        return;
    m_config->setPetPosition(index, x, y);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    rebuild();
}

void PetViewModel::tapPet(int index, double x, double y)
{
    if (index < 0 || index >= m_petCount)
        return;

    setActivePetIndex(index);

    const QString boundCharacter = petCharacterId(index);
    const QString currentCharacter = m_config->values().characterId;
    if (!boundCharacter.isEmpty() && boundCharacter != currentCharacter)
        emit characterActivated(boundCharacter);

    const QString characterId = boundCharacter.isEmpty() ? currentCharacter : boundCharacter;

    QStringList lines;
    const QString areaId = [&]() -> QString {
        if (characterId.isEmpty() || !m_characters->contains(characterId))
            return QString();
        CharacterMeta meta;
        QString error;
        if (!m_characters->meta(characterId, &meta, &error) || meta.hitAreas.isEmpty())
            return QString();
        HitAreaMeta hitAreaMeta;
        const QJsonObject root{{QStringLiteral("hitAreas"), hitAreasToArray(meta.hitAreas)}};
        if (!hitAreaMeta.loadFromJson(QJsonDocument(root).toJson(), &error))
            return QString();
        const QString id = hitAreaMeta.hitAreaAt(QPointF(x, y));
        if (const HitArea *area = hitAreaMeta.find(id))
            lines = area->lines;
        return id;
    }();
    Q_UNUSED(areaId);

    if (lines.isEmpty()) {
        if (y < 0.32)
            lines = headLines();
        else if (y > 0.72)
            lines = feetLines();
        else
            lines = bodyLines();
    }

    if (m_intimacy->loaded()) {
        const int score = m_intimacy->score();
        const int trusted = static_cast<int>(IntimacyService::trustedThreshold);
        const int close = static_cast<int>(IntimacyService::closeThreshold);
        if (score >= trusted)
            lines.append(QStringLiteral("最喜欢你了！"));
        else if (score >= close)
            lines.append(QStringLiteral("今天也一起玩吧～"));
        m_intimacy->add(1, QStringLiteral("点击"));
    }

    const QString text = lines.at(QRandomGenerator::global()->bounded(lines.size()));
    showBubble(text);
}

void PetViewModel::doubleTapPet(int index)
{
    if (index < 0 || index >= m_petCount)
        return;
    emit doubleTapRequested(index);
}

void PetViewModel::contextMenu(int index)
{
    if (index < 0 || index >= m_petCount)
        return;
    emit contextMenuRequested(index);
}

void PetViewModel::dragFinished(int index)
{
    if (index < 0 || index >= m_petCount)
        return;
    QString error;
    if (!m_config->save(&error))
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
    rebuild();
}

QVariantList PetViewModel::hitAreaRects(int index) const
{
    QVariantList rows;
    QString characterId = petCharacterId(index);
    if (characterId.isEmpty())
        characterId = m_config->values().characterId;
    if (characterId.isEmpty() || !m_characters->contains(characterId))
        return rows;

    CharacterMeta meta;
    QString error;
    if (!m_characters->meta(characterId, &meta, &error) || meta.hitAreas.isEmpty())
        return rows;

    for (auto it = meta.hitAreas.constBegin(); it != meta.hitAreas.constEnd(); ++it) {
        const QJsonObject area = it.value().toObject();
        const QJsonValue rectValue = area.value(QStringLiteral("rect"));
        double x = 0.0;
        double y = 0.0;
        double w = 0.0;
        double h = 0.0;
        bool valid = false;
        if (rectValue.isArray()) {
            const QJsonArray rect = rectValue.toArray();
            if (rect.size() >= 4) {
                x = rect.at(0).toDouble();
                y = rect.at(1).toDouble();
                w = rect.at(2).toDouble();
                h = rect.at(3).toDouble();
                valid = true;
            }
        } else if (rectValue.isObject()) {
            const QJsonObject rect = rectValue.toObject();
            x = rect.value(QStringLiteral("x")).toDouble();
            y = rect.value(QStringLiteral("y")).toDouble();
            w = rect.value(QStringLiteral("width")).toDouble();
            h = rect.value(QStringLiteral("height")).toDouble();
            valid = true;
        }
        if (!valid)
            continue;

        QString id = area.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            id = it.key();
        QVariantMap item;
        item.insert(QStringLiteral("id"), id);
        item.insert(QStringLiteral("x"), x);
        item.insert(QStringLiteral("y"), y);
        item.insert(QStringLiteral("w"), w);
        item.insert(QStringLiteral("h"), h);
        rows.append(item);
    }
    return rows;
}

void PetViewModel::showBubble(const QString &text)
{
    if (m_bubbleText != text) {
        m_bubbleText = text;
        emit bubbleTextChanged();
    }
    if (!m_bubbleVisible) {
        m_bubbleVisible = true;
        emit bubbleVisibleChanged();
    }
    m_bubbleHideTimer.start(bubbleLifetimeMs);
}

void PetViewModel::hideBubble()
{
    m_bubbleHideTimer.stop();
    if (!m_bubbleVisible)
        return;
    m_bubbleVisible = false;
    emit bubbleVisibleChanged();
}

void PetViewModel::rebuild()
{
    const AppConfig &values = m_config->values();
    const int count = qBound(1, values.petCount, 3);

    QStringList boundCharacters;
    for (const QString &id : values.petCharacters)
        boundCharacters.append(id);

    QVariantList rows;
    for (int i = 0; i < count; ++i) {
        QString characterId;
        if (i < boundCharacters.size() && !boundCharacters.at(i).isEmpty())
            characterId = boundCharacters.at(i);
        if (characterId.isEmpty())
            characterId = values.characterId;

        QString name;
        QString avatar = QStringLiteral("qrc:/pet.png");
        double scale = 1.0;
        if (!characterId.isEmpty() && m_characters->contains(characterId)) {
            for (const Character &character : m_characters->list()) {
                if (character.id == characterId) {
                    name = character.name;
                    break;
                }
            }
            const QString custom = m_characters->primaryAvatar(characterId);
            if (!custom.isEmpty())
                avatar = QUrl::fromLocalFile(custom).toString();
            CharacterMeta meta;
            QString error;
            if (m_characters->meta(characterId, &meta, &error) && meta.scale > 0.0)
                scale = meta.scale;
        }

        QPointF position(defaultX + i * 90.0, defaultY + i * 40.0);
        if (i < values.petPositions.size()) {
            const QPointF saved = values.petPositions.at(i);
            if (saved.x() >= 0.0 && saved.y() >= 0.0)
                position = saved;
        }

        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("characterId"), characterId);
        item.insert(QStringLiteral("name"), name);
        item.insert(QStringLiteral("avatar"), avatar);
        item.insert(QStringLiteral("x"), position.x());
        item.insert(QStringLiteral("y"), position.y());
        item.insert(QStringLiteral("visible"), true);
        item.insert(QStringLiteral("scale"), scale);
        rows.append(item);
    }

    if (m_pets != rows) {
        m_pets = rows;
        emit petsChanged();
    }

    if (m_petCount != count) {
        m_petCount = count;
        emit petCountChanged();
    }
    if (m_activePetIndex >= count) {
        m_activePetIndex = count - 1;
        emit activePetIndexChanged();
    }
}

void PetViewModel::syncFromConfig()
{
    const AppConfig &values = m_config->values();

    if (m_idleMotion != values.idleMotion) {
        m_idleMotion = values.idleMotion;
        emit idleMotionChanged();
    }
    if (!qFuzzyCompare(m_idleAmplitude, values.idleAmplitude)) {
        m_idleAmplitude = values.idleAmplitude;
        emit idleAmplitudeChanged();
    }
    if (m_clickThrough != values.clickThrough) {
        m_clickThrough = values.clickThrough;
        emit clickThroughChanged();
    }
    if (m_edgeHide != values.edgeHide) {
        m_edgeHide = values.edgeHide;
        emit edgeHideChanged();
    }
    if (m_debugHitAreas != values.debugHitAreas) {
        m_debugHitAreas = values.debugHitAreas;
        emit debugHitAreasChanged();
    }
}

QString PetViewModel::petCharacterId(int index) const
{
    const QVector<QString> characters = m_config->values().petCharacters;
    if (index >= 0 && index < characters.size())
        return characters.at(index);
    return QString();
}

void PetViewModel::setActivePetIndex(int index)
{
    if (m_activePetIndex == index)
        return;
    m_activePetIndex = index;
    emit activePetIndexChanged();
}
