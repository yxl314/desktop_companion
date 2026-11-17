/**
 * \file src/app/application.h
 * \brief 应用外壳：组装 Model/ViewModel/View，管理窗口、托盘、快捷键与生命周期。
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QString>
#include <QVector>

class QGuiApplication;
class QQmlApplicationEngine;
class QSystemTrayIcon;
class QMenu;
class QTranslator;

class WindowRegistry;
class ConfigStore;
class PersonaStore;
class ChatStore;
class MemoryStore;
class CharacterStore;
class WorldStore;
class IntimacyService;
class ChatService;
class DialogService;
class AppViewModel;
class ChatViewModel;
class CharactersViewModel;
class PersonasViewModel;
class WorldsViewModel;
class MemoriesViewModel;
class ModelSettingsViewModel;
class RecordsViewModel;
class DataViewModel;
class PetViewModel;
class GlobalHotkey;
class ForegroundWatcher;

/**
 * \brief 应用外壳（View 层的 C++ 装配器）。
 *
 * 按经典 MVVM 组织：构造 Model（data/services/domain），再构造 ViewModel，
 * 把 ViewModel 注入 QML 上下文，最后由 WindowRegistry 加载 QML 窗口。
 * 本类只做装配与窗口/托盘/快捷键协调，不包含业务规则。
 */
class Application : public QObject
{
    Q_OBJECT

public:
    /**
     * \brief 构造应用外壳（不加载窗口）。
     * \param app Qt 应用对象。
     * \param parent 父对象。
     */
    explicit Application(QGuiApplication &app, QObject *parent = nullptr);
    ~Application() override;

    /**
     * \brief 初始化 Model/ViewModel、QML 上下文、窗口、托盘与快捷键。
     * \return 是否初始化成功。
     */
    bool initialize();

    /**
     * \brief 首屏显示：主窗口 + 按配置创建桌宠/气泡/悬浮球窗口。
     */
    void showFirstScreen();

    /// QML 引擎。
    QQmlApplicationEngine *engine() const { return m_engine; }
    /// 窗口注册表。
    WindowRegistry *windows() const { return m_windows; }

    /// 外壳 ViewModel。
    AppViewModel *appViewModel() const { return m_appViewModel; }
    /// 聊天 ViewModel。
    ChatViewModel *chatViewModel() const { return m_chatViewModel; }

    /**
     * \brief 把当前全部窗口截图保存到目录（验证用）。
     * \param dir 输出目录。
     * \param includeHidden 是否包含隐藏窗口。
     * \return 保存的张数。
     */
    int grabWindows(const QString &dir, bool includeHidden = false) const;

    /**
     * \brief 应用桌宠透明区域点击穿透遮罩（按当前形象 alpha）。
     */
    void applyPetMasks();

private:
    void setupServices();
    void setupViewModels();
    void setupContextProperties();
    void setupWindows();
    void setupTray();
    void setupHotkeys();
    void setupConnections();
    /// 初始化前台应用探测与自动模式联动。
    void setupForegroundWatcher();
    /**
     * \brief 弹出桌宠右键菜单（QWidget 菜单由 View 层负责）。
     * \param index 桌宠下标。
     */
    void showPetMenu(int index);

    QGuiApplication &m_app;                 ///< Qt 应用对象。
    QQmlApplicationEngine *m_engine = nullptr; ///< QML 引擎。
    WindowRegistry *m_windows = nullptr;    ///< 窗口注册表。

    ConfigStore *m_config = nullptr;        ///< 配置仓库。
    PersonaStore *m_personas = nullptr;     ///< 人设仓库。
    ChatStore *m_store = nullptr;           ///< 聊天仓库。
    MemoryStore *m_memory = nullptr;        ///< 记忆仓库。
    CharacterStore *m_characters = nullptr; ///< 角色仓库。
    WorldStore *m_worlds = nullptr;         ///< 世界观仓库。
    IntimacyService *m_intimacy = nullptr;  ///< 好感度。
    ChatService *m_chat = nullptr;          ///< 对话服务。
    DialogService *m_dialogs = nullptr;     ///< 对话框抽象。

    AppViewModel *m_appViewModel = nullptr;               ///< 外壳 ViewModel。
    ChatViewModel *m_chatViewModel = nullptr;             ///< 聊天 ViewModel。
    CharactersViewModel *m_charactersViewModel = nullptr; ///< 角色 ViewModel。
    PersonasViewModel *m_personasViewModel = nullptr;     ///< 人设 ViewModel。
    WorldsViewModel *m_worldsViewModel = nullptr;         ///< 世界观 ViewModel。
    MemoriesViewModel *m_memoriesViewModel = nullptr;     ///< 记忆 ViewModel。
    ModelSettingsViewModel *m_modelsViewModel = nullptr;  ///< 模型设置 ViewModel。
    RecordsViewModel *m_recordsViewModel = nullptr;       ///< 记录 ViewModel。
    DataViewModel *m_dataViewModel = nullptr;             ///< 数据 ViewModel。
    PetViewModel *m_petViewModel = nullptr;               ///< 桌宠 ViewModel。

    QSystemTrayIcon *m_tray = nullptr;      ///< 托盘图标。
    QMenu *m_trayMenu = nullptr;            ///< 托盘菜单。
    GlobalHotkey *m_hotkey = nullptr;       ///< 全局快捷键。
    ForegroundWatcher *m_foreground = nullptr; ///< 前台应用探测（自动模式）。
    QTranslator *m_translator = nullptr;    ///< 翻译器（可选，main 传入）。
};

#endif // APPLICATION_H
