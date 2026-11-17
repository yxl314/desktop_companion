/**
 * \file src/viewmodels/appviewmodel.h
 * \brief 应用外壳 ViewModel：页面导航、主题/语言、桌宠控制与系统设置。
 *
 * AppViewModel 是主窗口的根 ViewModel：侧栏导航、状态栏、设置页与桌宠窗口的
 * 显示开关都由它驱动。它只读写 ConfigStore / autostart 等模型层，
 * 不直接创建窗口；窗口的显示/隐藏通过信号交给 app shell。
 */

#ifndef APPVIEWMODEL_H
#define APPVIEWMODEL_H

#include <QObject>
#include <QString>

#include "viewmodelbase.h"

class ConfigStore;
class DialogService;
class QNetworkAccessManager;

/**
 * \brief 应用外壳与设置 ViewModel。
 *
 * 页面下标约定：0 聊天 1 角色 2 人设 3 世界观 4 记忆 5 模型 6 记录 7 数据 8 设置。
 * 主题/语言/模式等设置持久化到 config.json；桌宠显示开关由 petVisible 驱动
 * PetWindow 的显隐（主窗口不受影响）。
 */
class AppViewModel : public ViewModelBase
{
    Q_OBJECT
    /** \brief 当前页面下标：0 聊天 … 8 设置。 */
    Q_PROPERTY(int pageIndex READ pageIndex WRITE setPageIndex NOTIFY pageIndexChanged)
    /** \brief 应用版本号（只读）。 */
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    /** \brief 状态栏文本（只读，与 statusText 同步）。 */
    Q_PROPERTY(QString appStatus READ appStatus NOTIFY appStatusChanged)
    /** \brief 主题下标：0 浅色 1 深色 2 跟随系统。 */
    Q_PROPERTY(int themeIndex READ themeIndex WRITE setThemeIndex NOTIFY themeIndexChanged)
    /** \brief 语言下标：0 中文 1 English 2 跟随系统。 */
    Q_PROPERTY(int languageIndex READ languageIndex WRITE setLanguageIndex NOTIFY languageIndexChanged)
    /** \brief 桌宠窗口总开关。 */
    Q_PROPERTY(bool petVisible READ petVisible WRITE setPetVisible NOTIFY petVisibleChanged)
    /** \brief 桌宠数量（1~3）。 */
    Q_PROPERTY(int petCount READ petCount WRITE setPetCount NOTIFY petCountChanged)
    /** \brief 自动/手动启动的桌宠角色 id。 */
    Q_PROPERTY(QString activePetCharacterId READ activePetCharacterId WRITE setActivePetCharacter
                    NOTIFY activePetCharacterChanged)
    /** \brief 开机自动启动桌宠。 */
    Q_PROPERTY(bool autoStartPet READ autoStartPet WRITE setAutoStartPet NOTIFY autoStartPetChanged)
    /** \brief 是否按前台应用自动切换模式。 */
    Q_PROPERTY(bool autoMode READ autoMode WRITE setAutoMode NOTIFY autoModeChanged)
    /** \brief 当前模式：0 休闲 1 工作 2 勿扰。 */
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    /** \brief 是否贴边隐藏。 */
    Q_PROPERTY(bool edgeHide READ edgeHide WRITE setEdgeHide NOTIFY edgeHideChanged)
    /** \brief 是否开启点击穿透。 */
    Q_PROPERTY(bool clickThrough READ clickThrough WRITE setClickThrough NOTIFY clickThroughChanged)
    /** \brief 是否启用待机动作。 */
    Q_PROPERTY(bool idleMotion READ idleMotion WRITE setIdleMotion NOTIFY idleMotionChanged)
    /** \brief 待机动作幅度（0.2~2.0）。 */
    Q_PROPERTY(double idleAmplitude READ idleAmplitude WRITE setIdleAmplitude NOTIFY idleAmplitudeChanged)
    /** \brief 是否显示命中区调试。 */
    Q_PROPERTY(bool debugHitAreas READ debugHitAreas WRITE setDebugHitAreas NOTIFY debugHitAreasChanged)
    /** \brief 是否显示悬浮球。 */
    Q_PROPERTY(bool ballVisible READ ballVisible WRITE setBallVisible NOTIFY ballVisibleChanged)
    /** \brief 悬浮球横坐标（逻辑像素；持久化到 config.json ballX）。 */
    Q_PROPERTY(double ballX READ ballX NOTIFY ballPositionChanged)
    /** \brief 悬浮球纵坐标（逻辑像素；持久化到 config.json ballY）。 */
    Q_PROPERTY(double ballY READ ballY NOTIFY ballPositionChanged)
    /** \brief 好感度是否跨天衰减。 */
    Q_PROPERTY(bool intimacyDecay READ intimacyDecay WRITE setIntimacyDecay NOTIFY intimacyDecayChanged)
    /** \brief 应用是否开机自启。 */
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)
    /** \brief 主动对话频率：0 关 1 低 2 中 3 高。 */
    Q_PROPERTY(int proactiveFrequency READ proactiveFrequency WRITE setProactiveFrequency
                    NOTIFY proactiveFrequencyChanged)
    /** \brief 桌宠状态文案，如「桌宠运行中（1 只）」。 */
    Q_PROPERTY(QString petStatusText READ petStatusText NOTIFY petStatusTextChanged)
    /** \brief 新手引导是否已完成。 */
    Q_PROPERTY(bool onboardingDone READ onboardingDone WRITE setOnboardingDone NOTIFY onboardingDoneChanged)
    /** \brief 更新检查结果文本（只读）。 */
    Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateStatusChanged)
    /** \brief 更新检查地址（config.json 的 updateUrl）。 */
    Q_PROPERTY(QString updateUrl READ updateUrl WRITE setUpdateUrl NOTIFY updateUrlChanged)

