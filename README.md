# 桌面桌宠（desktop_companion）

Windows 桌面 AI 桌宠原型。**0.9.3：模型配置多套切换 + 卡片回复 + 前台应用自动模式**。
Qt 6.12 + QML + C++23（MSYS2 ucrt64，g++ 16），14 组单元测试。

## 0.6 已实现

- 透明、无边框、置顶桌宠窗口（占位形象 `resources/pet.png`），空闲时上下漂浮
- **角色库**：创建/重命名/删除角色，默认角色小柚；角色形象导入（PNG/JPG）
- **多套人设**：frontmatter 编辑（称呼/生日/星座/爱好/食物/背景 + 正文），导入 .md，切换即时生效
- **世界观**：新建/导入/编辑正文/重命名/删除，绑定到角色后注入提示词
- **角色包**：zip 导入导出（人设 + 形象 + meta），自研 ZIP store 实现
- **聊天/日志导出**：聊天记录导出 Markdown，日志目录导出到指定文件夹
- **错误码体系**：0x0001~0x0009 统一错误对象，界面提示带错误码与原因
- **记忆系统**：SQLite memory.db，按角色+人设分区；自动抽取（模型 + 本地句式兜底）、
  pin、增删改、上限 200、关键词+新鲜度检索注入提示词；回溯/重启时重置关联记忆
- **好感度**：0~100 四档（陌生/熟悉/亲近/信赖），对话 +2，事件记录，跨天衰减默认关闭
- **重启对话 / 消息回溯**：聊天窗「重启对话」；右键气泡「回溯到此处」，二次确认
- **主动对话**：频率关/低/中/高，空闲触发本地台词气泡，不打断输入
- **透明点击穿透**：按形象 alpha 生成窗口区域（SetWindowRgn），透明区域不挡桌面操作；
  可在设置/右键菜单关闭
- **meta 命中区**：meta.json hitAreas（归一化矩形 + 台词）决定点击区域与随机台词；
  缺失时回退头/身/脚比例分区；角色包数组写法与内部对象写法互相兼容
- **拖拽三态**：拖动时倾斜 + 放大，松手回正；双击打开聊天
- **待机浮动**：开/关 + 幅度滑杆（0~3）；命中区调试叠加显示
- 右键菜单：打开聊天 / 切换角色 / 设置 / 点击穿透开关 / 隐藏 / 退出
- **多桌宠**：1~3 只，独立位置与角色（右键「切换角色」绑定该只），位置持久化
- **贴边隐藏**：拖到左右屏幕边缘自动收起为细条，鼠标移入滑出；顶部不隐藏
- **全局快捷键**：Ctrl+Alt+P 呼出/隐藏桌宠、Ctrl+Alt+C 打开聊天、Ctrl+Alt+M 勿扰
- **模式**：休闲 / 工作 / 勿扰（勿扰暂停主动互动与待机动效）
- **悬浮球**：隐藏桌宠后显示小圆球，可拖动、点击恢复桌宠
- **新手引导**：首次启动欢迎弹窗；控制中心可重新打开
- **检查更新**：读取 config.json updateUrl 的版本清单（version/url/notes），与当前版本比较
- **主题**：浅色 / 深色 / 跟随系统（Material 风格，运行时切换，控制中心/聊天/气泡同步）
- **数据恢复**：config.json 损坏自动备份 `.bak` 并回退默认；chat.db 损坏备份后重建；
  chat.db 写入 schema 版本（PRAGMA user_version）
- **启动优化**：桌宠窗口按配置数量创建（1~3），不再固定创建 3 个
- **日志脱敏**：聊天内容只记录字符数，不写原文
- **多语言**：中文 / English / 跟随系统，控制中心-系统页切换（重启生效），
  QML 主界面通过 `qsTr` + `translations/desktop_companion_en.ts` 英文化
