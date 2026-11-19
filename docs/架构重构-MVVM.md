# desktop_companion MVVM 重构方案（1.1）

> 目标：把当前「以桌宠为软件本体」的原型重构为 **经典 MVVM + 模块化** 的桌面应用：
> 主窗口是软件本体（管理桌宠 / 模型 / 角色 / 专注聊天），桌宠是可从主窗口启动的可选窗口。
> 参考 `/home/honor/桌面/code/real_esrguisoft` 的分层、主题 Token、i18n 与组件化实践。

## 1. 分层与目录

```
src/
├── main.cpp                  # 仅 bootstrap：QApplication + Application
├── app/                      # View 层（C++ 窗口/托盘/热键装配，不含业务）
│   ├── application.{h,cpp}   # 引擎、窗口注册表、托盘、热键、单实例生命周期
│   ├── windowregistry.{h,cpp}# 创建/显示 MainWindow、PetWindow×N、Ball；二启激活
│   └── traycontroller.{h,cpp}
├── common/                   # 无业务、无 UI 的通用设施
│   ├── paths.{h,cpp}         # QStandardPaths 统一封装
│   └── version.h
├── domain/                   # Model：纯值类型（无 Qt GUI、无 IO）
│   ├── appconfig.h           # AppConfig / ModelProfile
│   ├── character.h           # Character / CharacterMeta / PersonaData / World
│   ├── chatmessage.h         # StoredMessage / Feedback
│   ├── memoryentry.h
│   └── intimacy.h            # IntimacyEvent / Level
├── data/                     # Model：持久化（只做存储，不含交互规则）
│   ├── configstore.{h,cpp}
│   ├── chatstore.{h,cpp}
│   ├── characterstore.{h,cpp}
│   ├── worldstore.{h,cpp}
│   ├── personastore.{h,cpp}
│   ├── memorystore.{h,cpp}
│   └── intimacyrepository.{h,cpp}   # 原 intimacyservice，改名避免 Service 语义混用
├── services/                 # Model：领域服务（模型调用、提示词、角色包、抽取）
│   ├── llmclient.{h,cpp}
│   ├── sseparser.{h,cpp}
│   ├── chatengine.{h,cpp}
│   ├── chatservice.{h,cpp}
│   ├── promptbuilder.{h,cpp}
│   ├── characterpack.{h,cpp}
│   ├── personafile.{h,cpp}
│   ├── autostart.{h,cpp}
│   ├── errorcodes.{h,cpp}
│   └── secretstore.{h,cpp}
├── platform/                 # 平台/基础设施
│   ├── globalhotkey.{h,cpp}
│   ├── foregroundwatcher.{h,cpp}
│   ├── hitmask.{h,cpp}
│   ├── hitareameta.{h,cpp}
│   ├── petwindowhelper.{h,cpp}
│   └── (third_party: stacklog, singleproc)
├── viewmodels/               # ViewModel 层（QObject + Q_PROPERTY + Q_INVOKABLE）
│   ├── viewmodelbase.{h,cpp} # statusText/error/busy 基类
│   ├── appviewmodel.{h,cpp}  # 导航、主题、语言、状态栏、窗口动作
│   ├── chatviewmodel.{h,cpp}
│   ├── charactersviewmodel.{h,cpp}
│   ├── personasviewmodel.{h,cpp}
│   ├── worldsviewmodel.{h,cpp}
│   ├── memoriesviewmodel.{h,cpp}
│   ├── modelsettingsviewmodel.{h,cpp}
│   ├── recordsviewmodel.{h,cpp}
│   ├── dataviewmodel.{h,cpp}
│   ├── settingsviewmodel.{h,cpp}   # 行为/外观/快捷键/更新/引导
│   ├── petviewmodel.{h,cpp}        # 桌宠布局/行为/点击意图
│   └── dialogservice.{h,cpp}       # 文件/目录选择与消息对话框抽象（可替换测试）
└── views/qml 由 qml/ 承担（见下）
```

**依赖方向（单向向下）**：`View(QML) → ViewModel → services/data → domain`；
`platform/common` 可被任意层依赖，不得反向依赖上层。ViewModel 不允许 include QML/窗口类；
服务层不允许直接引用 ViewModel。

## 2. 经典 MVVM 规则

1. **View（QML）**：只绑定 ViewModel 的 `Q_PROPERTY`，只调用 `Q_INVOKABLE` 命令；
   禁止读文件、发网络、拼 SQLite、直接引用服务单例。
