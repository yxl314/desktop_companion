/**
 * \file src/viewmodels/appviewmodel.cpp
 * \brief AppViewModel 实现：配置同步、桌宠控制、更新检查与日志导出。
 */

#include "appviewmodel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>

#include "data/configstore.h"
#include "dialogservice.h"
#include "services/autostart.h"
#include "services/errorcodes.h"

namespace
{
/// 读取 CMake 注入的版本号；未定义时给出开发版兜底。
QString versionString()
{
#ifdef DESKTOP_COMPANION_VERSION
    return QStringLiteral(DESKTOP_COMPANION_VERSION);
#else
    return QStringLiteral("0.0.0-dev");
#endif
}
} // namespace

AppViewModel::AppViewModel(ConfigStore *config, QObject *parent, DialogService *dialogs)
    : ViewModelBase(parent)
    , m_config(config)
    , m_dialogs(dialogs)
{
    Q_ASSERT(m_config != nullptr);

    m_updateUrl = m_config->values().updateUrl;
    connect(m_config, &ConfigStore::changed, this, &AppViewModel::syncFromConfig);
    syncFromConfig();
}

AppViewModel::~AppViewModel() = default;

int AppViewModel::pageIndex() const
{
    return m_pageIndex;
}

QString AppViewModel::appVersion() const
{
    return versionString();
}

QString AppViewModel::appStatus() const
{
    return m_appStatus;
}

int AppViewModel::themeIndex() const
{
    return m_themeIndex;
}

int AppViewModel::languageIndex() const
{
    return m_languageIndex;
}

bool AppViewModel::petVisible() const
{
    return m_petVisible;
}

int AppViewModel::petCount() const
{
    return m_petCount;
}

QString AppViewModel::activePetCharacterId() const
{
    return m_activePetCharacterId;
}

bool AppViewModel::autoStartPet() const
{
    return m_autoStartPet;
}

bool AppViewModel::autoMode() const
{
    return m_autoMode;
}

int AppViewModel::mode() const
{
    return m_mode;
}

bool AppViewModel::edgeHide() const
{
    return m_edgeHide;
}

bool AppViewModel::clickThrough() const
{
    return m_clickThrough;
}

bool AppViewModel::idleMotion() const
{
    return m_idleMotion;
}

double AppViewModel::idleAmplitude() const
{
    return m_idleAmplitude;
}

bool AppViewModel::debugHitAreas() const
{
    return m_debugHitAreas;
}

bool AppViewModel::ballVisible() const
{
    return m_ballVisible;
}

bool AppViewModel::intimacyDecay() const
{
    return m_intimacyDecay;
}

bool AppViewModel::autostart() const
{
    return m_autostart;
}

int AppViewModel::proactiveFrequency() const
{
    return m_proactiveFrequency;
}

QString AppViewModel::petStatusText() const
{
    return m_petStatusText;
}

bool AppViewModel::onboardingDone() const
{
    return m_onboardingDone;
}

QString AppViewModel::updateStatus() const
{
    return m_updateStatus;
}

QString AppViewModel::updateUrl() const
{
    return m_updateUrl;
}

void AppViewModel::setPageIndex(int index)
{
    const int bounded = qBound(0, index, 8);
    if (m_pageIndex == bounded)
        return;
    m_pageIndex = bounded;
    emit pageIndexChanged();
}

void AppViewModel::setThemeIndex(int index)
{
    m_config->setTheme(qBound(0, index, 2));
    saveConfig();
}

void AppViewModel::setLanguageIndex(int index)
{
    m_config->setLanguage(qBound(0, index, 2));
    saveConfig();
}

void AppViewModel::setPetVisible(bool visible)
{
    if (m_petVisible == visible)
        return;
    m_petVisible = visible;
    emit petVisibleChanged();
    refreshPetStatus();
}

void AppViewModel::setPetCount(int count)
{
    m_config->setPetCount(qBound(1, count, 3));
    saveConfig();
    refreshPetStatus();
}

void AppViewModel::launchPet()
{
    setPetVisible(true);
    setAppStatus(tr("桌宠已启动"));
    emit toast(tr("桌宠已启动"));
}

void AppViewModel::stopPet()
{
    setPetVisible(false);
    setAppStatus(tr("桌宠已停止"));
    emit toast(tr("桌宠已停止"));
}

void AppViewModel::setActivePetCharacter(const QString &characterId)
{
    m_config->setPetCharacter(0, characterId);
    saveConfig();
}

void AppViewModel::setAutoStartPet(bool enabled)
{
    if (m_autoStartPet == enabled && m_autostart == enabled)
        return;
    autostart::setEnabled(enabled, QCoreApplication::applicationFilePath());
    syncFromConfig();
}

void AppViewModel::setAutoMode(bool enabled)
{
    m_config->setAutoMode(enabled);
    saveConfig();
}

void AppViewModel::setMode(int mode)
{
    m_config->setMode(mode);
    saveConfig();
}

void AppViewModel::setEdgeHide(bool enabled)
{
    m_config->setEdgeHide(enabled);
    saveConfig();
}