- **多套模型配置**：保存/应用/删除（每套 Key 独立 DPAPI 加密）
- **卡片式回复**：模型按约定输出 ```card 块，客户端折叠展示
- **前台应用自动模式**：工作应用自动切工作模式，其余切休闲
- **验收与发布**：`docs/需求验收矩阵.md` 逐条对照方案设计；`docs/发布校验清单.md`
- 点击桌宠：按**命中区**（头 / 身体 / 脚）说不同台词（4.2 秒自动隐藏），桌宠弹跳反馈
- 拖拽：8px 阈值区分点击与拖拽，松手保存位置，下次启动恢复
- 右键菜单：打开聊天 / 设置… / 隐藏桌宠 / 退出；托盘菜单同步
- **真实模型对话**：OpenAI 兼容 `POST /chat/completions`，SSE 流式逐字显示
  - 预设：Ollama、LM Studio、DeepSeek、OpenAI、OpenCode Go、自定义
  - OpenCode Go 网关自动附加 `x-opencode-session` 会话头
  - 未配置模型时自动回退到本地 mock，开箱即用
  - 超时 / 401 / 404 / 网络错误映射为聊天窗内联错误
- **人设导入**：控制中心导入 `.md`（首行 `# 标题` 作为名称），注入 system prompt；
  切换人设会清空当前上下文；内置「小柚（内置）」兜底
- **控制中心**：模型设置（预设、Base URL、API Key、模型、测试连接）、人设管理、关于
- 聊天窗：人设/模型状态、清空对话、停止生成、mock 模式提示
- **单实例**：singleproc 锁 + 门铃；二次启动只通知主实例并弹气泡
- **日志**：stacklog，落 `%APPDATA%/desktop_companion/desktop_companion/logs/`
- **配置**：`config.json`（原子写，含版本号），存模型配置与人设选择
- **API Key 加密**：Windows DPAPI（当前用户）加密后存 `apiKeyProtected`，旧明文配置读取时自动迁移
- **聊天记录持久化**：SQLite（`chat.db`），启动自动恢复最近 20 条；清空对话删除当前会话；切人设自动开新会话
- **开机自启**：控制中心「关于」页开关，写 HKCU Run 项
- 验证模式：`--smoke <dir>` 抓各窗口；`--shot <dir>` 抓全屏；`--demo <dir>` 发消息等回复完成后抓窗口

## 依赖的仓库内基础库

| 目录 | 用途 | 来源 |
|---|---|---|
| `third_party/stacklog` | 日志（轮转、trace、Qt 日志接管） | 自研 stacklog，源码内嵌 |
| `third_party/singleproc` | 单实例锁 + 二次启动门铃 | 自研 singleproc，源码内嵌（含 Windows 后端） |

两者通过 `add_subdirectory` 引入，作为子项目时不生成安装与 CMake 包配置文件。

## 目录

```
CMakeLists.txt
ci.toml / standardize.toml / audit.toml / cxx_pkg.ini   # 三个工程管理工具配置
.clang-format / .clang-tidy
src/main.cpp             # 装配、托盘、单实例、日志、smoke/shot/demo 验证模式
src/bridge.{h,cpp}       # QML ↔ C++ 桥接（命令 / 属性 / 信号）
src/chatservice.{h,cpp}  # 对话调度：LLM 优先，未配置回退 mock，维护历史并落库
src/chatstore.{h,cpp}    # SQLite 聊天记录（会话 / 消息 / meta）
src/characterstore.{h,cpp} # 角色库（目录/卡/元数据/形象/多套人设）
src/worldstore.{h,cpp}   # 世界观 .md
src/personafile.{h,cpp}  # 人设 frontmatter 解析/渲染
src/characterpack.{h,cpp}# 角色包 zip（store 方式）导入导出
src/errorcodes.{h,cpp}   # 统一错误码
src/hitmask.{h,cpp}      # alpha 命中遮罩
src/hitareameta.{h,cpp}  # meta.json 命中区解析
src/memorystore.{h,cpp}  # 记忆库（0.5 接入对话）
src/promptbuilder.{h,cpp}# 提示词固定顺序组装（0.5 接入对话）
src/intimacyservice.{h,cpp} # 好感度（0.5 接入对话）
src/secretstore.{h,cpp}  # Windows DPAPI 加解密（API Key）
src/autostart.{h,cpp}    # HKCU Run 开机自启
src/llmclient.{h,cpp}    # OpenAI 兼容流式客户端 + /models 连通性检查
src/sseparser.{h,cpp}    # SSE 增量解析（可单测）
src/configstore.{h,cpp}  # config.json 原子读写
src/personastore.{h,cpp} # 人设扫描 / 导入 / 删除
src/chatengine.{h,cpp}   # 本地 mock 对话引擎
qml/Pet.qml              # 桌宠窗口
qml/Bubble.qml           # 气泡窗口（点击穿透）
qml/Chat.qml             # 聊天窗口
qml/Manager.qml          # 控制中心（模型 / 人设 / 关于）
resources/pet.png        # 占位形象（透明 PNG，240x320）
tests/                   # 14 组单测：对话/SSE/聊天库/配置/错误码/角色包/人设/角色库/世界观/
                         # 命中遮罩/命中区/记忆库/提示词/好感度
third_party/             # stacklog、singleproc 源码
scripts/                 # 占位图生成、本地 mock LLM 服务
docs/                    # 设计文档、竞品参考、截图
```

