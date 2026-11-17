# qml/ 新 UI（T3 交付说明）

本目录是 MVVM 重构后的 QML 视图层：主窗口是软件本体，桌宠为可选窗口。
契约唯一来源：`docs/架构重构-MVVM.md` §3 / §4 / §7（含 §7.14 集成补充）。
QML 只通过 context property 与 ViewModel 交互，不直接引用 `bridge`、文件或网络 API。

## 1. 文件树

```
qml/
├── MainWindow.qml               # 主窗口：侧栏 200 + 页面栈 + 状态栏 + Toast + 新手引导
├── theme/
│   ├── qmldir                   # 注册 Theme 单例（必须随 qrc 一起打包）
│   ├── Theme.qml                # QML singleton：唯一 token 入口（按 themeIndex 转发亮/暗）
│   ├── ThemeLight.qml           # 亮色 token 表
│   └── ThemeDark.qml            # 暗色 token 表（继承 ThemeLight，仅覆盖颜色）
├── components/
│   ├── Card.qml                 # 10 圆角卡片（标题/描述/默认内容插槽）
│   ├── CharacterCard.qml        # 角色卡片（形象/世界观/启用/使用/桌宠/重命名/导出/删除）
│   ├── PageHeader.qml           # 52 高页头（标题 + 描述 + 右侧操作插槽）
│   ├── SidebarItem.qml          # 28 高侧栏导航行（选中/ hover / 行尾徽章）
│   ├── PrimaryButton.qml        # 品牌青绿实心按钮（32/28）
│   ├── SecondaryButton.qml      # 描边/安静/危险次按钮
│   ├── IconButton.qml           # 28×28 图标按钮（可 fallbackText）
│   ├── TextFieldCard.qml        # 带标签的单行/多行输入（自绘占位 + 焦点环）
│   ├── EmptyState.qml           # 空态（图标框 + 标题 + 说明 + 描边按钮）
│   ├── StatusBar.qml            # 底部状态栏（appStatus / 桌宠状态点 / 版本）
│   ├── Toast.qml                # 轻提示（3.2s 淡出）
│   ├── Avatar.qml               # 头像/形象缩略图（路径解析 + 首字占位）
│   ├── Tag.qml                  # 18 高徽章（neutral/accent/success/warning/danger）
│   ├── ToggleSwitch.qml         # 30×18 开关（父级回写 VM）
│   ├── SegmentedControl.qml     # 36/28 分段控件（200ms 滑块）
│   ├── SelectBox.qml            # 下拉选择（自绘弹层）
│   ├── SliderField.qml          # 带数值的滑块字段
│   ├── FormRow.qml              # 设置表单行（120px 标签列）
│   ├── InputDialog.qml          # 文本输入对话框（单行/多行）
│   ├── PageScroll.qml           # 页面滚动容器（24/16/48 留白）
│   ├── Icon.qml                 # 线性图标（Shapes 描边 IconData 路径，按 token 着色）
│   └── IconData.js              # 图标路径数据（由 resources/icons/*.svg 生成）
├── pages/
│   ├── ChatPage.qml             # 专注聊天：760 居中、气泡/头像/时间/👍👎/回溯/卡片折叠/输入卡
│   ├── CharactersPage.qml       # 角色卡片网格 + 新建/导入形象/导入角色包/导出
│   ├── PersonasPage.qml         # 人设列表 + 名称/正文/预览/保存/删除
│   ├── WorldsPage.qml           # 世界观列表 + 编辑/预览/重命名/删除/导入
│   ├── MemoriesPage.qml         # 记忆添加/置顶/编辑/删除/立即抽取
│   ├── ModelsPage.qml           # 预设/URL/Key/模型/温度/maxTokens/多配置/测试
│   ├── RecordsPage.qml          # 聊天记录 + 赞踩 + 导出/清空
│   ├── DataPage.qml             # 数据/日志目录 + 占用 + 导出 + 引导重置
│   ├── SettingsPage.qml         # 外观/桌宠行为/模式/快捷键/更新/系统与数据
│   └── AboutPage.qml            # 关于（不在 0~8 页导航，主窗口本地 aboutVisible 维护）
├── pet/
│   ├── PetWindow.qml            # 桌宠窗口（petIndex 注入；拖拽三态/贴边/命中区调试/右键菜单）
│   ├── BubbleWindow.qml         # 气泡窗口（petVM.bubbleText/visible，点击打开聊天）
│   └── BallWindow.qml           # 悬浮球（appVM.ballVisible && !petVisible）
└── tools/
    └── gen_icon_data.py         # 从 resources/icons/*.svg 重新生成 components/IconData.js
```

