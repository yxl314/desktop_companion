/**
 * \file tests/test_viewmodels.cpp
 * \brief ViewModel 层单元测试实现：用真实 Store + ChatService 覆盖 8 个 ViewModel。
 *
 * 所有数据目录都落在 QTemporaryDir 内；在禁止使用 /tmp 的机器上可设置
 * 环境变量 DSH_TEST_TMP 指向允许的临时目录（例如 work/tmp-tests）。
 */

#include "test_viewmodels.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariantMap>
#include <QtTest>

#include "data/characterstore.h"
#include "data/chatstore.h"
#include "data/configstore.h"
#include "data/intimacyrepository.h"
#include "data/memorystore.h"
#include "data/personastore.h"
#include "data/worldstore.h"
#include "domain/chatmessage.h"
#include "services/characterpack.h"
#include "services/chatservice.h"
#include "services/promptbuilder.h"
#include "viewmodels/appviewmodel.h"
#include "viewmodels/charactersviewmodel.h"
#include "viewmodels/chatviewmodel.h"
#include "viewmodels/dialogservice.h"
#include "viewmodels/memoriesviewmodel.h"
#include "viewmodels/modelsettingsviewmodel.h"
#include "viewmodels/petviewmodel.h"
#include "viewmodels/worldsviewmodel.h"

namespace
{
/// QTemporaryDir 模板：优先 DSH_TEST_TMP（wpx 上禁止 /tmp），否则交给系统默认。
QString tempTemplate()
{
    const QString base = qEnvironmentVariable("DSH_TEST_TMP");
    if (base.isEmpty())
        return QString();
    QDir().mkpath(base);
    return base + QStringLiteral("/dc-viewmodels-XXXXXX");
}

/// 写入测试文件（父目录自动创建）；失败返回 false。
bool writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(content) == content.size();
}

/**
 * \brief 测试环境：真实 Store + ChatService，全部落在同一个 QTemporaryDir 内。
 *
 * 构造顺序与 src/app/application.cpp 的 setupServices() 保持一致，
 * 避免测试与运行时行为分叉。
 */
class Env
{
public:
    Env()
        : tempDir(tempTemplate())
        , configStore(tempDir.path() + QStringLiteral("/config.json"))
        , personaStore(tempDir.path() + QStringLiteral("/personas"))
        , chatStore(tempDir.path() + QStringLiteral("/chat.db"))
        , memoryStore(tempDir.path() + QStringLiteral("/memory.db"))
        , characterStore(tempDir.path() + QStringLiteral("/data"))
        , worldStore(tempDir.path() + QStringLiteral("/data"))
        , intimacyService(tempDir.path() + QStringLiteral("/data/intimacy"))
        , chatService(&configStore, &personaStore, &chatStore, &memoryStore, &intimacyService)
    {
    }

    /// 临时目录是否可用。
    bool isValid() const { return tempDir.isValid(); }

    QTemporaryDir tempDir;      ///< 测试隔离目录。
    ConfigStore configStore;    ///< 应用配置。
    PersonaStore personaStore;  ///< 全局人设。
    ChatStore chatStore;        ///< 聊天记录。
    MemoryStore memoryStore;    ///< 记忆。
    CharacterStore characterStore; ///< 角色。
    WorldStore worldStore;      ///< 世界观。
    IntimacyService intimacyService; ///< 好感度。
    ChatService chatService;    ///< 对话服务（mock 引擎）。
};
} // namespace