void AppViewModel::setClickThrough(bool enabled)
{
    const AppConfig &values = m_config->values();
    m_config->setPetBehavior(values.idleMotion, values.idleAmplitude, enabled, values.debugHitAreas);
    saveConfig();
}

void AppViewModel::setIdleMotion(bool enabled)
{
    const AppConfig &values = m_config->values();
    m_config->setPetBehavior(enabled, values.idleAmplitude, values.clickThrough, values.debugHitAreas);
    saveConfig();
}

void AppViewModel::setIdleAmplitude(double amplitude)
{
    const AppConfig &values = m_config->values();
    m_config->setPetBehavior(values.idleMotion, qBound(0.2, amplitude, 2.0), values.clickThrough,
                             values.debugHitAreas);
    saveConfig();
}

void AppViewModel::setDebugHitAreas(bool enabled)
{
    const AppConfig &values = m_config->values();
    m_config->setPetBehavior(values.idleMotion, values.idleAmplitude, values.clickThrough, enabled);
    saveConfig();
}

void AppViewModel::setBallVisible(bool visible)
{
    const AppConfig &values = m_config->values();
    const double x = values.ballX >= 0.0 ? values.ballX : 900.0;
    const double y = values.ballY >= 0.0 ? values.ballY : 600.0;
    m_config->setBall(visible, x, y);
    saveConfig();
}

double AppViewModel::ballX() const
{
    const double x = m_config->values().ballX;
    return x >= 0.0 ? x : 900.0;
}

double AppViewModel::ballY() const
{
    const double y = m_config->values().ballY;
    return y >= 0.0 ? y : 600.0;
}

void AppViewModel::setBallPosition(double x, double y)
{
    m_config->setBall(m_config->values().ballVisible, x, y);
    saveConfig();
    emit ballPositionChanged();
}

void AppViewModel::setIntimacyDecay(bool enabled)
{
    m_config->setIntimacyDecay(enabled);
    saveConfig();
}

void AppViewModel::setProactiveFrequency(int frequency)
{
    m_config->setMemoryPrefs(m_config->values().autoExtractMemories, qBound(0, frequency, 3));
    saveConfig();
}

void AppViewModel::setAutostart(bool enabled)
{
    if (m_autostart == enabled && m_autoStartPet == enabled)
        return;
    autostart::setEnabled(enabled, QCoreApplication::applicationFilePath());
    syncFromConfig();
}

void AppViewModel::setUpdateUrl(const QString &url)
{
    m_config->setUpdateUrl(url);
    saveConfig();
}

void AppViewModel::setOnboardingDone(bool done)
{
    m_config->setOnboardingDone(done);
    saveConfig();
}

void AppViewModel::checkUpdate()
{
    if (m_network == nullptr)
        m_network = new QNetworkAccessManager(this);

    const QString url = m_config->values().updateUrl;
    if (url.isEmpty()) {
        setUpdateStatus(tr("未配置更新地址（config.json 的 updateUrl 可填版本清单 URL）"));
        setAppStatus(m_updateStatus);
        return;
    }

    setUpdateStatus(tr("正在检查更新…"));
    setAppStatus(m_updateStatus);

    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(8000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setUpdateStatus(tr("检查失败：%1").arg(reply->errorString()));
            return;
        }
        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setUpdateStatus(tr("检查失败：版本清单格式错误"));
            return;
        }
        const QJsonObject root = document.object();
        const QString latest = root.value(QStringLiteral("version")).toString();
        const QString downloadUrl = root.value(QStringLiteral("url")).toString();
        const QVersionNumber current = QVersionNumber::fromString(appVersion());
        const QVersionNumber remote = QVersionNumber::fromString(latest);
        if (!remote.isNull() && !current.isNull() && remote > current) {
            setUpdateStatus(tr("发现新版本 %1：%2").arg(latest, downloadUrl));
            emit updateAvailable(latest);
            emit toast(tr("发现新版本 %1").arg(latest));
        } else {
            setUpdateStatus(tr("已是最新版本（%1）").arg(appVersion()));
        }
    });
}

void AppViewModel::restartApp()
{
    saveConfig();
    const QString executable = QCoreApplication::applicationFilePath();
    QProcess::startDetached(QStringLiteral("cmd.exe"),
                            {QStringLiteral("/c"), QStringLiteral("start"), QString(),
                             QDir::toNativeSeparators(executable)});
    QTimer::singleShot(150, this, [] { QCoreApplication::quit(); });
}

void AppViewModel::openDataDir()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(dataDir()));
}

void AppViewModel::openLogsDir()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir()));
}