public:
    /**
     * \brief 构造应用外壳 ViewModel。
     * \param config 应用配置仓库，必须非空。
     * \param parent 父对象。
     * \param dialogs 可选的对话框服务；为空时导出日志走默认目录，不弹文件框。
     */
    explicit AppViewModel(ConfigStore *config, QObject *parent = nullptr,
                          DialogService *dialogs = nullptr);

    /// 析构函数。
    ~AppViewModel() override;

    /// \brief 当前页面下标。
    int pageIndex() const;
    /// \brief 应用版本号。
    QString appVersion() const;
    /// \brief 状态栏文本。
    QString appStatus() const;
    /// \brief 主题下标。
    int themeIndex() const;
    /// \brief 语言下标。
    int languageIndex() const;
    /// \brief 桌宠窗口是否可见。
    bool petVisible() const;
    /// \brief 桌宠数量。
    int petCount() const;
    /// \brief 当前桌宠角色 id。
    QString activePetCharacterId() const;
    /// \brief 是否开机自动启动桌宠。
    bool autoStartPet() const;
    /// \brief 是否按前台应用自动切换模式。
    bool autoMode() const;
    /// \brief 当前模式下标。
    int mode() const;
    /// \brief 是否贴边隐藏。
    bool edgeHide() const;
    /// \brief 是否开启点击穿透。
    bool clickThrough() const;
    /// \brief 是否启用待机动作。
    bool idleMotion() const;
    /// \brief 待机动作幅度。
    double idleAmplitude() const;
    /// \brief 是否显示命中区调试。
    bool debugHitAreas() const;
    /// \brief 是否显示悬浮球。
    bool ballVisible() const;
    /// 悬浮球横坐标。
    double ballX() const;
    /// 悬浮球纵坐标。
    double ballY() const;
    /// \brief 好感度是否跨天衰减。
    bool intimacyDecay() const;
    /// \brief 应用是否开机自启。
    bool autostart() const;
    /// \brief 主动对话频率。
    int proactiveFrequency() const;
    /// \brief 桌宠状态文案。
    QString petStatusText() const;
    /// \brief 新手引导是否完成。
    bool onboardingDone() const;
    /// \brief 更新检查结果文本。
    QString updateStatus() const;
    /// \brief 更新检查地址。
    QString updateUrl() const;

    /**
     * \brief 切换页面。
     * \param index 目标页面下标，越界时夹到 0~8。
     */
    Q_INVOKABLE void setPageIndex(int index);
    /**
     * \brief 设置主题。
     * \param index 0 浅色 1 深色 2 跟随系统。
     */
    Q_INVOKABLE void setThemeIndex(int index);
    /**
     * \brief 设置语言。
     * \param index 0 中文 1 English 2 跟随系统。
     */
    Q_INVOKABLE void setLanguageIndex(int index);
    /**
     * \brief 显示/隐藏桌宠窗口。
     * \param visible 是否显示。
     */
    Q_INVOKABLE void setPetVisible(bool visible);
    /**
     * \brief 设置桌宠数量。
     * \param count 目标数量，夹到 1~3。
     */
    Q_INVOKABLE void setPetCount(int count);
    /// 启动桌宠（petVisible = true）。
    Q_INVOKABLE void launchPet();
    /// 停止桌宠（petVisible = false）。
    Q_INVOKABLE void stopPet();
    /**
     * \brief 设置当前桌宠角色。
     * \param characterId 角色 id，空串表示跟随当前角色。
     */
    Q_INVOKABLE void setActivePetCharacter(const QString &characterId);
    /**
     * \brief 设置开机自动启动桌宠。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setAutoStartPet(bool enabled);
    /**
     * \brief 设置前台自动模式。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setAutoMode(bool enabled);
    /**
     * \brief 设置当前模式。
     * \param mode 0 休闲 1 工作 2 勿扰。
     */
    Q_INVOKABLE void setMode(int mode);
    /**
     * \brief 设置贴边隐藏。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setEdgeHide(bool enabled);
    /**
     * \brief 设置点击穿透。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setClickThrough(bool enabled);
    /**
     * \brief 设置待机动作开关。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setIdleMotion(bool enabled);
    /**
     * \brief 设置待机动作幅度。
     * \param amplitude 幅度，夹到 0.2~2.0。
     */
    Q_INVOKABLE void setIdleAmplitude(double amplitude);
    /**
     * \brief 设置命中区调试显示。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setDebugHitAreas(bool enabled);
    /**
     * \brief 设置悬浮球显示。
     * \param visible 是否显示。
     */
    Q_INVOKABLE void setBallVisible(bool visible);
    /**
     * \brief 保存悬浮球位置。
     * @param x 逻辑像素横坐标。
     * @param y 逻辑像素纵坐标。
     */
    Q_INVOKABLE void setBallPosition(double x, double y);
    /**
     * \brief 设置好感度跨天衰减。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setIntimacyDecay(bool enabled);
    /**
     * \brief 设置主动对话频率。
     * \param frequency 0~3，越界时夹取。
     */
    Q_INVOKABLE void setProactiveFrequency(int frequency);
    /**
     * \brief 设置应用开机自启。
     * \param enabled 是否启用。
     */
    Q_INVOKABLE void setAutostart(bool enabled);
    /**
     * \brief 设置更新检查地址。
     * \param url 版本清单 URL。
     */
    Q_INVOKABLE void setUpdateUrl(const QString &url);
    /**
     * \brief 设置新手引导完成状态。
     * \param done 是否完成。
     */
    Q_INVOKABLE void setOnboardingDone(bool done);
    /// 异步检查更新（读取 updateUrl 的版本清单）。
    Q_INVOKABLE void checkUpdate();
    /// 重启应用：保存配置后启动新实例并退出当前实例。
    Q_INVOKABLE void restartApp();
    /// 打开数据目录。
    Q_INVOKABLE void openDataDir();
    /// 打开日志目录。
    Q_INVOKABLE void openLogsDir();
    /// 导出日志到用户选择（或默认）目录。
    Q_INVOKABLE void exportLogs();
    /**
     * \brief 请求在指定桌宠上弹出右键菜单。
     * \param index 桌宠下标。
     */
    Q_INVOKABLE void showPetMenu(int index);
    /// 请求显示主窗口（由 Application 响应）。
    Q_INVOKABLE void showMainWindow();
    /// 请求隐藏主窗口（由 Application 响应）。
    Q_INVOKABLE void hideMainWindow();
    /// 请求切换主窗口显隐（由 Application 响应）。
    Q_INVOKABLE void toggleMainWindow();