void TestViewModels::viewModelBaseStatusReset()
{
    TestableViewModel vm;
    QSignalSpy statusSpy(&vm, &ViewModelBase::statusTextChanged);
    QSignalSpy errorSpy(&vm, &ViewModelBase::errorTextChanged);
    QSignalSpy busySpy(&vm, &ViewModelBase::busyChanged);

    QVERIFY(vm.statusText().isEmpty());
    QVERIFY(vm.errorText().isEmpty());
    QVERIFY(!vm.busy());

    vm.setStatusForTest(QStringLiteral("就绪"));
    vm.setErrorForTest(QStringLiteral("出错了"));
    vm.setBusyForTest(true);
    QCOMPARE(vm.statusText(), QStringLiteral("就绪"));
    QCOMPARE(vm.errorText(), QStringLiteral("出错了"));
    QVERIFY(vm.busy());
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(busySpy.count(), 1);

    // 相同值不重复发信号（与其它 VM 的属性去重约定一致）。
    vm.setStatusForTest(QStringLiteral("就绪"));
    vm.setErrorForTest(QStringLiteral("出错了"));
    vm.setBusyForTest(true);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(busySpy.count(), 1);

    // clearStatus 只复位状态与错误，不改 busy。
    vm.clearStatus();
    QVERIFY(vm.statusText().isEmpty());
    QVERIFY(vm.errorText().isEmpty());
    QVERIFY(vm.busy());
    QCOMPARE(statusSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 2);
}

void TestViewModels::appViewModelDefaults()
{
    Env env;
    QVERIFY(env.isValid());
    AppViewModel vm(&env.configStore);

    QCOMPARE(vm.pageIndex(), 0);
    QCOMPARE(vm.themeIndex(), 0);
    QCOMPARE(vm.languageIndex(), 2); // 默认跟随系统
    QCOMPARE(vm.petCount(), 1);
    QVERIFY(vm.petVisible());
    QVERIFY(vm.ballVisible());
    QVERIFY(qFuzzyCompare(vm.ballX(), 900.0)); // 未保存位置时的默认展示坐标
    QVERIFY(qFuzzyCompare(vm.ballY(), 600.0));
    QVERIFY(vm.idleMotion());
    QVERIFY(qFuzzyCompare(vm.idleAmplitude(), 1.0));
    QVERIFY(vm.edgeHide());
    QVERIFY(vm.clickThrough());
    QVERIFY(!vm.debugHitAreas());
    QVERIFY(!vm.autoMode());
    QCOMPARE(vm.mode(), 0);
    QCOMPARE(vm.proactiveFrequency(), 0);
    QVERIFY(!vm.petStatusText().isEmpty());
    QVERIFY(!vm.appVersion().isEmpty());
}

void TestViewModels::appViewModelPageIndexClamped()
{
    Env env;
    QVERIFY(env.isValid());
    AppViewModel vm(&env.configStore);

    vm.setPageIndex(5);
    QCOMPARE(vm.pageIndex(), 5);

    vm.setPageIndex(42);
    QCOMPARE(vm.pageIndex(), 8); // 夹到最后一页

    vm.setPageIndex(-3);
    QCOMPARE(vm.pageIndex(), 0); // 夹到第一页

    // 桌宠数量同样夹取 1~3。
    vm.setPetCount(9);
    QCOMPARE(vm.petCount(), 3);
    QCOMPARE(env.configStore.values().petCount, 3);
    vm.setPetCount(0);
    QCOMPARE(vm.petCount(), 1);
    QCOMPARE(env.configStore.values().petCount, 1);

    // 主动对话频率夹取 0~3。
    vm.setProactiveFrequency(99);
    QCOMPARE(vm.proactiveFrequency(), 3);
    vm.setProactiveFrequency(-7);
    QCOMPARE(vm.proactiveFrequency(), 0);
}

void TestViewModels::appViewModelThemeLanguagePersist()
{
    Env env;
    QVERIFY(env.isValid());
    AppViewModel vm(&env.configStore);

    vm.setThemeIndex(1);
    QCOMPARE(env.configStore.values().theme, 1);
    QCOMPARE(vm.themeIndex(), 1);

    vm.setLanguageIndex(1);
    QCOMPARE(env.configStore.values().language, 1);
    QCOMPARE(vm.languageIndex(), 1);

    // 越界输入按 0~2 夹取。
    vm.setThemeIndex(99);
    QCOMPARE(env.configStore.values().theme, 2);
    vm.setLanguageIndex(-5);
    QCOMPARE(env.configStore.values().language, 0);

    // 已落盘：新实例读回同样的主题/语言。
    ConfigStore reloaded(env.configStore.filePath());
    QCOMPARE(reloaded.values().theme, 2);
    QCOMPARE(reloaded.values().language, 0);
}

