# 桌面桌宠（desktop_companion）

最小原型：**桌宠互动 + mock 聊天 + 单实例 + 日志**。目标平台 Windows 10 / 11 x64；Qt 6.12 + QML + C++23（MSYS2 ucrt64，g++ 16）。

## 已实现

- 透明、无边框、置顶桌宠窗口（占位形象 `resources/pet.png`），空闲时上下漂浮
- 点击桌宠：头顶弹气泡说一句随机台词（4.2 秒自动隐藏），桌宠弹跳反馈
- 拖拽：8px 阈值区分点击与拖拽，松手保存位置，下次启动恢复
- 右键菜单：打开聊天 / 隐藏桌宠 / 退出
- 系统托盘图标与菜单（显示隐藏、打开聊天、退出）
- 聊天窗：消息气泡（用户右绿 / 小柚左白）、Enter 发送、Shift+Enter 换行
- mock 对话引擎：关键词匹配 + 350ms「思考中」+ 逐字流式输出，不联网
- **单实例**：基于自研 `singleproc`，第二次启动只发门铃通知并退出，主实例弹气泡提示
- **日志**：基于自研 `stacklog`，日志落在 `%APPDATA%/desktop_companion/desktop_companion/logs/`
- 验证模式：`--smoke <dir>` 自动开聊天、发消息并截图各窗口；`--shot <dir>` 截全屏

## 依赖的仓库内基础库

| 目录 | 用途 | 来源 |
|---|---|---|
| `third_party/stacklog` | 日志（轮转、trace、Qt 日志接管） | 自研 stacklog 项目，源码内嵌 |
| `third_party/singleproc` | 单实例锁 + 二次启动门铃 | 自研 singleproc 项目，源码内嵌（含 Windows 后端） |

两者均通过 `add_subdirectory` 引入，作为子项目时不生成安装与 CMake 包配置文件。

## 目录

```
CMakeLists.txt
ci.toml / standardize.toml / audit.toml / cxx_pkg.ini   # 三个工程管理工具的配置
.clang-format / .clang-tidy
src/main.cpp            # 装配、托盘、单实例、日志、验证模式
src/bridge.{h,cpp}      # QML ↔ C++ 桥接（命令 Q_INVOKABLE / 事件信号）
src/chatengine.{h,cpp}  # 本地 mock 对话引擎
qml/Pet.qml             # 桌宠窗口（拖动、点击、漂浮）
qml/Bubble.qml          # 气泡窗口（点击穿透）
qml/Chat.qml            # 聊天窗口
resources/pet.png       # 占位形象（透明 PNG，240x320）
tests/test_chatengine.* # ChatEngine 单元测试（Qt Test）
third_party/            # stacklog、singleproc 源码
docs/                   # 设计文档、竞品参考、截图
scripts/gen_placeholder.py
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

打包配置里的 `[qt] qmldir = qml` 让 windeployqt 扫描仓库内 QML 的 import，
把 QtQuick/Controls 等模块一起打进 zip（内嵌 qrc 的 QML 无法被 windeployqt 自动发现）。

## 运行

```bash
build/desktop_companion.exe
```

## 验证

```bash
# 各窗口截图（自动打开聊天并发送一条消息）
build/desktop_companion.exe --smoke C:/tmp/shots
# 全屏截图（验证桌宠真的显示在桌面上）
build/desktop_companion.exe --shot C:/tmp/shots
```

## 已知限制（原型阶段）

- 对话是本地 mock，无真实模型；配置项固定
- 无命中区分区、点击穿透、贴边隐藏、悬浮球、多桌宠
- 无记忆 / 人设 / 好感度 / 控制中心
- QML 走 `qt_add_resources` 的 qrc 资源加载（未走 QML 模块），打包时依赖 `[qt] qmldir` 扫描

## 下一步

M2：接入 OpenAI 兼容 API、命中区分区、角色包导入导出、控制中心骨架。