signals:
    /// pageIndex 属性变化信号。
    void pageIndexChanged();
    /// appStatus 属性变化信号。
    void appStatusChanged();
    /// themeIndex 属性变化信号。
    void themeIndexChanged();
    /// languageIndex 属性变化信号。
    void languageIndexChanged();
    /// petVisible 属性变化信号。
    void petVisibleChanged();
    /// petCount 属性变化信号。
    void petCountChanged();
    /// activePetCharacterId 属性变化信号。
    void activePetCharacterChanged();
    /// autoStartPet 属性变化信号。
    void autoStartPetChanged();
    /// autoMode 属性变化信号。
    void autoModeChanged();
    /// mode 属性变化信号。
    void modeChanged();
    /// edgeHide 属性变化信号。
    void edgeHideChanged();
    /// clickThrough 属性变化信号。
    void clickThroughChanged();
    /// idleMotion 属性变化信号。
    void idleMotionChanged();
    /// idleAmplitude 属性变化信号。
    void idleAmplitudeChanged();
    /// debugHitAreas 属性变化信号。
    void debugHitAreasChanged();
    /// ballVisible 属性变化信号。
    void ballVisibleChanged();
    /// 悬浮球位置变化信号。
    void ballPositionChanged();
    /// intimacyDecay 属性变化信号。
    void intimacyDecayChanged();
    /// autostart 属性变化信号。
    void autostartChanged();
    /// proactiveFrequency 属性变化信号。
    void proactiveFrequencyChanged();
    /// petStatusText 属性变化信号。
    void petStatusTextChanged();
    /// onboardingDone 属性变化信号。
    void onboardingDoneChanged();
    /// updateStatus 属性变化信号。
    void updateStatusChanged();
    /// updateUrl 属性变化信号。
    void updateUrlChanged();

    /// 状态栏消息（供 shell 显示 toast/状态栏）。
    void statusMessage(const QString &message);
    /// 发现新版本（参数为远端版本号）。
    void updateAvailable(const QString &version);
    /// 轻量提示消息（供 shell 显示 toast）。
    void toast(const QString &message);
    /// 请求 Application 在指定桌宠上弹出右键菜单（附加信号，契约外集成项）。
    void petMenuRequested(int index);
    /// 请求 Application 显示/隐藏主窗口（附加信号，契约外集成项）。
    void mainWindowVisibilityRequested(bool visible);

