/**
 * \file src/viewmodels/dataviewmodel.cpp
 * \brief DataViewModel 实现：目录展示、导出、占用统计与更新检查。
 */

#include "dataviewmodel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVersionNumber>

#include "data/chatstore.h"
#include "data/configstore.h"
#include "dialogservice.h"
#include "services/chatservice.h"
#include "services/errorcodes.h"

namespace
{
/// 递归统计目录中普通文件的总字节数。
qint64 directorySize(const QString &path)
{
    qint64 total = 0;
    QDirIterator iterator(path, QDir::Files);
    while (iterator.hasNext()) {
        iterator.next();
        total += iterator.fileInfo().size();
    }
    return total;
}

/// 应用版本号（CMake 注入宏；未定义时给出开发版兜底）。
QString versionString()
{
#ifdef DESKTOP_COMPANION_VERSION
    return QStringLiteral(DESKTOP_COMPANION_VERSION);
#else
    return QStringLiteral("0.0.0-dev");
#endif
}
} // namespace

DataViewModel::DataViewModel(ConfigStore *config, ChatStore *store, ChatService *chat,
                             DialogService *dialogs, QObject *parent)
    : ViewModelBase(parent)
    , m_config(config)
    , m_store(store)
    , m_chat(chat)
    , m_dialogs(dialogs)
{
    Q_ASSERT(m_config != nullptr);
    Q_ASSERT(m_store != nullptr);
    Q_ASSERT(m_chat != nullptr);
    Q_ASSERT(m_dialogs != nullptr);

    connect(m_store, &ChatStore::changed, this, &DataViewModel::refreshSize);
    refreshSize();
}

DataViewModel::~DataViewModel() = default;

QString DataViewModel::dataPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString DataViewModel::logPath() const
{
    return dataPath() + QStringLiteral("/logs");
}

QString DataViewModel::dataSize() const
{
    return m_dataSize;
}

QString DataViewModel::lastExportPath() const
{
    return m_lastExportPath;
}

void DataViewModel::exportChats()
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString path = m_dialogs->saveFile(
        tr("导出聊天记录"), tr("Markdown (*.md)"),
        QDir::homePath() + QStringLiteral("/chat-") + stamp + QStringLiteral(".md"));
    if (path.isEmpty())
        return;

    if (!m_store->ready()) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), tr("聊天库不可用"));
        return;
    }

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
    setLastExportPath(path);
    setStatus(tr("聊天记录已导出：%1").arg(path));
}

void DataViewModel::exportLogs()
{
    const QString targetDir = m_dialogs->openDirectory(tr("选择日志导出目录"));
    if (targetDir.isEmpty())
        return;

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString destination = targetDir + QStringLiteral("/desktop_companion-logs-") + stamp;
    if (!QDir().mkpath(destination)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), destination);
        return;
    }

    setBusy(true);
    int copied = 0;
    QDirIterator iterator(logPath(), QDir::Files);
    while (iterator.hasNext()) {
        const QString source = iterator.next();
        const QString target = destination + QLatin1Char('/') + QFileInfo(source).fileName();
        if (QFile::copy(source, target))
            ++copied;
    }
    setBusy(false);
    setLastExportPath(destination);
    setStatus(tr("已导出 %1 个日志文件到 %2").arg(copied).arg(destination));
}

void DataViewModel::openDataDir()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(dataPath()));
}

void DataViewModel::openLogsDir()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath()));
}

void DataViewModel::resetOnboarding()
{
    m_config->setOnboardingDone(false);
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
    setStatus(tr("已重置新手引导，下次启动时重新展示"));
}

void DataViewModel::checkUpdate()
{
    if (m_network == nullptr)
        m_network = new QNetworkAccessManager(this);

    const QString url = m_config->values().updateUrl;
    if (url.isEmpty()) {
        setStatus(tr("未配置更新地址（config.json 的 updateUrl 可填版本清单 URL）"));
        return;
    }

    setStatus(tr("正在检查更新…"));
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(8000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setError(tr("检查失败：%1").arg(reply->errorString()));
            return;
        }
        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setError(tr("检查失败：版本清单格式错误"));
            return;
        }
        const QJsonObject root = document.object();
        const QString latest = root.value(QStringLiteral("version")).toString();
        const QString downloadUrl = root.value(QStringLiteral("url")).toString();
        const QVersionNumber current = QVersionNumber::fromString(versionString());
        const QVersionNumber remote = QVersionNumber::fromString(latest);
        if (!remote.isNull() && !current.isNull() && remote > current)
            setStatus(tr("发现新版本 %1：%2").arg(latest, downloadUrl));
        else
            setStatus(tr("已是最新版本（%1）").arg(versionString()));
    });
}

void DataViewModel::refreshSize()
{
    const QString text = QLocale::system().formattedDataSize(directorySize(dataPath()));
    if (m_dataSize == text)
        return;
    m_dataSize = text;
    emit dataSizeChanged();
}

void DataViewModel::setLastExportPath(const QString &path)
{
    if (m_lastExportPath == path)
        return;
    m_lastExportPath = path;
    emit lastExportPathChanged();
}
