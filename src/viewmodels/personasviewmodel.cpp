/**
 * \file src/viewmodels/personasviewmodel.cpp
 * \brief PersonasViewModel 实现：人设列表、编辑保存、导入与切换。
 */

#include "personasviewmodel.h"

#include <QFileInfo>
#include <QFutureWatcher>

#include "data/characterstore.h"
#include "data/configstore.h"
#include "data/intimacyrepository.h"
#include "data/personastore.h"
#include "dialogservice.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"
#include "services/personafile.h"

PersonasViewModel::PersonasViewModel(PersonaStore *personas, ChatService *chat, ConfigStore *config,
                                     DialogService *dialogs, CharacterStore *characters,
                                     IntimacyService *intimacy, QObject *parent)
    : ViewModelBase(parent)
    , m_personas(personas)
    , m_chat(chat)
    , m_config(config)
    , m_dialogs(dialogs)
    , m_characters(characters)
    , m_intimacy(intimacy)
{
    Q_ASSERT(m_personas != nullptr);
    Q_ASSERT(m_chat != nullptr);
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_dialogs != nullptr);

    connect(m_personas, &PersonaStore::changed, this, &PersonasViewModel::rebuild);
    connect(m_config, &ConfigStore::changed, this, &PersonasViewModel::rebuild);
    if (m_characters != nullptr)
        connect(m_characters, &CharacterStore::changed, this, &PersonasViewModel::rebuild);
    if (m_intimacy != nullptr)
        connect(m_intimacy, &IntimacyService::changed, this, &PersonasViewModel::rebuild);

    rebuild();
}

PersonasViewModel::~PersonasViewModel() = default;

QVariantList PersonasViewModel::personas() const
{
    return m_personasCache;
}

QString PersonasViewModel::currentName() const
{
    return m_currentName;
}

QString PersonasViewModel::editName() const
{
    return m_editName;
}

QString PersonasViewModel::editBody() const
{
    return m_editBody;
}

bool PersonasViewModel::editing() const
{
    return m_editing;
}

bool PersonasViewModel::preview() const
{
    return m_preview;
}

QString PersonasViewModel::editId() const
{
    return m_editId;
}

QString PersonasViewModel::intimacyLevel() const
{
    return m_intimacyLevel;
}

void PersonasViewModel::createNew()
{
    m_editId.clear();
    m_creatingNew = true;
    setEditName(QString());
    setEditBody(QString());
    m_preview = false;
    emit previewChanged();
    m_editing = true;
    emit editIdChanged();
    emit editingChanged();
    setStatus(tr("新建人设"));
}

void PersonasViewModel::loadFor(const QString &id)
{
    loadPersona(id);
}

void PersonasViewModel::save()
{
    const QString cid = characterId();
    if (cid.isEmpty() || m_characters == nullptr) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请先选择角色"));
        return;
    }

    PersonaData data;
    data.id = m_editId;
    data.name = m_editName.trimmed().isEmpty() ? tr("未命名人设") : m_editName.trimmed();
    data.body = m_editBody;

    QString error;
    if (m_creatingNew || !m_editId.isEmpty()) {
        // 新建或编辑非默认人设：由存储层按名字规范化生成/复用 id。
        QString personaId;
        if (!m_characters->savePersona(cid, data, &personaId, &error)) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
        if (m_creatingNew) {
            m_creatingNew = false;
            m_editId = personaId;
            emit editIdChanged();
            m_config->setPersonaId(personaId);
            QString saveError;
            if (!m_config->save(&saveError))
                reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
        }
    } else {
        // 编辑角色卡默认人设：写回 card.md。
        if (!m_characters->saveCard(cid, data, &error)) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
    }

    m_editing = false;
    emit editingChanged();
    setStatus(tr("人设已保存"));
}

void PersonasViewModel::importMd()
{
    const QString path = m_dialogs->openFile(tr("导入人设（Markdown）"), tr("Markdown (*.md)"));
    if (path.isEmpty())
        return;

    const QString cid = characterId();
    setBusy(true);

    if (cid.isEmpty() || m_characters == nullptr) {
        // 没有当前角色：走全局 PersonaStore 的异步导入。
        auto *watcher = new QFutureWatcher<PersonaImportResult>(this);
        connect(watcher, &QFutureWatcher<PersonaImportResult>::finished, this, [this, watcher, path] {
            const PersonaImportResult result = watcher->result();
            watcher->deleteLater();
            setBusy(false);
            if (!result.ok) {
                reportError(static_cast<quint32>(ErrorCode::InvalidPersona), result.error);
                return;
            }
            setStatus(tr("已导入人设：%1").arg(QFileInfo(path).completeBaseName()));
        });
        watcher->setFuture(m_personas->importFromAsync(path));
        return;
    }

    // 有当前角色：异步解析 .md，随后在界面线程写入角色人设目录。
    auto *watcher = new QFutureWatcher<PersonaParseResult>(this);
    connect(watcher, &QFutureWatcher<PersonaParseResult>::finished, this, [this, watcher, cid] {
        const PersonaParseResult result = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        if (!result.ok) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), result.error);
            return;
        }
        const PersonaData &data = result.data;
        for (const PersonaData &existing : m_characters->personas(cid)) {
            if (existing.name == data.name) {
                reportError(static_cast<quint32>(ErrorCode::FileNameConflict),
                            tr("人设已存在：%1").arg(data.name));
                return;
            }
        }
        QString error;
        QString personaId;
        if (!m_characters->savePersona(cid, data, &personaId, &error)) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
        m_config->setPersonaId(personaId);
        QString saveError;
        if (!m_config->save(&saveError))
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
        setStatus(tr("已导入人设：%1").arg(data.name));
    });
    watcher->setFuture(parsePersonaFileAsync(path));
}