void TestViewModels::appViewModelBallPosition()
{
    Env env;
    QVERIFY(env.isValid());
    AppViewModel vm(&env.configStore);

    QSignalSpy positionSpy(&vm, &AppViewModel::ballPositionChanged);
    vm.setBallPosition(120.5, 340.25);
    QCOMPARE(positionSpy.count(), 1);
    QVERIFY(qFuzzyCompare(vm.ballX(), 120.5));
    QVERIFY(qFuzzyCompare(vm.ballY(), 340.25));
    QVERIFY(qFuzzyCompare(env.configStore.values().ballX, 120.5));
    QVERIFY(qFuzzyCompare(env.configStore.values().ballY, 340.25));

    ConfigStore reloaded(env.configStore.filePath());
    QVERIFY(qFuzzyCompare(reloaded.values().ballX, 120.5));
    QVERIFY(qFuzzyCompare(reloaded.values().ballY, 340.25));
    QCOMPARE(reloaded.values().ballVisible, vm.ballVisible()); // 显隐与位置互不影响
}

void TestViewModels::appViewModelMainWindowSignals()
{
    Env env;
    QVERIFY(env.isValid());
    AppViewModel vm(&env.configStore);

    QSignalSpy spy(&vm, &AppViewModel::mainWindowVisibilityRequested);

    vm.showMainWindow();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);

    vm.hideMainWindow();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), false);

    vm.toggleMainWindow();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);

    QSignalSpy menuSpy(&vm, &AppViewModel::petMenuRequested);
    vm.showPetMenu(2);
    QCOMPARE(menuSpy.count(), 1);
    QCOMPARE(menuSpy.takeFirst().at(0).toInt(), 2);
}

void TestViewModels::chatViewModelRejectsEmptyInput()
{
    Env env;
    QVERIFY(env.isValid());
    ChatViewModel vm(&env.chatService, &env.chatStore, &env.memoryStore, &env.intimacyService,
                     &env.configStore, &env.characterStore, &env.worldStore);

    QCOMPARE(vm.messages().size(), 0);

    vm.send(QStringLiteral("   "));
    QVERIFY(!vm.errorText().isEmpty());
    QCOMPARE(vm.messages().size(), 0);
    QVERIFY(!vm.streaming());

    vm.clearStatus();
    QVERIFY(vm.errorText().isEmpty());

    // 越界下标上的评价/展开请求应安全返回。
    vm.setFeedback(-1, 1);
    vm.setFeedback(99, 1);
    vm.toggleExpanded(99);
    QCOMPARE(vm.messages().size(), 0);
}