> 旧版 `qml/Manager.qml`、`qml/Chat.qml`、`qml/Pet.qml`、`qml/Bubble.qml`、`qml/Ball.qml`
> 已删除（功能迁移到 MainWindow/pages/pet）。**不再有独立桌面 Chat 窗口。**

## 2. Lead 集成清单（必须）

1. **CMake `qml_sources` 文件列表**改为本目录全部 `.qml` 文件，并额外加入：
   - `qml/theme/qmldir`（Theme singleton 注册，缺了会导致所有页面报 `Theme is not a type`）
   - `qml/components/IconData.js`（Icon 的 JS 数据，必须进 qrc）
   - 建议保留 `PREFIX "/qml" BASE "qml"`，入口 `qrc:/qml/MainWindow.qml`。
2. **图标资源**：运行时图标来自 `qml/components/IconData.js`（已内嵌路径数据），
   `resources/icons/*.svg` 仅作为再生成源与许可留存，可不进 qrc；如保留请连同
   `resources/icons/LICENSE` 一起打包。`resources/pet.png` 仍需打包（`qrc:/pet.png` 为桌宠/悬浮球兜底图）。
3. **context property 注册顺序**：先创建 11 个 ViewModel 与 DialogService，并
   `setContextProperty`（`appVM/chatVM/charactersVM/personasVM/worldsVM/memoriesVM/modelsVM/recordsVM/dataVM/petVM/dialogs`），
   **再** load `MainWindow.qml`（Theme 单例初始化时会读 `appVM.themeIndex`）。
4. **窗口加载**：
   - 主窗口：`qrc:/qml/MainWindow.qml`，`objectName = "mainWindow"`（默认 `visible: true`；
     如需启动隐藏，请在创建后 `setVisible(false)`，并注意 QML 里 `onClosing` 会调用
     `appVM.hideMainWindow()` 隐藏窗口）。
   - 桌宠：`qrc:/qml/pet/PetWindow.qml` 每只实例一份，创建后注入 `petIndex`（0-based），
     `objectName` 规则：`petWindow`、`petWindow1`、`petWindow2`。
   - 气泡：`qrc:/qml/pet/BubbleWindow.qml`（`objectName = "bubbleWindow"`）。
   - 悬浮球：`qrc:/qml/pet/BallWindow.qml`（`objectName = "ballWindow"`）。
   - 删除旧 `Chat`/`Manager` 的加载；托盘/快捷键「打开聊天」= 显示主窗口 + `appVM.setPageIndex(0)`。
5. **点击穿透/命中区遮罩**仍由 C++ `petwindowhelper` 按 alpha 生成窗口区域；
   PetWindow 未设置 `WindowTransparentForInput`（否则无法点击桌宠）。
6. **QML 模块部署**：`windeployqt` 已配置扫描 `qml/`（`cxx_pkg.ini [qt] qmldir`），
   会带上 `QtQuick.Shapes`（Icon 使用）与 `QtQuick.Controls` 等；无需额外链接。
7. 控件样式：组件全部自绘背景/文字，不依赖 Material 的浮动 label；
   `QT_QUICK_CONTROLS_STYLE` 保持 Material 或 Basic 均可。

## 3. 页面 ↔ ViewModel 绑定清单

