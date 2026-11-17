/**
 * \file src/viewmodels/charactersviewmodel.cpp
 * \brief CharactersViewModel 实现：角色增删改查、角色包导入导出与桌宠绑定。
 */

#include "charactersviewmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>

#include "data/characterstore.h"
#include "data/configstore.h"
#include "data/worldstore.h"
#include "dialogservice.h"
#include "services/characterpack.h"
#include "services/errorcodes.h"

CharactersViewModel::CharactersViewModel(CharacterStore *characters, WorldStore *worlds,
                                         ConfigStore *config, DialogService *dialogs,
                                         QObject *parent)
    : ViewModelBase(parent)
    , m_characters(characters)
    , m_worlds(worlds)
    , m_config(config)
    , m_dialogs(dialogs)
{
    Q_ASSERT(m_characters != nullptr);
    Q_ASSERT(m_worlds != nullptr);
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_dialogs != nullptr);

    connect(m_characters, &CharacterStore::changed, this, &CharactersViewModel::refresh);
    connect(m_worlds, &WorldStore::changed, this, &CharactersViewModel::refresh);
    connect(m_config, &ConfigStore::changed, this, &CharactersViewModel::rebuild);
    rebuild();
}

CharactersViewModel::~CharactersViewModel() = default;

QVariantList CharactersViewModel::characters() const
{
    return m_charactersCache;
}

QString CharactersViewModel::emptyHint() const
{
    return m_emptyHint;
}

void CharactersViewModel::create(const QString &name)
{
    QString newId;
    QString error;
    if (!m_characters->create(name, &newId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    m_config->setCharacterId(newId);
    m_config->setPersonaId(QString());
    QString saveError;
    if (!m_config->save(&saveError)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
        return;
    }
    setStatus(tr("已创建角色：%1").arg(name));
}

void CharactersViewModel::importAvatar()
{
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请先选择角色"));
        return;
    }
    const QString path = m_dialogs->openFile(tr("选择角色形象"),
                                             tr("图片 (*.png *.jpg *.jpeg *.webp *.gif *.bmp)"));
    if (path.isEmpty())
        return;

    setBusy(true);
    auto *watcher = new QFutureWatcher<AvatarImportResult>(this);
    connect(watcher, &QFutureWatcher<AvatarImportResult>::finished, this, [this, watcher] {
        const AvatarImportResult result = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        if (!result.ok) {
            reportError(static_cast<quint32>(ErrorCode::InvalidImage), result.error);
            return;
        }
        setStatus(tr("已更新角色形象"));
    });
    watcher->setFuture(m_characters->importAvatarAsync(characterId, path));
}

void CharactersViewModel::importPack()
{
    const QString path = m_dialogs->openFile(tr("导入角色包"), tr("Zip (*.zip)"));
    if (path.isEmpty())
        return;

    setBusy(true);
    auto *watcher = new QFutureWatcher<characterpack::AsyncImportResult>(this);
    connect(watcher, &QFutureWatcher<characterpack::AsyncImportResult>::finished, this,
            [this, watcher] {
                const characterpack::AsyncImportResult result = watcher->result();
                watcher->deleteLater();
                setBusy(false);
                if (!result.ok) {
                    reportError(static_cast<quint32>(ErrorCode::IncompleteCharacterPack),
                                result.error);
                    return;
                }
                setStatus(tr("已导入角色：%1").arg(result.characterId));
            });
    watcher->setFuture(characterpack::importPackAsync(path, m_characters->charactersDir()));
}

void CharactersViewModel::exportPack(const QString &id)
{
    const QString characterId = id.isEmpty() ? m_config->values().characterId : id;
    if (characterId.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请先选择角色"));
        return;
    }
    const QString defaultName = characterId + QStringLiteral("-pack.zip");
    const QString path = m_dialogs->saveFile(tr("导出角色包"), tr("Zip (*.zip)"),
                                             QDir::homePath() + QLatin1Char('/') + defaultName);
    if (path.isEmpty())
        return;

    setBusy(true);
    auto *watcher = new QFutureWatcher<characterpack::AsyncExportResult>(this);
    connect(watcher, &QFutureWatcher<characterpack::AsyncExportResult>::finished, this,
            [this, watcher, path] {
                const characterpack::AsyncExportResult result = watcher->result();
                watcher->deleteLater();
                setBusy(false);
                if (!result.ok) {
                    reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), result.error);
                    return;
                }
                setStatus(tr("角色包已导出：%1").arg(path));
            });
    watcher->setFuture(characterpack::exportPackAsync(m_characters->characterDir(characterId), path));
}

void CharactersViewModel::rename(const QString &id, const QString &name)
{
    QString error;
    if (!m_characters->rename(id, name, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("已重命名角色"));
}

void CharactersViewModel::remove(const QString &id)
{
    QString error;
    if (!m_characters->remove(id, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    if (m_config->values().characterId == id) {
        m_config->setCharacterId(QString());
        m_config->setPersonaId(QString());
        QString saveError;
        if (!m_config->save(&saveError))
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
    }
    setStatus(tr("已删除角色"));
}

void CharactersViewModel::select(const QString &id)
{
    if (!m_characters->contains(id)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("角色不存在：%1").arg(id));
        return;
    }
    m_config->setCharacterId(id);
    m_config->setPersonaId(QString());
    QString error;
    if (!m_config->save(&error))
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
}

void CharactersViewModel::setEnabled(const QString &id, bool enabled)
{
    CharacterMeta meta;
    QString error;
    if (!m_characters->meta(id, &meta, &error)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), error);
        return;
    }
    meta.enabled = enabled;
    if (!m_characters->saveMeta(id, meta, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
}

void CharactersViewModel::launchAsPet(const QString &id)
{
    if (!m_characters->contains(id)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("角色不存在：%1").arg(id));
        return;
    }
    m_config->setPetCharacter(0, id);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("已设为桌宠角色：%1").arg(id));
    emit launchPetRequested(id);
}

void CharactersViewModel::refresh()
{
    rebuild();
}

void CharactersViewModel::rebuild()
{
    QVariantList rows;
    const QString currentId = m_config->values().characterId;

    QHash<QString, QString> worldNameById;
    for (const World &world : m_worlds->list())
        worldNameById.insert(world.id, world.name);

    QStringList petCharacters;
    for (const QString &id : m_config->values().petCharacters)
        petCharacters.append(id);

    for (const Character &character : m_characters->list()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), character.id);
        item.insert(QStringLiteral("name"), character.name);
        item.insert(QStringLiteral("avatar"), character.avatarPath);
        item.insert(QStringLiteral("worldId"), character.worldId);
        item.insert(QStringLiteral("worldName"), worldNameById.value(character.worldId));
        item.insert(QStringLiteral("enabled"), character.enabled);
        item.insert(QStringLiteral("current"), character.id == currentId);
        item.insert(QStringLiteral("isPet"), petCharacters.contains(character.id));
        rows.append(item);
    }
    m_charactersCache = rows;
    emit charactersChanged();

    const QString hint = rows.isEmpty() ? tr("还没有角色，点击「新建角色」开始。") : QString();
    if (m_emptyHint != hint) {
        m_emptyHint = hint;
        emit emptyHintChanged();
    }
}