void AppViewModel::exportLogs()
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    QString destination;
    if (m_dialogs != nullptr) {
        const QString targetDir = m_dialogs->openDirectory(tr("选择日志导出目录"));
        if (targetDir.isEmpty())
            return;
        destination = targetDir + QStringLiteral("/desktop_companion-logs-") + stamp;
    } else {
        const QString exportsDir = dataDir() + QStringLiteral("/exports");
        QDir().mkpath(exportsDir);
        destination = exportsDir + QStringLiteral("/logs-") + stamp;
    }

    if (!QDir().mkpath(destination)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), destination);
        return;
    }

    int copied = 0;
    QDirIterator iterator(logDir(), QDir::Files);
    while (iterator.hasNext()) {
        const QString source = iterator.next();
        const QString target = destination + QLatin1Char('/') + QFileInfo(source).fileName();
        if (QFile::copy(source, target))
            ++copied;
    }
    setAppStatus(tr("已导出 %1 个日志文件到 %2").arg(copied).arg(destination));
}

void AppViewModel::showPetMenu(int index)
{
    emit petMenuRequested(index);
}

void AppViewModel::showMainWindow()
{
    m_mainWindowVisible = true;
    emit mainWindowVisibilityRequested(true);
}

void AppViewModel::hideMainWindow()
{
    m_mainWindowVisible = false;
    emit mainWindowVisibilityRequested(false);
}

void AppViewModel::toggleMainWindow()
{
    m_mainWindowVisible = !m_mainWindowVisible;
    emit mainWindowVisibilityRequested(m_mainWindowVisible);
}

void AppViewModel::syncFromConfig()
{
    const AppConfig &values = m_config->values();

    if (m_themeIndex != values.theme) {
        m_themeIndex = values.theme;
        emit themeIndexChanged();
    }
    if (m_languageIndex != values.language) {
        m_languageIndex = values.language;
        emit languageIndexChanged();
    }
    if (m_petCount != values.petCount) {
        m_petCount = values.petCount;
        emit petCountChanged();
    }
    if (m_mode != values.mode) {
        m_mode = values.mode;
        emit modeChanged();
    }
    if (m_proactiveFrequency != values.proactiveFrequency) {
        m_proactiveFrequency = values.proactiveFrequency;
        emit proactiveFrequencyChanged();
    }
    if (!qFuzzyCompare(m_idleAmplitude, values.idleAmplitude)) {
        m_idleAmplitude = values.idleAmplitude;
        emit idleAmplitudeChanged();
    }
    if (m_autoMode != values.autoMode) {
        m_autoMode = values.autoMode;
        emit autoModeChanged();
    }
    if (m_edgeHide != values.edgeHide) {
        m_edgeHide = values.edgeHide;
        emit edgeHideChanged();
    }
    if (m_clickThrough != values.clickThrough) {
        m_clickThrough = values.clickThrough;
        emit clickThroughChanged();
    }
    if (m_idleMotion != values.idleMotion) {
        m_idleMotion = values.idleMotion;
        emit idleMotionChanged();
    }
    if (m_debugHitAreas != values.debugHitAreas) {
        m_debugHitAreas = values.debugHitAreas;
        emit debugHitAreasChanged();
    }
    if (m_ballVisible != values.ballVisible) {
        m_ballVisible = values.ballVisible;
        emit ballVisibleChanged();
    }
    if (m_intimacyDecay != values.intimacyDecay) {
        m_intimacyDecay = values.intimacyDecay;
        emit intimacyDecayChanged();
    }
    if (m_onboardingDone != values.onboardingDone) {
        m_onboardingDone = values.onboardingDone;
        emit onboardingDoneChanged();
    }
    if (m_updateUrl != values.updateUrl) {
        m_updateUrl = values.updateUrl;
        emit updateUrlChanged();
    }

    const bool autostartEnabled = autostart::enabled();
    if (m_autostart != autostartEnabled) {
        m_autostart = autostartEnabled;
        emit autostartChanged();
    }
    if (m_autoStartPet != autostartEnabled) {
        m_autoStartPet = autostartEnabled;
        emit autoStartPetChanged();
    }

    QString petCharacter;
    if (!values.petCharacters.isEmpty())
        petCharacter = values.petCharacters.at(0);
    if (petCharacter.isEmpty())
        petCharacter = values.characterId;
    if (m_activePetCharacterId != petCharacter) {
        m_activePetCharacterId = petCharacter;
        emit activePetCharacterChanged();
    }

    refreshPetStatus();
}

void AppViewModel::refreshPetStatus()
{
    const QString text = m_petVisible ? tr("桌宠运行中（%1 只）").arg(m_petCount) : tr("桌宠已隐藏");
    if (m_petStatusText == text)
        return;
    m_petStatusText = text;
    emit petStatusTextChanged();
}

void AppViewModel::setAppStatus(const QString &text)
{
    setStatus(text);
    if (m_appStatus != text) {
        m_appStatus = text;
        emit appStatusChanged();
    }
    emit statusMessage(text);
}

void AppViewModel::setUpdateStatus(const QString &text)
{
    if (m_updateStatus == text)
        return;
    m_updateStatus = text;
    emit updateStatusChanged();
}

void AppViewModel::saveConfig()
{
    QString error;
    if (!m_config->save(&error)) {
        reportError(static_cast<quint32>(ErrorCode::FileWriteFailed), error);
        return;
    }
}

QString AppViewModel::dataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString AppViewModel::logDir() const
{
    return dataDir() + QStringLiteral("/logs");
}