void TestViewModels::chatViewModelMockStreaming()
{
    Env env;
    QVERIFY(env.isValid());
    ChatViewModel vm(&env.chatService, &env.chatStore, &env.memoryStore, &env.intimacyService,
                     &env.configStore, &env.characterStore, &env.worldStore);

    QSignalSpy doneSpy(&env.chatService, &ChatService::done);
    vm.send(QStringLiteral("你好"));
    QVERIFY2(doneSpy.wait(5000), "mock 引擎未在 5 秒内完成回复");

    const QVariantList messages = vm.messages();
    QCOMPARE(messages.size(), 2);
    const QVariantMap userRow = messages.at(0).toMap();
    const QVariantMap assistantRow = messages.at(1).toMap();
    QCOMPARE(userRow.value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(userRow.value(QStringLiteral("text")).toString(), QStringLiteral("你好"));
    QCOMPARE(assistantRow.value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
    QVERIFY(!assistantRow.value(QStringLiteral("text")).toString().isEmpty());
    QVERIFY(assistantRow.value(QStringLiteral("id")).toLongLong() >= 0); // 已落盘
    QVERIFY(!assistantRow.value(QStringLiteral("time")).toString().isEmpty());
    QVERIFY(assistantRow.contains(QStringLiteral("card")));
    QVERIFY(!vm.streaming());
    QVERIFY(!vm.busy());
    QVERIFY(!vm.llmReady()); // 未配置模型 → 本地 mock
}

void TestViewModels::chatViewModelFeedbackAndExpanded()
{
    Env env;
    QVERIFY(env.isValid());
    ChatViewModel vm(&env.chatService, &env.chatStore, &env.memoryStore, &env.intimacyService,
                     &env.configStore, &env.characterStore, &env.worldStore);

    QSignalSpy doneSpy(&env.chatService, &ChatService::done);
    vm.send(QStringLiteral("你好"));
    QVERIFY2(doneSpy.wait(5000), "mock 引擎未在 5 秒内完成回复");
    QCOMPARE(vm.messages().size(), 2);

    const int assistantIndex = 1;
    const qint64 assistantId =
        vm.messages().at(assistantIndex).toMap().value(QStringLiteral("id")).toLongLong();
    QVERIFY(assistantId >= 0);

    // 评价写入 messages 行并落盘到 message_feedback。
    vm.setFeedback(assistantIndex, 1);
    QCOMPARE(vm.messages().at(assistantIndex).toMap().value(QStringLiteral("feedback")).toInt(), 1);
    bool storedFeedback = false;
    for (const StoredMessage &message : env.chatStore.recentMessages(10)) {
        if (message.id == assistantId && message.feedback == 1)
            storedFeedback = true;
    }
    QVERIFY(storedFeedback);

    vm.setFeedback(assistantIndex, -1);
    QCOMPARE(vm.messages().at(assistantIndex).toMap().value(QStringLiteral("feedback")).toInt(), -1);

    // 卡片展开状态在行内切换。
    QVERIFY(!vm.messages().at(0).toMap().value(QStringLiteral("expanded")).toBool());
    vm.toggleExpanded(0);
    QVERIFY(vm.messages().at(0).toMap().value(QStringLiteral("expanded")).toBool());
    vm.toggleExpanded(0);
    QVERIFY(!vm.messages().at(0).toMap().value(QStringLiteral("expanded")).toBool());
}

void TestViewModels::memoriesViewModelCrud()
{
    Env env;
    QVERIFY(env.isValid());

    PromptContext context;
    context.characterId = QStringLiteral("theo");
    env.chatService.setContext(QStringLiteral("character/theo/default"), QStringLiteral("theo"),
                               QString(), context);

    MemoriesViewModel vm(&env.memoryStore, &env.chatService);
    QCOMPARE(vm.count(), 0);
    QVERIFY(vm.memories().isEmpty());

    vm.add(QStringLiteral("用户喜欢猫"));
    QCOMPARE(vm.count(), 1);
    QCOMPARE(vm.memories().size(), 1);
    QVERIFY(!vm.statusText().isEmpty());

    const QVariantMap row = vm.memories().first().toMap();
    QVERIFY(row.value(QStringLiteral("content")).toString().contains(QStringLiteral("猫")));
    QVERIFY(!row.value(QStringLiteral("pinned")).toBool());
    QVERIFY(!row.value(QStringLiteral("createdAt")).toString().isEmpty());
    const QString memoryId = row.value(QStringLiteral("id")).toString();
    QVERIFY(!memoryId.isEmpty());

    vm.update(memoryId, QStringLiteral("用户喜欢狗"));
    QCOMPARE(vm.count(), 1);
    QCOMPARE(vm.memories().first().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("用户喜欢狗"));

    vm.togglePin(memoryId);
    QVERIFY(vm.memories().first().toMap().value(QStringLiteral("pinned")).toBool());

    vm.remove(memoryId);
    QCOMPARE(vm.count(), 0);
    QVERIFY(vm.memories().isEmpty());

    // 空白内容被拒绝。
    vm.add(QStringLiteral("  "));
    QVERIFY(!vm.errorText().isEmpty());
    QCOMPARE(vm.count(), 0);
}

void TestViewModels::memoriesViewModelRequiresCharacter()
{
    Env env;
    QVERIFY(env.isValid());

    // 未调用 ChatService::setContext：分区键为空，记忆页应提示先选角色。
    MemoriesViewModel vm(&env.memoryStore, &env.chatService);
    vm.add(QStringLiteral("用户喜欢猫"));
    QVERIFY(!vm.errorText().isEmpty());
    QCOMPARE(vm.count(), 0);
    QVERIFY(vm.memories().isEmpty());
}

void TestViewModels::modelSettingsViewModelSave()
{
    Env env;
    QVERIFY(env.isValid());
    ModelSettingsViewModel vm(&env.configStore, &env.chatService);

    // 默认只有模型名，没有 Base URL → 未就绪。
    QCOMPARE(vm.model(), QStringLiteral("qwen2.5:7b"));
    QVERIFY(vm.baseUrl().isEmpty());
    QVERIFY(!vm.llmReady());

    vm.setBaseUrl(QStringLiteral("http://127.0.0.1:11434/v1"));
    vm.setModel(QStringLiteral("qwen2.5:7b"));
    vm.save();

    const AppConfig &values = env.configStore.values();
    QCOMPARE(values.baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
    QCOMPARE(values.model, QStringLiteral("qwen2.5:7b"));
    QVERIFY(!vm.statusText().isEmpty());

    // 已落盘：新实例读回连接参数。
    QVERIFY(QFile::exists(env.configStore.filePath()));
    ConfigStore reloaded(env.configStore.filePath());
    QCOMPARE(reloaded.values().baseUrl, QStringLiteral("http://127.0.0.1:11434/v1"));
}

void TestViewModels::modelSettingsViewModelSaveGenerationKnownBug()
{
    // 已知 BUG（T8 测试发现，未改 src，仅记录）：
    // save() 先调用 ConfigStore::setLlm()，其 changed() 同步触发 reloadFromConfig()，
    // 把编辑中的 temperature / maxTokens 覆盖回旧值；随后 setGeneration() 落盘的是旧值。
    // 期望行为：save() 应保留用户在编辑区设置的温度与 maxTokens。
    Env env;
    QVERIFY(env.isValid());
    ModelSettingsViewModel vm(&env.configStore, &env.chatService);

    vm.setTemperature(0.5);
    vm.setMaxTokens(2048);
    vm.save();

    QVERIFY2(qFuzzyCompare(env.configStore.values().temperature, 0.5),
             "temperature 未按编辑值保存（VM BUG）");
    QCOMPARE(env.configStore.values().maxTokens, 2048);
}

void TestViewModels::modelSettingsViewModelProfiles()
{
    Env env;
    QVERIFY(env.isValid());
    ModelSettingsViewModel vm(&env.configStore, &env.chatService);

    vm.setBaseUrl(QStringLiteral("http://127.0.0.1:11434/v1"));
    vm.setModel(QStringLiteral("qwen2.5:7b"));
    vm.saveAsProfile(QStringLiteral("本地"));

    QCOMPARE(vm.profiles().size(), 1);
    QCOMPARE(vm.profiles().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("本地"));

    vm.saveAsProfile(QStringLiteral("云端"));
    QCOMPARE(vm.profiles().size(), 2);

    // 应用第 0 套配置：active 标记回到该行。
    vm.applyProfile(0);
    QCOMPARE(vm.profiles().size(), 2);
    QVERIFY(vm.profiles().at(0).toMap().value(QStringLiteral("active")).toBool());
    QVERIFY(!vm.profiles().at(1).toMap().value(QStringLiteral("active")).toBool());
    QCOMPARE(env.configStore.values().activeProfile, 0);

    // 删除第 0 套配置后只剩「云端」。
    vm.deleteProfile(0);
    QCOMPARE(vm.profiles().size(), 1);
    QCOMPARE(vm.profiles().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("云端"));

    // 空名称被拒绝。
    vm.saveAsProfile(QStringLiteral("  "));
    QVERIFY(!vm.errorText().isEmpty());
    QCOMPARE(env.configStore.values().modelProfiles.size(), 1);
}

void TestViewModels::modelSettingsViewModelLlmReady()
{
    Env env;
    QVERIFY(env.isValid());
    ModelSettingsViewModel vm(&env.configStore, &env.chatService);

    QVERIFY(!vm.llmReady());
    vm.setBaseUrl(QStringLiteral("https://api.deepseek.com/v1"));
    vm.setModel(QStringLiteral("deepseek-chat"));
    QVERIFY(vm.llmReady());

    // 清空任一项即未就绪。
    vm.setBaseUrl(QString());
    QVERIFY(!vm.llmReady());
    vm.setModel(QString());
    QVERIFY(!vm.llmReady());

    // 切换预设会填充默认地址/模型并重新就绪。
    vm.setPresetIndex(2); // DeepSeek
    QCOMPARE(vm.presetIndex(), 2);
    QVERIFY(!vm.baseUrl().isEmpty());
    QVERIFY(!vm.model().isEmpty());
    QVERIFY(vm.llmReady());

    // 预设下标夹取。
    vm.setPresetIndex(99);
    QCOMPARE(vm.presetIndex(), 5);
}

void TestViewModels::charactersViewModelRowsAndSelection()
{
    Env env;
    QVERIFY(env.isValid());
    DialogService dialogs;
    CharactersViewModel vm(&env.characterStore, &env.worldStore, &env.configStore, &dialogs);

    QVERIFY(vm.characters().isEmpty());
    QVERIFY(!vm.emptyHint().isEmpty());

    vm.create(QStringLiteral("阿桃"));
    QCOMPARE(vm.characters().size(), 1);
    QCOMPARE(env.configStore.values().characterId, QStringLiteral("阿桃"));

    QVariantMap row = vm.characters().first().toMap();
    QCOMPARE(row.value(QStringLiteral("id")).toString(), QStringLiteral("阿桃"));
    QCOMPARE(row.value(QStringLiteral("name")).toString(), QStringLiteral("阿桃"));
    QVERIFY(row.value(QStringLiteral("enabled")).toBool());
    QVERIFY(row.value(QStringLiteral("current")).toBool());
    QVERIFY(!row.value(QStringLiteral("isPet")).toBool());
    QVERIFY(row.contains(QStringLiteral("avatar")));
    QVERIFY(row.contains(QStringLiteral("worldId")));
    QVERIFY(row.contains(QStringLiteral("worldName")));
    QVERIFY(vm.emptyHint().isEmpty());

    vm.create(QStringLiteral("小柚"));
    QCOMPARE(vm.characters().size(), 2);

    QString currentId;
    for (const QVariant &value : vm.characters()) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("current")).toBool())
            currentId = item.value(QStringLiteral("id")).toString();
    }
    QCOMPARE(currentId, QStringLiteral("小柚"));

    // select 更新 ConfigStore 与 current 标记。
    vm.select(QStringLiteral("阿桃"));
    QCOMPARE(env.configStore.values().characterId, QStringLiteral("阿桃"));
    currentId.clear();
    for (const QVariant &value : vm.characters()) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("current")).toBool())
            currentId = item.value(QStringLiteral("id")).toString();
    }
    QCOMPARE(currentId, QStringLiteral("阿桃"));

    // 不存在角色：报错且不改变当前角色。
    vm.select(QStringLiteral("不存在"));
    QVERIFY(!vm.errorText().isEmpty());
    QCOMPARE(env.configStore.values().characterId, QStringLiteral("阿桃"));

    // setEnabled 落 meta.json 并刷新列表行。
    vm.setEnabled(QStringLiteral("阿桃"), false);
    CharacterMeta meta;
    QString error;
    QVERIFY2(env.characterStore.meta(QStringLiteral("阿桃"), &meta, &error), qPrintable(error));
    QVERIFY(!meta.enabled);
    for (const QVariant &value : vm.characters()) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toString() == QStringLiteral("阿桃"))
            QVERIFY(!item.value(QStringLiteral("enabled")).toBool());
    }
}


