# UI 图标使用说明与资源清单（G-L-02 / G-H-10 配套）

> 本文是「次要按钮图标 + 悬浮说明（ToolTip）」需求的落地方案与资源清单。
> 图标组件与 QML 接线由 ui-redesign 负责（`qml/**`），T5 只提供资源清单与使用建议，
> 不修改任何 QML；出现视觉调整时以 QML 侧实现为准。

## 1. 现状（已具备的能力）

| 资产 | 位置 | 说明 |
| --- | --- | --- |
| 图标组件 | `qml/components/Icon.qml` | 用 `QtQuick.Shapes` 描边 SVG path，按 `Theme` 颜色着色；不依赖 QtSvg / shader，离屏与软件渲染可用 |
| 图标按钮 | `qml/components/IconButton.qml` | 28×28 / 32×32，16px 图标，hover 浅底；内置 `ToolTip`（延迟 450ms）；支持 `danger` 与 `fallbackText` |
| 图标路径数据 | `qml/components/IconData.js` | 由 `resources/icons/*.svg` 生成（lucide-static v0.544.0，ISC），`pathFor(name)` 返回 24×24 viewBox 的描边路径 |
| 图标源资产 | `resources/icons/*.svg`（35 个） | 设计源文件；运行时通过 `IconData.js` 绘制，**不需要**逐个打进 qrc |
| 许可 | `resources/icons/LICENSE` | lucide-static / ISC，已随仓库分发 |

> 集成状态：`qml/components/IconData.js` 已由 Lead 加入 `CMakeLists.txt` 的
> `qt_add_resources(desktop_companion "qml_sources" ... FILES)`（与
> `qml/components/Icon.qml` 同级），运行期 `Icon` 可正常解析路径数据。
> `resources/icons/*.svg` 仅作源资产，不必加入 qrc。

## 2. 图标资源清单

图标名即 `Icon { name: "..." }` 与 `IconButton { iconName: "..." }` 使用的键。

| 图标名 | 语义 | 建议用途 | 当前是否已用 |
| --- | --- | --- | --- |
| `activity` | 活动 / 聊天 | 侧栏「聊天」导航、对话状态 | 未用（侧栏用文字） |
| `alert-triangle` | 警告 | 错误提示、危险操作确认 | 已用（状态提示） |
| `book-open` | 书 / 记忆 | 「记忆」页、提示词预览 | 已用 |
| `check` | 对勾 | 保存成功、启用状态 | 已用 |
| `chevron-down` | 下拉箭头 | 折叠面板、SelectBox | 已用 |
| `chevron-right` | 右箭头 | 列表项进入、展开 | 未用 |
| `cpu` | 芯片 / 模型 | 「模型」页、模型配置 | 未用（侧栏用文字） |
| `external-link` | 外链 | 打开数据/日志目录、官网、许可 | 已用 |
| `film` | 胶片 / 记录 | 「记录」页 | 已用 |
| `folder-open` | 文件夹 | 「数据」页、打开目录 | 已用 |
| `github` | GitHub | 关于页项目链接 | 未用（按需启用） |
| `image` | 单图 | 导入形象、头像占位 | 已用 |
| `images` | 多图 / 角色 | 「角色」页、角色卡 | 已用 |
| `info` | 信息 | 说明、ToolTip 图标、引导 | 已用 |
| `keyboard` | 键盘 | 快捷键说明 | 未用（可按需启用） |
| `maximize` / `minimize` | 最大化/最小化 | 窗口动作 | 未用 |
| `minus` | 减号 | 数量减、清除 | 未用 |
| `monitor` | 显示器 / 世界观 | 「世界观」页、系统语言 | 未用（侧栏用文字） |
| `moon` / `sun` | 月亮/太阳 | 深色/浅色主题切换 | 未用（当前用 `IconButton` + iconName 占位，可替换） |
| `panel-left-close` / `panel-left-open` | 侧栏收起/展开 | 主窗口侧栏折叠 | 未用（预留） |
| `pause` / `play` | 暂停/播放 | 桌宠待机动效、主动对话开关 | 未用（预留） |
| `plus` | 加号 | 新建角色/人设/世界观、添加记忆 | 已用 |
| `refresh-cw` | 刷新 | 各页刷新、重试、检查更新 | 已用（最高频） |
| `rotate-ccw` | 回退 | 回溯对话、重置 | 已用 |
| `settings` | 设置 | 「设置」页、配置 | 已用 |
| `sparkles` | 星光 / 人设 | 「人设」页、AI 生成 | 已用 |
| `square` | 方块 | 停止（停止生成/停止桌宠） | 未用（预留） |
| `trash-2` | 删除 | 删除角色/记忆/配置 | 已用 |
| `video` | 视频 | 预留（动效/媒体） | 未用 |
| `x` | 关闭 | 关闭弹窗、清除输入 | 未用（输入框可用） |
| `zap` | 闪电 | 主动对话、快捷操作 | 已用 |