private:
    /// 从 ConfigStore 同步缓存值并派发属性变化信号。
    void syncFromConfig();
    /// 重新计算桌宠状态文案。
    void refreshPetStatus();
    /**
     * \brief 更新状态栏文本并广播 statusMessage。
     * \param text 新文本。
     */
    void setAppStatus(const QString &text);
    /**
     * \brief 更新更新检查结果文本。
     * \param text 新文本。
     */
    void setUpdateStatus(const QString &text);
    /// 保存配置，失败时写入 errorText。
    void saveConfig();
    /// 应用数据目录。
    QString dataDir() const;
    /// 日志目录。
    QString logDir() const;

    ConfigStore *m_config = nullptr;                 ///< 配置仓库。
    DialogService *m_dialogs = nullptr;              ///< 可选对话框服务。
    QNetworkAccessManager *m_network = nullptr;      ///< 更新检查网络访问（惰性创建）。

    int m_pageIndex = 0;             ///< 当前页面下标。
    bool m_petVisible = true;        ///< 桌宠窗口总开关。
    bool m_mainWindowVisible = true; ///< 主窗口当前显隐（由 Application 保持同步）。
    QString m_appStatus;             ///< 状态栏文本。
    QString m_petStatusText;         ///< 桌宠状态文案。
    QString m_updateStatus;          ///< 更新检查结果。
    QString m_updateUrl;             ///< 更新地址缓存。

    int m_themeIndex = 0;            ///< 主题缓存。
    int m_languageIndex = 2;         ///< 语言缓存。
    int m_petCount = 1;              ///< 桌宠数量缓存。
    int m_mode = 0;                  ///< 模式缓存。
    int m_proactiveFrequency = 0;    ///< 主动对话频率缓存。
    double m_idleAmplitude = 1.0;    ///< 待机幅度缓存。
    bool m_autoMode = false;         ///< 自动模式缓存。
    bool m_edgeHide = true;          ///< 贴边隐藏缓存。
    bool m_clickThrough = true;      ///< 点击穿透缓存。
    bool m_idleMotion = true;        ///< 待机动作缓存。
    bool m_debugHitAreas = false;    ///< 命中区调试缓存。
    bool m_ballVisible = true;       ///< 悬浮球缓存。
    bool m_intimacyDecay = false;    ///< 好感度衰减缓存。
    bool m_autostart = false;        ///< 应用自启缓存。
    bool m_autoStartPet = false;     ///< 桌宠自启缓存。
    bool m_onboardingDone = false;   ///< 引导完成缓存。
    QString m_activePetCharacterId;  ///< 桌宠角色缓存。
};

#endif // APPVIEWMODEL_H