void TestViewModels::charactersViewModelImportPackRefreshKnownBug()
{
    // 已知 BUG（T8 测试发现，未改 src，仅记录）：
    // CharactersViewModel::importPack() 的异步回调只设置 statusText，
    // 没有调用 refresh()；而 characterpack::importPack 直接写文件、
    // 不经过 CharacterStore，因此不会触发 CharacterStore::changed。
    // 结果：导入成功后角色列表要等下一次刷新才出现。
    Env env;
    QVERIFY(env.isValid());
    FakeDialogService dialogs;

    const QString sourceDir = env.tempDir.path() + QStringLiteral("/pack/yuki");
    QVERIFY(writeFile(sourceDir + QStringLiteral("/card.md"), QByteArray("# 小雪\n你是小雪。")));
    QVERIFY(writeFile(sourceDir + QStringLiteral("/avatars/yuki.png"), QByteArray("png")));
    const QString zipPath = env.tempDir.path() + QStringLiteral("/yuki-pack.zip");
    QString error;
    QVERIFY2(characterpack::exportPack(sourceDir, zipPath, &error), qPrintable(error));
    dialogs.nextOpenFile = zipPath;

    CharactersViewModel vm(&env.characterStore, &env.worldStore, &env.configStore, &dialogs);
    QCOMPARE(vm.characters().size(), 0);

    vm.importPack();
    QTRY_VERIFY_WITH_TIMEOUT(!vm.busy(), 5000);

    // 数据层确实落盘了新角色，且没有报错。
    QVERIFY(env.characterStore.contains(QStringLiteral("yuki")));
    QCOMPARE(vm.errorText(), QString());
    QVERIFY(!vm.statusText().isEmpty());

    QCOMPARE(vm.characters().size(), 1);
}