## 3. 使用规则（G-L-02「重要按钮带文字，次要按钮图标 + 悬浮说明」）

1. **重要主操作保留文字按钮**：新建/导入角色包/发送/保存/导出/重启对话/开始使用等，
   使用 `PrimaryButton` / `SecondaryButton`，必须能脱离图标理解语义。
2. **次要、重复出现的操作使用图标按钮**：刷新、重命名、删除、回溯、展开/收起、
   打开目录、主题切换、语言切换、关于等，使用 `IconButton`。
3. **图标按钮必须提供 ToolTip**：
   - 一律写 `tooltip: qsTr("...")`，并且 ToolTip 文案要和动作语义一致（动词开头）。
   - `fallbackText` 仅作为图标缺失时的兜底（如 `"✎"`），不能替代 ToolTip。
   - 新增的 tooltip 文案也要进 `translations/desktop_companion_en.ts`（见 §5）。
4. **尺寸与配色**：图标 16px（`Theme.iconMd`），按钮 28px（`Theme.hControlSm`）或 32px；
   颜色统一取 `Theme.text1/text2/dangerText`，QML 不硬编码色值。
5. **可访问性**：纯图标按钮 > 图标 + tooltip；若语义不能用一个通用图标表达（如「启动为桌宠」），
   继续用文字按钮，不要为了「有图标」牺牲可辨识性。
6. **一致性**：同一动作在不同页面使用同一图标名（如刷新统一 `refresh-cw`，删除统一 `trash-2`）。

## 4. 建议补充的图标（候选，按需从 lucide 补）

当前 35 个图标已覆盖主要动作；若后续页面需要，建议优先补充（保持同一 24×24 描边风格）：

- `circle-stop` 或 `square`：停止生成 / 停止桌宠（`square` 已在清单内，但停止语义建议用圆角停止图标）；
- `panel-left`：侧栏折叠（与 `panel-left-open/close` 配套）；
- `volume-x` / `volume-2`：主动对话/提示音开关；
- `history`：记录页更贴切的语义图标（当前用 `film`）；
- `loader` / `refresh-cw`：加载中（`refresh-cw` 已可用，必要时加旋转动画）。

新增流程：从 <https://lucide.dev>（ISC）取 24×24 SVG → 放入 `resources/icons/` →
把 `d` path 合并进 `qml/components/IconData.js` 的 `pathData`（多段子路径用空格连接）→
QML 直接用 `name` / `iconName` 引用 → 在本文 §2 清单补一行。

## 5. 翻译与验收

- 图标按钮的 `tooltip` 使用 `qsTr()`；`translations/desktop_companion_en.ts` 由
  `python3 tools/gen_translations.py` 生成（当前脚本扫描 `qml/*.qml` + 各 C++ 上下文；
  `qml/pages/**`、`qml/components/**` 的 QML 文案需把脚本的 glob 扩展为
  `qml/**/*.qml` 后再生成，属 T6/UI 收尾项）。
- 验收口径：任意纯图标按钮都能在 hover 时看到文字说明；重要按钮仍带文字；
  图标在浅色/深色主题下均清晰可辨。
