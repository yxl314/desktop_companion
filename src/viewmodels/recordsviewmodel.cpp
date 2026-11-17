/**
 * \file src/viewmodels/recordsviewmodel.cpp
 * \brief RecordsViewModel 实现：读取当前会话记录、统计评价并导出 Markdown。
 */

#include "recordsviewmodel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "data/chatstore.h"
#include "dialogservice.h"
#include "services/errorcodes.h"

RecordsViewModel::RecordsViewModel(ChatStore *store, DialogService *dialogs, QObject *parent)
    : ViewModelBase(parent)
    , m_store(store)
    , m_dialogs(dialogs)
{
    Q_ASSERT(m_store != nullptr);
    Q_ASSERT(m_dialogs != nullptr);

    connect(m_store, &ChatStore::changed, this, &RecordsViewModel::refresh);
    refresh();
}

RecordsViewModel::~RecordsViewModel() = default;

QVariantList RecordsViewModel::records() const
{
    return m_records;
}

QString RecordsViewModel::feedbackSummary() const
{
    return m_feedbackSummary;
}

void RecordsViewModel::refresh()
{
    QVariantList rows;
    if (m_store->ready()) {
        for (const StoredMessage &message : m_store->recentMessages(300)) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), message.id);
            item.insert(QStringLiteral("role"), message.role);
            item.insert(QStringLiteral("text"), message.content);
            item.insert(QStringLiteral("feedback"), message.feedback);
            item.insert(QStringLiteral("time"), QString());
            rows.append(item);
        }
    }
    m_records = rows;
    emit recordsChanged();

    int likes = 0;
    int dislikes = 0;
    m_store->feedbackStats(&likes, &dislikes);
    const QString summary = QStringLiteral("%1 / %2").arg(likes).arg(dislikes);
    if (m_feedbackSummary != summary) {
        m_feedbackSummary = summary;
        emit feedbackSummaryChanged();
    }
}

void RecordsViewModel::exportChat()
{
    const QString path = m_dialogs->saveFile(tr("导出聊天记录"), tr("Markdown (*.md)"),
                                             QDir::homePath() + QStringLiteral("/chat.md"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), file.errorString());
        return;
    }

    setBusy(true);
    QTextStream out(&file);
    out << tr("# 聊天记录\n\n");
    for (const StoredMessage &message : m_store->recentMessages(10000)) {
        const QString speaker =
            message.role == QLatin1String("user") ? tr("我") : tr("助手");
        out << QStringLiteral("**%1**：").arg(speaker) << message.content
            << QStringLiteral("\n\n");
    }
    file.close();
    setBusy(false);
    setStatus(tr("聊天记录已导出：%1").arg(path));
}

void RecordsViewModel::clearRecords()
{
    if (!m_store->clearCurrentMessages()) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), tr("清空聊天记录失败"));
        return;
    }
    refresh();
    setStatus(tr("聊天记录已清空"));
}
