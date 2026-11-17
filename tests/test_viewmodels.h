/**
 * \file tests/test_viewmodels.h
 * \brief ViewModel 层单元测试：状态基类、应用外壳、聊天、记忆、模型设置、角色、世界观与桌宠。
 *
 * 测试用真实 Store + ChatService（路径全部落在 QTemporaryDir 内），
 * 不依赖 QML 或窗口类，可用 QTEST_GUILESS_MAIN 直接运行。
 */

#ifndef TEST_VIEWMODELS_H
#define TEST_VIEWMODELS_H

#include <QObject>

#include "viewmodels/dialogservice.h"
#include "viewmodels/viewmodelbase.h"

/**
 * \brief 测试用的 ViewModelBase 子类：把受保护的状态设置入口暴露给测试。
 *
 * 只为验证基类的信号去重与 clearStatus() 复位语义，不含业务逻辑。
 */
class TestableViewModel : public ViewModelBase
{
    Q_OBJECT

public:
    /**
     * \brief 构造测试用 ViewModel。
     * \param parent 父对象。
     */
    explicit TestableViewModel(QObject *parent = nullptr)
        : ViewModelBase(parent)
    {
    }

    /// 写入状态文本（转发受保护的 setStatus）。
    void setStatusForTest(const QString &text) { setStatus(text); }
    /// 写入错误文本（转发受保护的 setError）。
    void setErrorForTest(const QString &text) { setError(text); }
    /// 写入忙碌标记（转发受保护的 setBusy）。
    void setBusyForTest(bool busy) { setBusy(busy); }
};

/**
 * \brief 测试用对话框服务：不弹真实窗口，由测试预置返回路径。
 */
class FakeDialogService : public DialogService
{
    Q_OBJECT

public:
    /// 构造测试用对话框服务。
    explicit FakeDialogService(QObject *parent = nullptr)
        : DialogService(parent)
    {
    }

    /// 预设的「打开文件」返回值。
    QString nextOpenFile;
    /// 预设的「保存文件」返回值。
    QString nextSaveFile;

    /// 返回 nextOpenFile。
    QString openFile(const QString &title, const QString &filter) override
    {
        Q_UNUSED(title);
        Q_UNUSED(filter);
        return nextOpenFile;
    }
    /// 返回 nextSaveFile。
    QString saveFile(const QString &title, const QString &filter,
                     const QString &defaultPath) override
    {
        Q_UNUSED(title);
        Q_UNUSED(filter);
        Q_UNUSED(defaultPath);
        return nextSaveFile;
    }
    /// 返回空串（测试不选目录）。
    QString openDirectory(const QString &title) override
    {
        Q_UNUSED(title);
        return QString();
    }
    /// 固定返回 true。
    bool confirm(const QString &title, const QString &text) override
    {
        Q_UNUSED(title);
        Q_UNUSED(text);
        return true;
    }
};

/**
 * \brief ViewModel 单元测试集合。
 */
class TestViewModels : public QObject
{
    Q_OBJECT

private slots:
    /// ViewModelBase：状态/错误写入、信号去重与 clearStatus 复位。
    void viewModelBaseStatusReset();
    /// AppViewModel：默认值（页面、主题、语言、桌宠数量、悬浮球可见性）。
    void appViewModelDefaults();
    /// AppViewModel：setPageIndex / setPetCount 越界钳制。
    void appViewModelPageIndexClamped();
    /// AppViewModel：主题/语言写入 ConfigStore 并落盘。
    void appViewModelThemeLanguagePersist();
    /// AppViewModel：悬浮球位置读写与持久化。
    void appViewModelBallPosition();
    /// AppViewModel：主窗口显隐请求信号。
    void appViewModelMainWindowSignals();
    /// ChatViewModel：空白输入报错。
    void chatViewModelRejectsEmptyInput();
    /// ChatViewModel：mock 流式后 messages 含 user/assistant 行。
    void chatViewModelMockStreaming();
    /// ChatViewModel：setFeedback 落盘与 toggleExpanded 切换。
    void chatViewModelFeedbackAndExpanded();
    /// MemoriesViewModel：add / update / togglePin / remove 与 count。
    void memoriesViewModelCrud();
    /// MemoriesViewModel：未选择角色时 add 报错。
    void memoriesViewModelRequiresCharacter();
    /// ModelSettingsViewModel：save 写入 ConfigStore 并落盘。
    void modelSettingsViewModelSave();
    /// ModelSettingsViewModel：save 丢失 temperature/maxTokens 的已知 BUG（QEXPECT_FAIL）。
    void modelSettingsViewModelSaveGenerationKnownBug();
    /// ModelSettingsViewModel：多配置保存/应用/删除。
    void modelSettingsViewModelProfiles();
    /// ModelSettingsViewModel：llmReady 计算。
    void modelSettingsViewModelLlmReady();
    /// CharactersViewModel：行字段、setEnabled 落 meta、select 更新 config。
    void charactersViewModelRowsAndSelection();
    /// CharactersViewModel：importPack 成功后未刷新角色列表的已知 BUG（QEXPECT_FAIL）。
    void charactersViewModelImportPackRefreshKnownBug();
    /// WorldsViewModel：createNew / save / assignToCurrent 后角色数变化。
    void worldsViewModelCreateSaveAssign();
    /// PetViewModel：pets 行数=petCount、movePet 持久化。
    void petViewModelRowsAndMove();
    /// PetViewModel：hitAreaRects 返回结构（无命中区时为空）。
    void petViewModelHitAreaRects();
};

#endif // TEST_VIEWMODELS_H