void PersonasViewModel::select(const QString &id)
{
    m_config->setPersonaId(id);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("已切换人设：%1").arg(m_currentName));
}

void PersonasViewModel::remove(const QString &id)
{
    const QString cid = characterId();
    if (!cid.isEmpty() && m_characters != nullptr) {
        if (id.isEmpty())
            return;
        QString error;
        if (!m_characters->removePersona(cid, id, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        if (m_config->values().personaId == id) {
            m_config->setPersonaId(QString());
            QString saveError;
            if (!m_config->save(&saveError))
                reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
        }
        if (m_editId == id) {
            m_editId.clear();
            emit editIdChanged();
        }
        setStatus(tr("人设已删除"));
        return;
    }

    if (!m_personas->remove(id))
        return;
    if (m_config->values().personaId == id) {
        m_config->setPersonaId(QString());
        QString saveError;
        if (!m_config->save(&saveError))
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), saveError);
    }
    setStatus(tr("人设已删除"));
}

void PersonasViewModel::setEditName(const QString &name)
{
    if (m_editName == name)
        return;
    m_editName = name;
    emit editNameChanged();
}

void PersonasViewModel::setEditBody(const QString &body)
{
    if (m_editBody == body)
        return;
    m_editBody = body;
    emit editBodyChanged();
}

void PersonasViewModel::togglePreview()
{
    m_preview = !m_preview;
    emit previewChanged();
}

void PersonasViewModel::rebuild()
{
    QVariantList rows;
    const QString cid = characterId();
    const QString currentPersonaId = m_config->values().personaId;

    if (!cid.isEmpty() && m_characters != nullptr) {
        PersonaData card;
        QString error;
        if (m_characters->card(cid, &card, &error)) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), QString());
            item.insert(QStringLiteral("name"),
                        card.name.isEmpty() ? tr("默认人设") : card.name);
            item.insert(QStringLiteral("current"), currentPersonaId.isEmpty());
            rows.append(item);
        }
        for (const PersonaData &persona : m_characters->personas(cid)) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), persona.id);
            item.insert(QStringLiteral("name"), persona.name);
            item.insert(QStringLiteral("current"), persona.id == currentPersonaId);
            rows.append(item);
        }
    } else {
        for (const Persona &persona : m_personas->personas()) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), persona.id);
            item.insert(QStringLiteral("name"), persona.name);
            item.insert(QStringLiteral("current"),
                        persona.id == currentPersonaId
                            || (currentPersonaId.isEmpty() && persona.builtin));
            rows.append(item);
        }
    }

    m_personasCache = rows;
    emit personasChanged();

    QString name;
    for (const QVariant &value : rows) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("current")).toBool()) {
            name = item.value(QStringLiteral("name")).toString();
            break;
        }
    }
    if (name.isEmpty() && rows.isEmpty() && !cid.isEmpty()) {
        PersonaData card;
        QString error;
        if (m_characters != nullptr && m_characters->card(cid, &card, &error))
            name = card.name;
    }
    if (name.isEmpty())
        name = tr("默认人设");
    if (m_currentName != name) {
        m_currentName = name;
        emit currentNameChanged();
    }

    const QString level = currentIntimacyLevel();
    if (m_intimacyLevel != level) {
        m_intimacyLevel = level;
        emit intimacyLevelChanged();
    }
}

void PersonasViewModel::loadPersona(const QString &id)
{
    const QString cid = characterId();
    if (!cid.isEmpty() && m_characters != nullptr) {
        PersonaData persona;
        QString error;
        const bool ok = id.isEmpty() ? m_characters->card(cid, &persona, &error)
                                     : m_characters->persona(cid, id, &persona, &error);
        if (!ok) {
            reportError(static_cast<quint32>(ErrorCode::InvalidPersona), error);
            return;
        }
        m_creatingNew = false;
        m_editId = id;
        setEditName(persona.name);
        setEditBody(persona.body);
        emit editIdChanged();
        m_editing = true;
        emit editingChanged();
        m_preview = false;
        emit previewChanged();
        return;
    }

    const Persona persona = m_personas->find(id);
    m_creatingNew = false;
    m_editId = persona.id;
    setEditName(persona.name);
    setEditBody(persona.prompt);
    emit editIdChanged();
    m_editing = true;
    emit editingChanged();
    m_preview = false;
    emit previewChanged();
}

QString PersonasViewModel::characterId() const
{
    const QString id = m_config->values().characterId;
    if (id.isEmpty() || m_characters == nullptr || !m_characters->contains(id))
        return QString();
    return id;
}

QString PersonasViewModel::currentIntimacyLevel() const
{
    if (m_intimacy != nullptr && m_intimacy->loaded())
        return m_intimacy->levelName();
    return tr("陌生");
}
