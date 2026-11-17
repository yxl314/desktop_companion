# 桌面桌宠（desktop_companion）

Windows 桌面 AI 桌宠原型。**0.2：真实模型对话（OpenAI 兼容流式）+ 人设 .md 导入 + 控制中心**。
Qt 6.12 + QML + C++23（MSYS2 ucrt64，g++ 16）。

## 0.2 已实现

- 透明、无边框、置顶桌宠窗口（占位形象 `resources/pet.png`），空闲时上下漂浮
- 点击桌宠：头顶气泡说随机台词（4.2 秒自动隐藏），桌宠弹跳反馈
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
src/chatservice.{h,cpp}  # 对话调度：LLM 优先，未配置回退 mock，维护历史
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
tests/                   # ChatEngine / SseParser / ConfigStore+PersonaStore 单测
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
%APPDATA%/desktop_companion/desktop_companion/config.json        模型配置、人设选择、桌宠位置
%APPDATA%/desktop_companion/desktop_companion/personas/*.md      导入的人设
%APPDATA%/desktop_companion/desktop_companion/logs/              日志
%APPDATA%/desktop_companion/desktop_companion/runtime/           单实例锁与门铃
```

## 已知限制（0.2 原型）

- 形象仍为占位图；命中区分区、点击穿透、贴边隐藏、悬浮球、多桌宠未做
- 对话历史仅内存保存（20 条），未接 SQLite；重启后清空
- 人设仅注入 system prompt，无记忆/好感度/角色包（zip/立绘/动作）
- 无快捷键、无自动更新、无多语言
- 云端 API Key 明文存 config.json（本地工具，后续接 DPAPI）

## 下一步（0.3）

SQLite 持久化（聊天记录 + 记忆）、命中区分区与拖拽动作、角色包导入导出、
快捷键与贴边隐藏、控制中心完善。