void TestViewModels::worldsViewModelCreateSaveAssign()
{
    Env env;
    QVERIFY(env.isValid());
    DialogService dialogs;

    QString characterId;
    QString error;
    QVERIFY2(env.characterStore.create(QStringLiteral("小柚"), &characterId, &error),
             qPrintable(error));
    env.configStore.setCharacterId(characterId);
    QVERIFY2(env.configStore.save(&error), qPrintable(error));

    WorldsViewModel vm(&env.worldStore, &env.characterStore, &dialogs, &env.configStore);
    QVERIFY(vm.worlds().isEmpty());

    vm.createNew();
    QVERIFY(vm.editing());
    QVERIFY(vm.editId().isEmpty());

    // 名称为空时保存应报错。
    vm.save();
    QVERIFY(!vm.errorText().isEmpty());
    QVERIFY(vm.worlds().isEmpty());

    vm.setEditName(QStringLiteral("云上小镇"));
    vm.setEditBody(QStringLiteral("一座漂浮在云上的小镇。"));
    vm.save();
    QVERIFY(!vm.editing());
    QCOMPARE(vm.worlds().size(), 1);

    const QVariantMap world = vm.worlds().first().toMap();
    const QString worldId = world.value(QStringLiteral("id")).toString();
    QVERIFY(!worldId.isEmpty());
    QCOMPARE(world.value(QStringLiteral("name")).toString(), QStringLiteral("云上小镇"));
    QVERIFY(world.value(QStringLiteral("bodyPreview")).toString().contains(QStringLiteral("云上")));
    QCOMPARE(world.value(QStringLiteral("characterCount")).toInt(), 0);

    // 把当前角色归属到该世界观：worlds 行角色数 +1，角色 meta 落盘。
    vm.assignToCurrent(worldId);
    QCOMPARE(env.characterStore.worldId(characterId), worldId);
    QCOMPARE(vm.worlds().first().toMap().value(QStringLiteral("characterCount")).toInt(), 1);

    // 取消归属：角色数回到 0。
    vm.assignToCurrent(QString());
    QVERIFY(env.characterStore.worldId(characterId).isEmpty());
    QCOMPARE(vm.worlds().first().toMap().value(QStringLiteral("characterCount")).toInt(), 0);
}

