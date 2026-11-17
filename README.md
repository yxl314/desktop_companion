# 桌面桌宠（desktop_companion）

Windows 桌面 AI 桌宠应用（1.1.0）：**主窗口是软件本体**，负责角色 / 人设 / 世界观 / 记忆 / 模型 /
专注聊天与桌宠管理；桌宠窗口可从主窗口启动，也可设为开机自动启动。

技术栈：Qt 6.12 + QML + C++23（MSYS2 ucrt64 / g++ 16）、CMake + Ninja、SQLite、DPAPI。
架构：**经典 MVVM + 模块化**（View → ViewModel → Model），详见
[`docs/架构重构-MVVM.md`](docs/架构重构-MVVM.md)。

## 架构

```
src/
├── app/           View 层 C++ 装配：Application / WindowRegistry（窗口、托盘、快捷键）
├── viewmodels/    ViewModel 层：App/Chat/Characters/Personas/Worlds/Memories/
│                  ModelSettings/Records/Data/Pet + ViewModelBase + DialogService
├── domain/        纯值类型：AppConfig、Character/Persona/World、StoredMessage、MemoryEntry、Intimacy
├── data/          持久化：config/chat/character/world/persona/memory/intimacy 仓库
├── services/      领域服务：LLM/SSE/ChatService/PromptBuilder/CharacterPack/PersonaFile/
│                  Autostart/ErrorCodes/SecretStore
├── platform/      OS 集成：全局快捷键、前台应用探测、alpha 命中、meta 命中区、窗口区域
└── main.cpp       入口：日志、单实例、翻译、CLI 验证模式、Application
qml/
├── MainWindow.qml 主窗口（1280×800，200px 侧栏 + 页面栈 + 状态栏）
├── theme/         设计 Token 单例（Theme.qml / ThemeLight / ThemeDark / qmldir）
├── components/    22 个可复用组件 + IconData.js（lucide 图标描边数据）
├── pages/         Chat/Characters/Personas/Worlds/Memories/Models/Records/Data/Settings/About
└── pet/           PetWindow / BubbleWindow / BallWindow（独立桌宠窗口）
```

规则：QML 只绑定 ViewModel（context property `appVM/chatVM/.../dialogs`），
不读文件、不发网络、不碰 SQLite；ViewModel 不 include QML/窗口类；
Model 层不感知界面。依赖单向向下。

## 主要功能

- **主窗口（专注聊天）**：760px 居中消息区、流式回复、停止/重试、推荐问句、
  ```card 折叠卡片、👍👎 本地评价、按索引回溯、重启/清空、导出。
- **角色**：角色库、形象导入、启用开关、当前角色、从角色卡直接「启动为桌宠」、
  角色包 zip 导入导出（异步）。
- **人设 / 世界观**：多套人设（默认卡 + 全局 .md）、Markdown 预览、世界观绑定当前角色。
- **记忆 / 好感度**：SQLite 分区存储、pin、增删改、上限 200、LLM 抽取 + 本地兜底；
  好感度 0~100 四档、对话 +2、点击 +1、跨天衰减开关。
- **模型**：OpenAI 兼容（Ollama / LM Studio / DeepSeek / OpenAI / OpenCode Go / 自定义）、
  连接测试、temperature / maxTokens、多套配置快速切换、Key DPAPI 加密。
- **桌宠**：透明点击穿透（alpha/meta 命中区）、拖拽、贴边隐藏、多桌宠 1~3、悬浮球、
  待机浮动、气泡台词、位置持久化；主窗口与托盘可启动/停止桌宠。
- **系统**：托盘、单实例（门铃唤醒主窗口）、开机自启、全局快捷键
  Ctrl+Alt+P/C/M、模式（休闲/工作/勿扰）、前台应用自动模式、主题浅/深/跟随、
  中英双语（QML + C++ 文案，切换重启生效）、检查更新、日志与数据导出、损坏自动恢复。
- **验证/发布**：`--smoke/--shot/--demo/--manager-page/--hide-pet`；
  cxx_ci / cxx_audit / cxx_pkg；需求矩阵与发布记录见 `docs/`。

## 构建

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys2/ucrt64
ninja -C build
ctest --test-dir build
```

运行：`build/desktop_companion.exe`（主窗口），桌宠从「设置→桌宠运行」或角色卡启动。
数据目录：`%APPDATA%/desktop_companion/desktop_companion/`
（`config.json`、`chat.db`、`memory.db`、`characters/`、`worlds/`、`personas/`、`intimacy/`、`logs/`）。

## 文档

- 架构与 MVVM 契约：`docs/架构重构-MVVM.md`
- 需求验收矩阵：`docs/需求验收矩阵.md`
- 发布记录：`docs/发布记录-1.1.md`
- UI 图标使用：`docs/UI图标使用.md`
- 发布校验清单：`docs/发布校验清单.md`

## 依赖的仓库内基础库

| 目录 | 用途 |
|---|---|
| `third_party/stacklog` | 日志（轮转、trace、Qt 日志接管） |
| `third_party/singleproc` | 单实例锁 + 二次启动门铃（Windows 文件锁 + 门铃） |

以 `add_subdirectory(... EXCLUDE_FROM_ALL)` 内嵌，保留各自 CMake/测试/许可。
