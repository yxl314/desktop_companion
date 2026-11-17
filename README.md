# 桌面桌宠（desktop_companion）

最小原型：**桌宠互动 + mock 聊天**。目标平台 Windows 10 / 11 x64；技术栈 Qt 6.12 + QML + C++23（MSYS2 ucrt64，g++ 16）。

## 已实现

- 透明、无边框、置顶桌宠窗口（占位形象 `resources/pet.png`），空闲时上下漂浮
- 点击桌宠：头顶弹气泡说一句随机台词（4.2 秒自动隐藏），桌宠弹跳反馈
- 拖拽：8px 阈值区分点击与拖拽，松手保存位置，下次启动恢复
- 右键菜单：打开聊天 / 隐藏桌宠 / 退出
- 系统托盘图标与菜单（显示隐藏、打开聊天、退出）
- 聊天窗：消息气泡（用户右绿 / 小柚左白）、Enter 发送、Shift+Enter 换行
- mock 对话引擎：关键词匹配 + 350ms「思考中」+ 逐字流式输出，不联网
- 验证模式：`--smoke <dir>` 自动开聊天、发一条消息并截图各窗口；`--shot <dir>` 截全屏

## 目录

```
CMakeLists.txt
src/main.cpp          # 装配、托盘、验证模式
src/bridge.{h,cpp}    # QML ↔ C++ 桥接（命令 Q_INVOKABLE / 事件信号）
src/chatengine.{h,cpp}# 本地 mock 对话引擎
qml/Pet.qml           # 桌宠窗口（拖动、点击、漂浮）
qml/Bubble.qml        # 气泡窗口（点击穿透）
qml/Chat.qml          # 聊天窗口
resources/pet.png     # 占位形象（透明 PNG，240x320）
scripts/gen_placeholder.py  # 占位形象生成脚本
docs/                 # 设计文档与截图
```

## 构建（Windows + MSYS2 ucrt64）

```bash
export PATH=/c/msys2/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys2/ucrt64
cmake --build build
```

发布前收集依赖：

```bash
windeployqt --release --no-translations --qmldir qml build/desktop_companion.exe
```

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
- QML 走 `qt_add_resources` 的 qrc 资源加载（未走 QML 模块），截图/部署更简单；后续再做模块化 + AOT
- 拖拽交互需人工确认（自动化只覆盖窗口位置与持久化）

## 下一步

M2：接入 OpenAI 兼容 API、命中区分区、角色包导入导出、控制中心骨架。