2. **ViewModel**：每个窗口/页面一个 VM；持有服务与数据仓库引用；暴露：
   - `Q_PROPERTY` + `NOTIFY`（列表用 `QVariantList`，行数据用 `QVariantMap` 或 `QAbstractListModel` 子类）；
   - 命令（`Q_INVOKABLE`）负责动作与状态复位；
   - `statusText` / `errorText` / `busy` 反馈（继承 `ViewModelBase`）。
3. **Model**：`domain` 值类型 + `data` 仓库 + `services` 服务；不感知界面。
4. **单向数据流**：View → ViewModel → Service → 仓库；结果通过信号回到 ViewModel → 属性 NOTIFY → View 重绘。
5. **测试**：ViewModel 可脱离 QML 实例化测试（依赖注入仓库/服务），新增 `tests/test_viewmodels.*`。

## 3. 主窗口 UI（软件本体）

窗口 `MainWindow`（1280×800，最小 980×640）：左侧栏 200px + 右侧内容区。

- 侧栏：应用名 + 版本；导航「聊天 / 角色 / 人设 / 世界观 / 记忆 / 模型 / 记录 / 数据 / 设置」；
  底部：主题切换、语言、状态点（桌宠是否运行）。
- 顶栏（页内）：页面标题 + 描述 + 主操作按钮。
- **聊天页（专注模式）**：内容居中最大 760px；消息气泡（头像/角色名/时间/👍👎/回溯）；
  底部输入卡（多行、Enter 发送、Shift+Enter 换行、停止/重试、空会话推荐问句、卡片折叠）。
- **角色页**：卡片网格（形象缩略图/名字/世界观/启用/当前），操作「新建/导入形象/导入角色包/导出」；
  每张卡可「启动为桌宠」。
- **桌宠管理**（设置在「设置-行为」或角色页顶部）：桌宠数量 1~3、启动/停止桌宠、开机自动启动某角色、
  位置重置、命中区调试。
- **设置页**：外观（主题/语言）、行为（数量/贴边/穿透/待机/主动频率/模式/自动模式）、
  模型（预设/参数/多配置/测试）、快捷键说明、更新、数据（导出/日志/打开目录）。
- 桌宠窗口 `PetWindow.qml`、气泡、悬浮球保留；**删除独立桌面 Chat 窗口**，聊天统一在主窗口内。

## 4. 设计 Token（对齐 real_esrguisoft）

`qml/theme/Theme.qml`（QML singleton）+ `qml/theme/ThemeDark.qml`：
颜色（两套）：`bgApp #FFFFFF / #1B1B1F`、`bgSidebar #F5F5F7 / #151518`、
`bgSubtle #EFEFF2 / #26262B`、`bgCard #FFFFFF / #202024`、`border #E4E4E9 / #32323A`、
`text1 #18181B / #F4F4F5`、`text2 #52525B / #A1A1AA`、`text3 #65656E / #71717A`、
`accent #0F766E`（品牌青绿，替代参考的橙色）、`accentSoft #E7F6F4`、
`success #16A34A`、`warning #D97706`、`danger #DC2626`。
尺寸：圆角 6/10/14，控件高 32/28，侧栏 200，页面留白 24，间距 4/8/12/16/20/24，
字号 11/12/13/14/15/16/18。所有 QML 不硬编码色值，统一取 Theme。

## 5. 与现有需求的关系

- 所有既有能力保留：模型接入/流式/错误分类、角色/人设/世界观/角色包、记忆/好感度/主动对话、
  点击穿透/命中区/多桌宠/贴边/悬浮球、快捷键/模式/主题/多语言/更新、导出与恢复。
- 主窗口作为入口后，托盘/右键菜单/快捷键的「打开聊天」都改为打开主窗口聊天页；
  「显示/隐藏桌宠」仍作用于桌宠窗口。
- WLD-L-01「同世界观角色互知」在本次补齐：构建提示词时加入同世界观其他角色摘要（Wave 2 任务）。
- 末尾更新 `docs/需求验收矩阵.md`，逐项标注新架构下的实现位置。

## 6. 协作拆分（共享任务板）