## 构建（Windows + MSYS2 ucrt64）

```bash
export PATH=/c/msys2/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys2/ucrt64
cmake --build build
ctest --test-dir build --output-on-failure
```

## 工程管理（三个工具）

```bash
# 构建 + 测试流水线（cxx_ci）
PYTHONPATH=<cxx_ci>/src python -m cxx_ci.ci_tool -c ci.toml

# 静态检查（cxx_audit，clang-tidy；cpd/asan/valgrind 按 Windows 环境关闭）
PYTHONPATH=<cxx_audit>/src python -m cpp_audit -c audit.toml tidy

# 打包：自包含 zip（cxx_pkg）
PYTHONPATH=<cxx_pkg>/src python -m cxx_pkg -c cxx_pkg.ini
```

打包配置里的 `[qt] qmldir = qml` 让 windeployqt 扫描 QML import，
把 QtQuick/Controls 等模块一起打进 zip（qrc 内嵌的 QML 无法被自动发现）。

## 运行与验证

```bash
build/desktop_companion.exe

# 各窗口截图
build/desktop_companion.exe --smoke C:/tmp/shots
# 全屏截图
build/desktop_companion.exe --shot C:/tmp/shots
# 发一条消息，等回复完成后截图
build/desktop_companion.exe --demo C:/tmp/shots
```

离线验证流式链路（无需联网）：

```bash
python scripts/mock_llm_server.py 8765
# 控制中心 Base URL 填 http://127.0.0.1:8765/v1，模型随意
```

## 数据位置

```
%APPDATA%/desktop_companion/desktop_companion/config.json        模型配置（Key 为 DPAPI 密文）、人设选择、桌宠位置
%APPDATA%/desktop_companion/desktop_companion/chat.db            SQLite 聊天记录（WAL）
%APPDATA%/desktop_companion/desktop_companion/characters/<id>/   角色卡、meta.json、avatars、personas
%APPDATA%/desktop_companion/desktop_companion/worlds/<id>/       世界观 world.md
%APPDATA%/desktop_companion/desktop_companion/memory.db           SQLite 记忆（按角色/人设分区）
%APPDATA%/desktop_companion/desktop_companion/intimacy/<id>.json  好感度与事件
%APPDATA%/desktop_companion/desktop_companion/personas/*.md      导入的人设
%APPDATA%/desktop_companion/desktop_companion/logs/              日志
%APPDATA%/desktop_companion/desktop_companion/runtime/           单实例锁与门铃
```

## 已知限制（0.9 原型）

- 英文覆盖 QML 主界面；C++ 文案与拼接文案仍为中文（1.0 补齐）
- 测试机启动 7~8s（受 VM GPU 抖动影响），控制中心页面未拆 Loader
- 命中区动作仅台词；meta 的 motions/scale 尚未驱动动画
- 记忆检索为关键词+新鲜度（非向量）；自动抽取在模型只返回推理时用本地句式兜底
- 好感度目前仅对话 +2，等级解锁台词、点击加成未做
- 无全局快捷键、模式切换、多语言、自动更新

## 下一步（1.0）

补齐验收矩阵中的 🟡/⬜ 高优项（C++ 文案英化、LLM 参数与多配置、卡片回复、
好感度等级解锁等），全量回归 + 最终打包验收。
