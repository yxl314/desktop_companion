/**
 * \file src/viewmodels/memoriesviewmodel.cpp
 * \brief MemoriesViewModel 实现：按角色/人设分区读写记忆并刷新列表。
 */

#include "memoriesviewmodel.h"

#include "data/memorystore.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"


MemoriesViewModel::MemoriesViewModel(MemoryStore *memory, ChatService *chat, QObject *parent)
    : ViewModelBase(parent)
    , m_memory(memory)
    , m_chat(chat)
{
    Q_ASSERT(m_memory != nullptr);
    Q_ASSERT(m_chat != nullptr);

    connect(m_memory, &MemoryStore::changed, this, &MemoriesViewModel::refresh);
    connect(m_chat, &ChatService::conversationCleared, this, &MemoriesViewModel::refresh);
    connect(m_chat, &ChatService::memoriesChanged, this, &MemoriesViewModel::refresh);
    refresh();
}

MemoriesViewModel::~MemoriesViewModel() = default;

QVariantList MemoriesViewModel::memories() const
{
    return m_memories;
}

int MemoriesViewModel::count() const
{
    return m_count;
}

void MemoriesViewModel::add(const QString &content)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请填写记忆内容"));
        return;
    }
    const QString cid = characterId();
    if (cid.isEmpty()) {
        reportError(static_cast<quint32>(ErrorCode::InvalidState), tr("请先选择角色"));
        return;
    }

    MemoryEntry entry;
    entry.characterId = cid;
    entry.personaId = personaId();
    entry.content = trimmed;
    QString error;
    if (!m_memory->add(&entry, &error)) {
        reportError(static_cast<quint32>(ErrorCode::MemoryLimitReached), error);
        return;
    }
    setStatus(tr("已添加记忆"));
}

void MemoriesViewModel::update(const QString &id, const QString &content)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty())
        return;
    const qint64 memoryId = id.toLongLong();
    for (const MemoryEntry &entry : m_memory->list(characterId(), personaId())) {
        if (entry.id != memoryId)
            continue;
        MemoryEntry updated = entry;
        updated.content = trimmed;
        QString error;
        if (!m_memory->update(updated, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        setStatus(tr("记忆已更新"));
        return;
    }
}

void MemoriesViewModel::remove(const QString &id)
{
    QString error;
    if (!m_memory->remove(id.toLongLong(), &error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("记忆已删除"));
}

void MemoriesViewModel::togglePin(const QString &id)
{
    const qint64 memoryId = id.toLongLong();
    for (const MemoryEntry &entry : m_memory->list(characterId(), personaId())) {
        if (entry.id != memoryId)
            continue;
        QString error;
        if (!m_memory->setPinned(memoryId, !entry.pinned, &error)) {
            reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
            return;
        }
        return;
    }
}

void MemoriesViewModel::refresh()
{
    QVariantList rows;
    if (m_memory->ready()) {
        const QString cid = characterId();
        const QString pid = personaId();
            if (!cid.isEmpty()) {
            for (const MemoryEntry &entry : m_memory->list(cid, pid)) {
                QVariantMap item;
                item.insert(QStringLiteral("id"), entry.id);
                item.insert(QStringLiteral("content"), entry.content);
                item.insert(QStringLiteral("pinned"), entry.pinned);
                item.insert(QStringLiteral("createdAt"), entry.createdAt);
                rows.append(item);
            }
        }
    }
    m_memories = rows;
    emit memoriesChanged();

    const int total = rows.size();
    if (m_count != total) {
        m_count = total;
        emit countChanged();
    }
}

void MemoriesViewModel::extractNow()
{
    setStatus(tr("正在抽取记忆…"));
    emit extractRequested();
}

QString MemoriesViewModel::characterId() const
{
    return m_chat->characterId();
}

QString MemoriesViewModel::personaId() const
{
    return m_chat->personaId();
}