| 任务 | 负责 | 写作用域 | 依赖 |
| --- | --- | --- | --- |
| T1 模块化目录迁移 + CMake + 服务层 Doxygen | mvvm-arch | src/**, tests/**, CMakeLists.txt | 无 |
| T2 ViewModel 层（12 个 VM + 基类 + 对话框抽象 + Doxygen） | mvvm-viewmodel | src/viewmodels/** | 契约（本文件 §2/§3），目录路径以本文件为准 |
| T3 新 UI（主窗口/页面/组件/Theme/Pet 窗口迁移，QML Doxygen 注释） | ui-redesign | qml/** | VM 契约 |
| T4 App Shell C++（application/windowregistry/tray/main.cpp 重写） | lead | src/app/**, src/main.cpp, CMakeLists.txt | T1/T2 完成 |
| T5 需求补齐（WLD-L-01、异步导入、图标+Tooltip、C++ 文案收尾、单测） | gaps-dev（Wave 2） | services/、data/、tests/** | T1 |
| T6 集成、Windows 构建、CI/audit/pkg、桌面包、honor 提交、验收矩阵 | lead | 全局 | 全部 |

**并行约束**：T1 独占 `CMakeLists.txt` 与 `src/**` 旧文件移动；T2 只新建 `src/viewmodels/**`，
不得修改旧文件；T3 只改 `qml/**`。Lead 在 T1/T2 完成后统一改 CMake 与 main.cpp。

## 7. ViewModel ↔ QML 接口契约（T2/T3 共用，必须逐字一致）

QML 通过 context property 访问：`appVM`、`chatVM`、`charactersVM`、`personasVM`、
`worldsVM`、`memoriesVM`、`modelsVM`、`recordsVM`、`dataVM`、`petVM`、`dialogs`。

### 7.1 基类 `ViewModelBase`
- 属性：`QString statusText`、`QString errorText`、`bool busy`（均有 NOTIFY）
- 命令：`Q_INVOKABLE void clearStatus()`
- 受保护：`void setStatus(const QString&)`、`void setError(const QString&)`、`void setBusy(bool)`

### 7.2 AppViewModel（外壳 + 系统设置 + 桌宠控制）
属性：
| 属性 | 类型 | 说明 |
| --- | --- | --- |
| pageIndex | int | 0聊天 1角色 2人设 3世界观 4记忆 5模型 6记录 7数据 8设置 |
| appVersion | QString | 只读 |
| appStatus | QString | 只读，状态栏文本 |
| themeIndex / languageIndex | int | 0浅1深2跟随 / 0中1英2系统 |
| petVisible | bool | 桌宠窗口总开关 |
| petCount | int | 1~3 |
| activePetCharacterId | QString | 自动/手动启动的桌宠角色 |
| autoStartPet | bool | 开机自动启动桌宠 |
| autoMode / mode | bool / int | 前台自动模式 / 0休闲1工作2勿扰 |
| edgeHide / clickThrough / idleMotion | bool | |
| idleAmplitude | double | 0.2~2.0 |
| debugHitAreas / ballVisible / intimacyDecay / autostart | bool | |
| proactiveFrequency | int | 0~3 |
| petStatusText | QString | 例如「桌宠运行中（1 只）」 |
| onboardingDone / updateStatus / updateUrl | bool / QString / QString | |

命令：`setPageIndex(int)`、`setThemeIndex(int)`、`setLanguageIndex(int)`、`setPetVisible(bool)`、
`setPetCount(int)`、`launchPet()`、`stopPet()`、`setActivePetCharacter(QString)`、
`setAutoStartPet(bool)`、`setAutoMode(bool)`、`setMode(int)`、`setEdgeHide(bool)`、`setClickThrough(bool)`、
`setIdleMotion(bool)`、`setIdleAmplitude(double)`、`setDebugHitAreas(bool)`、`setBallVisible(bool)`、
`setIntimacyDecay(bool)`、`setProactiveFrequency(int)`、`setAutostart(bool)`、`setUpdateUrl(QString)`、
`setOnboardingDone(bool)`、`checkUpdate()`、`restartApp()`、`openDataDir()`、`openLogsDir()`、
`exportLogs()`、`showPetMenu(int index)`。
信号：`statusMessage(QString)`、`updateAvailable(QString version)`、`toast(QString)`。

### 7.3 ChatViewModel
属性：`QVariantList messages`（行：id,role,text,feedback,card,cardTitle,cardBody,expanded,time）、
`bool streaming`、`QString chatTitle`、`QString modelSummary`、`bool llmReady`、
`QString welcomeText`、`QStringList quickPrompts`、`int intimacyScore`、`QString intimacyLevel`、
`QString errorText`、`bool busy`。
命令：`send(QString)`、`retry()`、`stop()`、`restart()`、`clear()`、`backtrack(int index)`、
`setFeedback(int index,int value)`、`toggleExpanded(int index)`、`exportChat()`、`openModelSettings()`。
`messages` 变化必须发 `messagesChanged`；流式增量由 VM 内部合并后节流发信号。

### 7.4 CharactersViewModel
属性：`QVariantList characters`（id,name,avatar,worldId,worldName,enabled,current,isPet）、
`QString emptyHint`、`bool busy`。
命令：`create(QString name)`、`importAvatar()`、`importPack()`、`exportPack(QString id)`、
`rename(QString id,QString name)`、`remove(QString id)`、`select(QString id)`、
`setEnabled(QString id,bool)`、`launchAsPet(QString id)`、`refresh()`。

### 7.5 PersonasViewModel
属性：`QVariantList personas`（id,name,current）、`QString currentName`、`QString editName`、
`QString editBody`、`bool editing`、`bool preview`、`QString editId`、`QString intimacyLevel`。
命令：`createNew()`、`loadFor(QString id)`、`save()`、`importMd()`、`select(QString id)`、
`remove(QString id)`、`setEditName(QString)`、`setEditBody(QString)`、`togglePreview()`。

### 7.6 WorldsViewModel
属性：`QVariantList worlds`（id,name,bodyPreview,characterCount）、`QString editId/editName/editBody`、
`bool editing`、`bool preview`。
命令：`createNew()`、`loadFor(QString id)`、`save()`、`importFile()`、`rename(QString id,QString)`、
`remove(QString id)`、`togglePreview()`、`setEditName(QString)`、`setEditBody(QString)`。

### 7.7 MemoriesViewModel
属性：`QVariantList memories`（id,content,pinned,createdAt）、`bool busy`、`int count`。
命令：`add(QString content)`、`update(QString id,QString content)`、`remove(QString id)`、
`togglePin(QString id)`、`refresh()`、`extractNow()`（异步调用 LLM 抽取）。

### 7.8 ModelSettingsViewModel
属性：`int presetIndex`、`QString baseUrl/apiKey/model`、`double temperature`、`int maxTokens`、
`QVariantList profiles`（index,name,model,active）、`bool llmReady`、`QString testResult`、
`bool testing`。
命令：`setPresetIndex(int)`、`setBaseUrl(QString)`、`setApiKey(QString)`、`setModel(QString)`、
`setTemperature(double)`、`setMaxTokens(int)`、`testConnection()`、`save()`、
`saveAsProfile(QString name)`、`applyProfile(int index)`、`deleteProfile(int index)`、`refresh()`。

### 7.9 RecordsViewModel
属性：`QVariantList records`（id,role,text,feedback,time）、`QString feedbackSummary`、`bool busy`。
命令：`refresh()`、`exportChat()`、`clearRecords()`。

### 7.10 DataViewModel
属性：`QString dataPath/logPath/dataSize`、`QString lastExportPath`、`bool busy`。
命令：`exportChats()`、`exportLogs()`、`openDataDir()`、`openLogsDir()`、`resetOnboarding()`、
`checkUpdate()`。

### 7.11 PetViewModel
属性：`QVariantList pets`（index,characterId,name,avatar,x,y,visible,scale）、`int petCount`、
`int activePetIndex`、`bool idleMotion`、`double idleAmplitude`、`bool clickThrough/edgeHide/debugHitAreas`。
命令：`movePet(int index,double x,double y)`、`tapPet(int index,double x,double y)`、
`doubleTapPet(int index)`、`contextMenu(int index)`、`dragFinished(int index)`。

### 7.12 DialogService
`Q_INVOKABLE QString openFile(QString title,QString filter)`、
`QStringList openFiles(...)`、`QString openDirectory(...)`、`QString saveFile(...)`、
`bool confirm(QString title,QString text)`、`void info(QString title,QString text)`。

### 7.13 窗口职责
- `MainWindow.qml` 唯一常驻主窗口；关闭 = 隐藏到托盘（`appVM.petVisible` 不影响主窗口）。
- `PetWindow.qml` 由 `appVM.petVisible` 驱动显示/隐藏；不再有独立 Chat 窗口。
- `BubbleWindow.qml`/`BallWindow.qml` 保留，由 appVM/petVM 状态驱动。

### 7.14 集成补充（Lead 裁决 1）

- `AppViewModel` 追加：`Q_INVOKABLE void showMainWindow()`、`hideMainWindow()`、
  `toggleMainWindow()`；信号 `mainWindowVisibilityRequested(bool visible)`（Application 连接 WindowRegistry）。
  可选构造参数 `DialogService *dialogs = nullptr`。
- `PetViewModel` 扩展：属性 `QString bubbleText`、`bool bubbleVisible`；
  命令 `showBubble(QString text)`、`hideBubble()`；信号 `characterActivated(QString id)`
  （Application 连接 CharactersViewModel::select）；构造
  `PetViewModel(ConfigStore*, CharacterStore*, IntimacyService*, QObject* = nullptr)`。
  点击逻辑：命中区台词（头/身/脚）+ 气泡 4.2s、好感度 +1、若 petCharacters[index] 与当前角色不同则发
  `characterActivated`（由 ViewModel 层切换，不直接操作窗口）。
- `ChatViewModel`：追加信号 `modelSettingsRequested()`（Application 切到模型页）。
  `AppViewModel`：追加信号 `petMenuRequested(int index)`（Application 用 QMenu 实现右键菜单，
  QML/A 层不弹 QWidget 菜单）。
- `MemoriesViewModel`：`extractNow()` 暂发 `extractRequested()` 信号；T5 提供
  `ChatService::extractMemoriesNow()` 后由 Application 接线。
- 导出类命令无 DialogService 时默认导出到 `<AppData>/exports/`，并写 `statusText`。

### 7.15 T5 补齐接口（集成接线清单）

T5 新增的公共接口均为 **增量**，未改变已有签名与行为语义；同步接口继续可用。

1. **WLD-L-01 同世界观互知**（`services/chatservice.h`）
   - `void ChatService::setWorldPeerSources(CharacterStore *characters, WorldStore *worlds = nullptr);`
   - 在 `Application::setupServices()` 创建 `m_chat` 后接线一行：
     ```cpp
     m_chat->setWorldPeerSources(m_characters, m_worlds); // WLD-L-01：注入同世界观伙伴
     ```
   - 每轮 `buildPrompt()` 按当前角色 `meta.json` 的 `worldId` 调
     `CharacterStore::worldPeerSummaries()`，结果写入 `PromptContext::worldPeers`；
     `PromptBuilder` 在世界观段之后追加「【同世界观的伙伴】」段。未接线时行为与 1.0 相同。
   - 数据来源：`CharacterStore::worldPeerSummaries(worldId, excludeId)`（只取 enabled 角色，
     摘要为「名字（一句话人设）」，一句话人设取 card.md 正文首行）。

2. **G-H-04 异步导入导出**（错误码/文案与同步版一致，UI 线程不冻结）
   - `characterpack::exportPackAsync(characterDir, zipPath)` → `QFuture<AsyncExportResult>`
   - `characterpack::importPackAsync(zipPath, charactersDir)` → `QFuture<AsyncImportResult>`
   - `CharacterStore::importAvatarAsync(id, sourcePath)` → `QFuture<AvatarImportResult>`
   - `PersonaStore::importFromAsync(filePath)` → `QFuture<PersonaImportResult>`
   - `WorldStore::importFromAsync(filePath)` → `QFuture<WorldImportResult>`
   - `parsePersonaFileAsync(filePath)` → `QFuture<PersonaParseResult>`
   - 实现：`QThreadPool` + `QPromise/QFuture`（QtCore），不新增 `Qt6::Concurrent` 链接依赖；
     QObject 类（CharacterStore/PersonaStore/WorldStore）成功后回到对象所属线程发 `changed()`。
   - 接线建议（CharactersViewModel / PersonasViewModel / WorldsViewModel）：
     调用异步入口 + `QFutureWatcher`（或 `.then()` 到界面线程）更新 `statusText/errorText` 并
     `refresh()`；同步入口保留给测试与无 UI 场景。

3. **G-H-10 / SYS-L-01 C++ 文案**
   - 翻译上下文：`errors`（errorcodes）、`MemoryStore`、`ChatStore`、`CharacterStore`、
     `WorldStore`、`PersonaStore`、`PersonaFile`、`IntimacyService`。
   - `tools/gen_translations.py` 的 `CPP_CONTEXTS` 已覆盖上述文件；重新生成：
     `python3 tools/gen_translations.py`（当前 ts：195 条，0 unfinished）。
   - 注意：脚本 QML 扫描仍是 `qml/*.qml`；新页面（`qml/pages/**`、`qml/components/**`）的
     `qsTr` 文案需在 T6 把 glob 扩为 `qml/**/*.qml` 后重新生成并补英文。

4. **G-L-02 图标 + Tooltip**
   - 方案与资源清单见 `docs/UI图标使用.md`；QML 由 ui-redesign 维护。
   - 集成状态：`qml/components/IconData.js` 已加入 `CMakeLists.txt` 的
     `qt_add_resources(desktop_companion "qml_sources" ... FILES ...)`（Lead，20:01），
     运行期图标可正常解析。