| 页面 | 数据属性（只读绑定） | 命令（Q_INVOKABLE） |
| --- | --- | --- |
| ChatPage | `messages`（id,role,text,feedback,card,cardTitle,cardBody,expanded,time）、`streaming`、`chatTitle`、`modelSummary`、`llmReady`、`welcomeText`、`quickPrompts`、`intimacyScore`、`intimacyLevel`、`errorText`、`busy` | `send`、`retry`、`stop`、`restart`、`clear`、`backtrack(index)`、`setFeedback(index,±1)`、`toggleExpanded(index)`、`exportChat`、`openModelSettings` |
| CharactersPage | `characters`（id,name,avatar,worldId,worldName,enabled,current,isPet）、`emptyHint`、`busy` | `create(name)`、`importAvatar`、`importPack`、`exportPack(id)`、`rename(id,name)`、`remove(id)`、`select(id)`、`setEnabled(id,bool)`、`launchAsPet(id)`、`refresh` |
| PersonasPage | `personas`（id,name,current）、`currentName`、`editName`、`editBody`、`editing`、`preview`、`editId`、`intimacyLevel` | `createNew`、`loadFor(id)`、`save`、`importMd`、`select(id)`、`remove(id)`、`setEditName`、`setEditBody`、`togglePreview` |
| WorldsPage | `worlds`（id,name,bodyPreview,characterCount）、`editId/editName/editBody`、`editing`、`preview` | `createNew`、`loadFor(id)`、`save`、`importFile`、`rename(id,name)`、`remove(id)`、`togglePreview`、`setEditName`、`setEditBody` |
| MemoriesPage | `memories`（id,content,pinned,createdAt）、`busy`、`count` | `add(content)`、`update(id,content)`、`remove(id)`、`togglePin(id)`、`refresh`、`extractNow` |
| ModelsPage | `presetIndex`、`baseUrl`、`apiKey`、`model`、`temperature`、`maxTokens`、`profiles`（index,name,model,active）、`llmReady`、`testResult`、`testing` | `setPresetIndex`、`setBaseUrl`、`setApiKey`、`setModel`、`setTemperature`、`setMaxTokens`、`testConnection`、`save`、`saveAsProfile(name)`、`applyProfile(index)`、`deleteProfile(index)`、`refresh` |
| RecordsPage | `records`（id,role,text,feedback,time）、`feedbackSummary`、`busy` | `refresh`、`exportChat`、`clearRecords` |
| DataPage | `dataPath`、`logPath`、`dataSize`、`lastExportPath`、`busy` | `exportChats`、`exportLogs`、`openDataDir`、`openLogsDir`、`resetOnboarding`、`checkUpdate` |
| SettingsPage | `appVM`：`themeIndex`、`languageIndex`、`petVisible`、`petCount`、`activePetCharacterId`、`autoStartPet`、`autoMode`、`mode`、`edgeHide`、`clickThrough`、`idleMotion`、`idleAmplitude`、`debugHitAreas`、`ballVisible`、`intimacyDecay`、`autostart`、`proactiveFrequency`、`petStatusText`、`updateStatus`、`updateUrl`、`appVersion`；`charactersVM.characters`（桌宠角色下拉） | appVM 对应 `setXxx`、`launchPet`、`stopPet`、`restartApp`、`openDataDir`、`openLogsDir`、`exportLogs`、`checkUpdate`、`setOnboardingDone` |
| AboutPage | `appVersion`、`updateStatus`、`updateUrl` | `checkUpdate`、`openDataDir`、`openLogsDir` |
| MainWindow | `pageIndex`、`appVersion`、`appStatus`、`petVisible`、`petStatusText`、`onboardingDone`、`themeIndex`（经 Theme）；信号 `toast`、`statusMessage`、`updateAvailable`；`hideMainWindow()` | `setPageIndex`、`setThemeIndex`、`setLanguageIndex`、`setPetVisible`、`hideMainWindow`、`setOnboardingDone` |
| ChatPage（补充） | — | `dialogs.confirm(title,text)` 用于清空/重启/回溯确认 |
| PetWindow | `pets[petIndex]`、`petCount`、`idleMotion`、`idleAmplitude`、`edgeHide`、`debugHitAreas`、`appVM.petVisible`、`appVM.mode` | `movePet`、`tapPet`、`doubleTapPet`、`contextMenu`、`dragFinished`；可选 `hitAreaRects(index)` |
| BubbleWindow | `bubbleText`、`bubbleVisible`、`pets`、`activePetIndex` | `hideBubble`；点击时 `appVM.setPageIndex(0)` + `appVM.showMainWindow()` |
| BallWindow | `appVM.ballVisible`、`appVM.petVisible`、`pets`、`activePetIndex` | `appVM.setPetVisible(true)` |