void TestViewModels::petViewModelRowsAndMove()
{
    Env env;
    QVERIFY(env.isValid());

    QString characterId;
    QString error;
    QVERIFY2(env.characterStore.create(QStringLiteral("小柚"), &characterId, &error),
             qPrintable(error));
    env.configStore.setCharacterId(characterId);
    env.configStore.setPetCount(2);
    env.configStore.setPetPosition(0, 10.0, 20.0);

    PetViewModel vm(&env.configStore, &env.characterStore, &env.intimacyService);
    QCOMPARE(vm.petCount(), 2);
    QCOMPARE(vm.pets().size(), 2);

    const QVariantMap first = vm.pets().first().toMap();
    QCOMPARE(first.value(QStringLiteral("index")).toInt(), 0);
    QCOMPARE(first.value(QStringLiteral("characterId")).toString(), characterId);
    QCOMPARE(first.value(QStringLiteral("name")).toString(), QStringLiteral("小柚"));
    QVERIFY(first.value(QStringLiteral("visible")).toBool());
    QVERIFY(first.contains(QStringLiteral("avatar")));
    QVERIFY(qFuzzyCompare(first.value(QStringLiteral("x")).toDouble(), 10.0));
    QVERIFY(qFuzzyCompare(first.value(QStringLiteral("y")).toDouble(), 20.0));
    QVERIFY(qFuzzyCompare(first.value(QStringLiteral("scale")).toDouble(), 1.0));

    // movePet 持久化到 config.json 并刷新 pets 行。
    vm.movePet(1, 123.5, 456.5);
    QCOMPARE(env.configStore.values().petPositions.size(), 3);
    QVERIFY(qFuzzyCompare(env.configStore.values().petPositions.at(1).x(), 123.5));
    QVERIFY(qFuzzyCompare(env.configStore.values().petPositions.at(1).y(), 456.5));
    QVERIFY(qFuzzyCompare(vm.pets().at(1).toMap().value(QStringLiteral("x")).toDouble(), 123.5));
    QVERIFY(QFile::exists(env.configStore.filePath()));

    // 越界下标不写入、不崩溃。
    vm.movePet(5, 1.0, 2.0);
    QVERIFY(qFuzzyCompare(env.configStore.values().petPositions.at(1).x(), 123.5));
}

