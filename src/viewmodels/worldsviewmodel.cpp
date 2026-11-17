/**
 * \file src/viewmodels/worldsviewmodel.cpp
 * \brief WorldsViewModel 实现：世界观列表、编辑保存、导入与删除。
 */

#include "worldsviewmodel.h"

#include <QFutureWatcher>
#include <QHash>

#include "data/characterstore.h"
#include "data/configstore.h"
#include "data/worldstore.h"
#include "dialogservice.h"
#include "services/errorcodes.h"

namespace
{
/// 列表中的正文预览长度。
constexpr int bodyPreviewLength = 80;
} // namespace

WorldsViewModel::WorldsViewModel(WorldStore *worlds, CharacterStore *characters,
                                 DialogService *dialogs, QObject *parent)
    : WorldsViewModel(worlds, characters, dialogs, static_cast<ConfigStore *>(nullptr), parent)
{
}

WorldsViewModel::WorldsViewModel(WorldStore *worlds, CharacterStore *characters,
                                 DialogService *dialogs, ConfigStore *config, QObject *parent)
    : ViewModelBase(parent)
    , m_worlds(worlds)
    , m_characters(characters)
    , m_dialogs(dialogs)
    , m_config(config)
{
    Q_ASSERT(m_worlds != nullptr);
    Q_ASSERT(m_characters != nullptr);
    Q_ASSERT(m_dialogs != nullptr);

    connect(m_worlds, &WorldStore::changed, this, &WorldsViewModel::rebuild);
    connect(m_characters, &CharacterStore::changed, this, &WorldsViewModel::rebuild);
    rebuild();
}

WorldsViewModel::~WorldsViewModel() = default;

QVariantList WorldsViewModel::worlds() const
{
    return m_worldsCache;
}

QString WorldsViewModel::editId() const
{
    return m_editId;
}

QString WorldsViewModel::editName() const
{
    return m_editName;
}

QString WorldsViewModel::editBody() const
{
    return m_editBody;
}

bool WorldsViewModel::editing() const
{
    return m_editing;
}

bool WorldsViewModel::preview() const
{
    return m_preview;
}

void WorldsViewModel::createNew()
{
    m_editId.clear();
    setEditName(QString());
    setEditBody(QString());
    m_preview = false;
    emit previewChanged();
    m_editing = true;
    emit editIdChanged();
    emit editingChanged();
    setStatus(tr("新建世界观"));
}

void WorldsViewModel::loadFor(const QString &id)
{
    World world;
    QString error;
    if (!m_worlds->read(id, &world, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileReadFailed), error);
        return;
    }
    m_editId = id;
    setEditName(world.name);
    setEditBody(world.content);
    emit editIdChanged();
    m_editing = true;
    emit editingChanged();
    m_preview = false;
    emit previewChanged();
}

void WorldsViewModel::save()
{
    if (m_editName.trimmed().isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("世界观名称不能为空"));
        return;
    }

    QString error;
    if (m_editId.isEmpty()) {
        QString newId;
        if (!m_worlds->create(m_editName.trimmed(), &newId, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        m_editId = newId;
        emit editIdChanged();
    }

    World world;
    world.id = m_editId;
    world.name = m_editName.trimmed();
    world.content = m_editBody;
    if (!m_worlds->write(m_editId, world, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }

    m_editing = false;
    emit editingChanged();
    setStatus(tr("世界观已保存"));
}

void WorldsViewModel::importFile()
{
    const QString path = m_dialogs->openFile(tr("导入世界观（Markdown）"), tr("Markdown (*.md)"));
    if (path.isEmpty())
        return;

    setBusy(true);
    auto *watcher = new QFutureWatcher<WorldImportResult>(this);
    connect(watcher, &QFutureWatcher<WorldImportResult>::finished, this, [this, watcher] {
        const WorldImportResult result = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        if (!result.ok) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), result.error);
            return;
        }
        setStatus(tr("已导入世界观"));
    });
    watcher->setFuture(m_worlds->importFromAsync(path));
}

void WorldsViewModel::assignToCurrent(const QString &worldId)
{
    if (!worldId.isEmpty() && !m_worlds->contains(worldId)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState),
                    tr("世界观不存在：%1").arg(worldId));
        return;
    }
    if (m_config == nullptr) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState),
                    tr("未注入配置仓库，无法确定当前角色"));
        return;
    }
    const QString characterId = m_config->values().characterId;
    if (characterId.isEmpty() || !m_characters->contains(characterId)) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请先选择角色"));
        return;
    }
    QString error;
    if (!m_characters->setWorldId(characterId, worldId, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    rebuild();
    setStatus(worldId.isEmpty() ? tr("已取消当前角色的世界观归属")
                                : tr("已把当前角色归属到该世界观"));
}

void WorldsViewModel::rename(const QString &id, const QString &name)
{
    QString error;
    if (!m_worlds->rename(id, name, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    if (m_editId == id)
        setEditName(name);
    setStatus(tr("世界观已重命名"));
}

void WorldsViewModel::remove(const QString &id)
{
    QString error;
    if (!m_worlds->remove(id, &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    if (m_editId == id) {
        m_editId.clear();
        setEditName(QString());
        setEditBody(QString());
        emit editIdChanged();
        m_editing = false;
        emit editingChanged();
    }
    setStatus(tr("世界观已删除"));
}

void WorldsViewModel::togglePreview()
{
    m_preview = !m_preview;
    emit previewChanged();
}

void WorldsViewModel::setEditName(const QString &name)
{
    if (m_editName == name)
        return;
    m_editName = name;
    emit editNameChanged();
}

void WorldsViewModel::setEditBody(const QString &body)
{
    if (m_editBody == body)
        return;
    m_editBody = body;
    emit editBodyChanged();
}

void WorldsViewModel::rebuild()
{
    QHash<QString, int> characterCounts;
    for (const Character &character : m_characters->list()) {
        if (!character.worldId.isEmpty())
            ++characterCounts[character.worldId];
    }

    QVariantList rows;
    for (const World &entry : m_worlds->list()) {
        World full;
        QString error;
        const bool ok = m_worlds->read(entry.id, &full, &error);
        const QString content = ok ? full.content : entry.content;

        QVariantMap item;
        item.insert(QStringLiteral("id"), entry.id);
        item.insert(QStringLiteral("name"), entry.name);
        item.insert(QStringLiteral("bodyPreview"), content.simplified().left(bodyPreviewLength));
        item.insert(QStringLiteral("characterCount"), characterCounts.value(entry.id, 0));
        rows.append(item);
    }
    m_worldsCache = rows;
    emit worldsChanged();
}