交互约束已覆盖：聊天 Enter 发送 / Shift+Enter 换行、流式显示停止、失败显示重试、
空会话 quickPrompts chips、```card 折叠、👍👎、回溯；桌宠拖拽三态、贴边隐藏（顶部不隐藏）、
点击穿透（C++）、命中区调试（可选方法）、右键菜单、双击打开聊天、悬浮球。

## 4. 契约缺项 / 需要 Lead 裁决

1. **命中区调试数据**：契约 §7.11 未提供命中区矩形。PetWindow 以可选方式调用
   `petVM.hitAreaRects(int index)`（返回 `[{id,x,y,w,h}]` 归一化矩形）；
   若 ViewModel 不提供，则 `debugHitAreas` 打开时无叠加层（功能降级，不报错）。
   建议 PetViewModel 补一个 `Q_INVOKABLE QVariantList hitAreaRects(int)`（或属性化）。
2. **模型预设名称**：`ModelsPage` 用本地 `presetLabels = ["Ollama","LM Studio","DeepSeek","OpenAI","OpenCode Go","自定义"]`，
   顺序需与 `ConfigStore`/`ModelSettingsViewModel` 的 `presetIndex` 语义一致；
   若 VM 暴露 `QStringList presets`，可改为绑定。
3. **悬浮球位置持久化**：旧 `Bridge::setBallPosition` 在契约中无替代，BallWindow 位置仅本次会话有效
   （默认主屏右下角）。如需持久化，请在 AppViewModel/PetViewModel 补 `ballX/ballY` 或 `setBallPosition`。
4. **世界观「绑定到当前角色」**：旧 Manager 有 `assignCharacterWorld`，新契约 §7.6 无对应命令；
   当前 WorldsPage 未提供该入口（角色绑定请在角色侧/后续补命令）。
5. **角色「导入形象」目标**：`charactersVM.importAvatar()` 无 id 参数，CharactersPage 放在页头，
   约定作用于当前选中角色。
6. **右键菜单**：QML 调用 `petVM.contextMenu(index)`；`appVM.showPetMenu(int)` 保留给 C++ 托盘/其它入口，
   `appVM.petMenuRequested(int)` 由 C++ 弹 QMenu，QML 不弹 QWidget 菜单。
7. **跟随系统主题**：Theme 以 `SystemPalette.window` 明度判断系统深浅；若 AppViewModel 已把
   `themeIndex` 解析为 0/1 也可，Theme 两种都兼容。
8. **关于页导航**：关于页不在契约 0~8 页中，由 MainWindow 本地 `aboutVisible` + StackLayout index 9 维护，
   不写入 `appVM.pageIndex`；入口在侧栏底部 info 按钮与设置页「关于本软件」。
9. **导出/导入反馈**：QML 监听 `appVM.toast(QString)` / `statusMessage(QString)` 显示轻提示；
   若各 ViewModel 只写自己的 `statusText`，需要 Application 转发到 AppViewModel 才有反馈。
10. **语言切换**：QML 只调用 `appVM.setLanguageIndex(i)`，并弹 `dialogs.confirm` 询问是否
    `restartApp()`；如 C++ 改为即时切换可删掉该确认。

## 5. 已验证

- `qmllint`（Qt 6.10）：全部文件 0 error（剩余 warning 均为 context property/委托 modelData 的
  unqualified 提示，属预期）。
- 离屏 QML 运行时（stub ViewModel，非 Windows 构建）：主窗口 10 页、亮/暗主题、空会话、流式+错误、
  桌宠窗口 + 命中区叠加、气泡、悬浮球、输入对话框、新手引导均成功加载并截图，无运行时错误。