void TestViewModels::petViewModelHitAreaRects()
{
    Env env;
    QVERIFY(env.isValid());

    QString characterId;
    QString error;
    QVERIFY2(env.characterStore.create(QStringLiteral("小柚"), &characterId, &error),
             qPrintable(error));
    env.configStore.setCharacterId(characterId);

    PetViewModel vm(&env.configStore, &env.characterStore, &env.intimacyService);

    // 没有 hitAreas 时返回空列表。
    QVERIFY(vm.hitAreaRects(0).isEmpty());
    QVERIFY(vm.hitAreaRects(99).isEmpty()); // 越界下标回退当前角色，仍为空

    // 写入一条命中区后返回 {id,x,y,w,h} 结构（归一化坐标）。
    CharacterMeta meta;
    QVERIFY2(env.characterStore.meta(characterId, &meta, &error), qPrintable(error));
    QJsonObject head;
    head.insert(QStringLiteral("id"), QStringLiteral("head"));
    head.insert(QStringLiteral("rect"), QJsonArray{0.1, 0.05, 0.8, 0.2});
    meta.hitAreas.insert(QStringLiteral("head"), head);
    QVERIFY2(env.characterStore.saveMeta(characterId, meta, &error), qPrintable(error));

    const QVariantList rects = vm.hitAreaRects(0);
    QCOMPARE(rects.size(), 1);
    const QVariantMap rect = rects.first().toMap();
    QCOMPARE(rect.value(QStringLiteral("id")).toString(), QStringLiteral("head"));
    QVERIFY(qFuzzyCompare(rect.value(QStringLiteral("x")).toDouble(), 0.1));
    QVERIFY(qFuzzyCompare(rect.value(QStringLiteral("y")).toDouble(), 0.05));
    QVERIFY(qFuzzyCompare(rect.value(QStringLiteral("w")).toDouble(), 0.8));
    QVERIFY(qFuzzyCompare(rect.value(QStringLiteral("h")).toDouble(), 0.2));

    // 显示气泡：4.2s 后自动隐藏由定时器负责，这里只验证立即可见。
    vm.showBubble(QStringLiteral("你好呀"));
    QVERIFY(vm.bubbleVisible());
    QCOMPARE(vm.bubbleText(), QStringLiteral("你好呀"));
    vm.hideBubble();
    QVERIFY(!vm.bubbleVisible());
}

QTEST_GUILESS_MAIN(TestViewModels)
