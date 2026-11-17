#!/usr/bin/env python3
"""从 qml/*.qml 收集 qsTr 字符串，生成英文 .ts（缺失翻译留空，回退中文）。

用法：python3 tools/gen_translations.py
产物：translations/desktop_companion_en.ts（提交到仓库，qt_add_translations 负责编译）。
"""
import glob
import os
import re
import xml.sax.saxutils as sax

EN = {
    "API Key": "API Key", "Base URL": "Base URL",
    "世界观名字": "World name", "世界观新名字": "New world name",
    "世界观正文（作为提示词的世界观段落）": "World body (injected into the system prompt)",
    "世界观：": "World: ", "主动对话频率": "Proactive chat frequency", "主题": "Theme", "语言": "Language",
    "重试": "Retry", "记录": "History", "收起": "Collapse", "跨天衰减": "Daily decay", "预览": "Preview", "展开": "Expand", "模型配置": "Model profiles", "应用": "Apply",
    "保存为配置": "Save as profile", "删除配置": "Delete profile", "配置名称": "Profile name",
    "按前台应用自动切换模式（工作/休闲）": "Auto switch mode by foreground app (work/casual)", "启用": "Enabled",
    "当前角色/人设的最近聊天记录": "Recent messages for this character/persona",
    "赞/踩：": "Likes/dislikes: ", "我：": "Me: ", "桌宠：": "Pet: ", "暂无记录": "No messages yet",
    "导出聊天记录": "Export chat history", "许可：Qt 6（LGPLv3）· SQLite（Public Domain）· stacklog / singleproc / cxx 工具（MIT）": "Licenses: Qt 6 (LGPLv3) · SQLite (Public Domain) · stacklog / singleproc / cxx tools (MIT)", "介绍一下你自己": "Introduce yourself",
    "今天有什么好玩的": "What is fun today?", "讲个冷笑话": "Tell a pun",
    "人设名字": "Persona name", "休闲": "Casual", "休闲模式会按频率主动说话": "Casual mode chats proactively at the chosen frequency",
    "使用": "Use", "使用中": "In use", "保存": "Save", "停止": "Stop",
    "全局快捷键：Ctrl+Alt+P 呼出/隐藏桌宠 · Ctrl+Alt+C 打开聊天 · Ctrl+Alt+M 勿扰模式":
        "Global hotkeys: Ctrl+Alt+P show/hide pet · Ctrl+Alt+C chat · Ctrl+Alt+M do-not-disturb",
    "删除": "Delete", "勿扰": "Do not disturb", "勿扰：暂停主动互动与待机动效": "DND: pauses proactive chat and idle motion",
    "发送": "Send", "取消置顶": "Unpin", "和小柚说点什么吧～": "Say something to Xiaoyou…",
    "回溯到这条消息，其后的消息与关联记忆会被清除，确定继续吗？":
        "Rewind to this message? Later messages and related memories will be removed.",
    "回溯对话": "Rewind chat", "好感度：": "Affection: ", "导入 .md": "Import .md",
    "导入形象": "Import avatar", "导入角色包": "Import character pack",
    "导出日志": "Export logs", "导出聊天记录": "Export chat history", "导出角色包": "Export character pack",
    "小柚": "Xiaoyou", "工作": "Work", "已保存": "Saved", "开机自动启动": "Start with Windows",
    "当前人设：": "Current persona: ", "当前角色：": "Current character: ", "待机浮动": "Idle float",
    "打开数据目录": "Open data folder", "打开日志目录": "Open log folder",
    "提示：更新地址在 config.json 的 updateUrl 填写（JSON：version/url/notes）。":
        "Tip: set updateUrl in config.json to a JSON manifest (version/url/notes).",
    "数据目录": "Data folder", "新人设": "New persona", "新建世界观": "New world",
    "新建人设": "New persona", "新建角色": "New character", "日志目录": "Log folder",
    "星座": "Zodiac", "显示命中区（调试）": "Show hit areas (debug)",
    "最爱的食物": "Favorite food", "未绑定世界观": "No world bound",
    "未配置模型，当前为本地 mock 回复。点「设置」接入 OpenAI 兼容 API。":
        "No model configured; using local mock replies. Open Settings to connect an OpenAI-compatible API.",
    "桌宠数量": "Pet count", "桌面桌宠 %1（原型）": "Desktop Companion %1 (prototype)",
    "检查更新": "Check for updates", "模型": "Model", "模式": "Mode",
    "欢迎使用桌面桌宠": "Welcome to Desktop Companion", "正文（注入提示词）": "Body (injected into the prompt)",
    "浅色": "Light", "测试连接": "Test connection", "浮动幅度": "Float amplitude", "深色": "Dark",
    "添加记忆": "Add memory", "清空对话": "Clear chat", "爱好": "Hobbies", "生日": "Birthday",
    "确定清空当前对话的聊天记录吗？": "Clear the chat history of this conversation?",
    "称呼": "Name", "绑定到当前角色": "Bind to current character", "编辑": "Edit",
    "编辑世界观": "Edit world", "编辑人设": "Edit persona", "编辑正文": "Edit body", "置顶": "Pin",
    "聊天 · ": "Chat · ", "背景故事": "Background", "自动抽取记忆": "Auto-extract memories",
    "角色名字": "Character name", "角色新名字": "New character name", "记忆内容": "Memory content",
    "记忆（当前角色 / 人设）": "Memories (character / persona)", "设置": "Settings",
    "设置 · 桌面桌宠 ": "Settings · Desktop Companion ",
    "说点什么…（Enter 发送）": "Type a message… (Enter to send)",
    "贴边隐藏（顶部不隐藏）": "Hide at screen edge (not top)", "跟随系统": "System",
    "输入": "Input", "透明区域点击穿透": "Click-through on transparent pixels",
    "重启对话": "Restart chat",
    "重启对话会保留聊天记录，但重置当前人设的记忆与上下文，确定继续吗？":
        "Restart chat? History is kept, but this persona's memories and context are reset.",
    "重命名": "Rename", "重新显示新手引导": "Show onboarding again",
    "隐藏桌宠时显示悬浮球": "Show floating ball when pet is hidden",
    "预设": "Preset", "默认世界": "Default world", "（": " (", "）": ")",
}

EN.update({
    "思考中…": "Thinking…", "回复中…": "Replying…",
    "已导入人设：%1": "Persona imported: %1", "已切换人设：%1": "Persona switched: %1",
    "打开聊天": "Open chat", "切换角色": "Switch character", "设置…": "Settings…",
    "隐藏桌宠": "Hide pet", "退出": "Quit",
    "已创建角色：%1": "Character created: %1", "已重命名角色": "Character renamed",
    "已删除角色": "Character deleted", "角色不存在：%1": "Character not found: %1",
    "请先选择角色": "Select a character first",
    "已更新角色形象；图片无透明通道，点击穿透会按整张图矩形生效":
        "Avatar updated; no alpha channel, click-through uses the full rectangle",
    "已更新角色形象": "Avatar updated", "人设已保存": "Persona saved",
    "已创建人设：%1": "Persona created: %1", "人设已存在：%1": "Persona already exists: %1",
    "已创建世界观：%1": "World created: %1", "世界观已重命名": "World renamed",
    "世界观已删除": "World deleted", "世界观已保存": "World saved",
    "已导入世界观": "World imported", "已更新角色世界观": "Character world updated",
    "角色包已导出：%1": "Character pack exported: %1", "已导入角色：%1": "Character imported: %1",
    "聊天库不可用": "Chat store unavailable", "聊天记录已导出：%1": "Chat history exported: %1",
    "已导出 %1 个日志文件到 %2": "Exported %1 log files to %2", "还没有角色": "No characters yet",
    "桌宠 %1 已切换到：%2": "Pet %1 switched to: %2",
    "请先选择角色并填写记忆内容": "Select a character and enter memory content",
    "已添加记忆": "Memory added", "记忆已更新": "Memory updated",
    "配置名称不能为空": "Profile name cannot be empty",
    "已保存模型配置：%1": "Model profile saved: %1", "已应用模型配置：%1": "Model profile applied: %1",
    "已重启对话，记忆已重置": "Chat restarted; memories reset",
    "已回溯，之后的消息与记忆已清除": "Rewound; later messages and memories removed",
    "桌面桌宠（原型）": "Desktop Companion (prototype)",
    "显示 / 隐藏桌宠": "Show / hide pet", "陌生": "Stranger", "我已经在桌面上啦～": "I am already on your desktop~", "熟悉": "Familiar",
    "亲近": "Close", "信赖": "Trusted",
    "已保存": "Saved", "保存": "Save", "删除": "Delete",
})

# T5 补齐：C++ 错误码 / 数据层用户可见文案（services/errorcodes.cpp、data/*）的英文翻译。
EN.update({
    # errors（services/errorcodes.cpp，QCoreApplication::translate("errors", ...)）
    "操作成功": "Success",
    "输入不能为空": "Input cannot be empty",
    "尚未配置模型，请先在设置中填写 Base URL 与模型名":
        "No model configured; fill in Base URL and model name in Settings first",
    "该服务需要 API Key，请先在设置中填写": "This service requires an API Key; enter it in Settings first",
    "Base URL 不是合法地址": "Base URL is not a valid address",
    "人设文件无效或不可读": "Persona file is invalid or unreadable",
    "图片无法解码或格式不支持": "The image cannot be decoded or its format is unsupported",
    "记忆条目已达上限，请先清理": "Memory limit reached; clean up memories first",
    "角色包不完整，缺少人设或形象": "Incomplete character pack: missing persona or avatar",
    "无法连接模型服务，请检查网络与服务地址":
        "Cannot reach the model service; check the network and service address",
    "请求超时，请稍后重试": "Request timed out; try again later",
    "鉴权失败，请检查 API Key 或权限": "Authentication failed; check the API Key or permissions",
    "模型不存在，请检查模型名": "Model not found; check the model name",
    "请求过于频繁或额度不足，请稍后重试": "Rate limited or quota exceeded; try again later",
    "模型服务异常，请稍后重试": "Model service error; try again later",
    "流式响应解析失败，回复可能中断": "Failed to parse the streamed response; the reply may be incomplete",
    "模型返回了空内容": "The model returned empty content",
    "读取文件失败，请检查路径与权限": "Failed to read file; check the path and permissions",
    "写入文件失败，请检查磁盘与权限": "Failed to write file; check the disk and permissions",
    "磁盘空间不足": "Not enough disk space",
    "没有访问权限": "Permission denied",
    "数据文件损坏，已尝试回退或备份": "Data file is corrupted; a fallback or backup was attempted",
    "名称与现有文件冲突，请换一个名字": "The name conflicts with an existing file; choose another name",
    "当前有任务进行中，请稍后再试": "Another task is in progress; try again later",
    "已中止": "Aborted",
    "当前状态不允许该操作": "This operation is not allowed in the current state",
    "该操作不可撤销，需要二次确认": "This action cannot be undone and needs confirmation",
    "未知错误，请查看日志": "Unknown error; see the logs",

    # MemoryStore（data/memorystore.cpp）
    "数据库不可用": "Database unavailable",
    "记忆库初始化失败": "Failed to initialize the memory database",
    "打开记忆库失败": "Failed to open the memory database",
    "SQLite 执行失败": "SQLite execution failed",
    "读取 schema 版本失败": "Failed to read schema version",
    "memory.db schema 版本 %1 高于程序支持的 %2，请升级应用":
        "memory.db schema version %1 is newer than the supported %2; please upgrade the app",
    "记忆库不可用": "Memory database unavailable",
    "记忆条目不能为空": "Memory entry cannot be null",
    "记忆内容不能为空": "Memory content cannot be empty",
    "角色 id 不能为空": "Character id cannot be empty",
    "记忆条目已达上限（%1 条），请先清理": "Memory limit reached (%1 entries); clean up first",
    "新增记忆失败：%1": "Failed to add memory: %1",
    "记忆 id 无效": "Invalid memory id",
    "更新记忆失败：%1": "Failed to update memory: %1",
    "记忆条目不存在：%1": "Memory entry not found: %1",
    "删除记忆失败：%1": "Failed to delete memory: %1",
    "更新置顶失败：%1": "Failed to update pin state: %1",
    "检索记忆失败：%1": "Failed to search memories: %1",
    "刷新记忆使用时间失败：%1": "Failed to refresh memory usage time: %1",
    "提交记忆使用时间失败：%1": "Failed to commit memory usage time: %1",
    "清空记忆失败：%1": "Failed to clear memories: %1",

    # ChatStore（data/chatstore.cpp）
    "chat schema 版本过新：%1": "Chat schema version is too new: %1",

    # CharacterStore（data/characterstore.cpp）
    "无法写入文件：%1": "Cannot write file: %1",
    "写入文件失败：%1": "Failed to write file: %1",
    "角色名不能为空": "Character name cannot be empty",
    "角色名无法用作目录名：%1": "Character name cannot be used as a directory name: %1",
    "角色已存在：%1": "Character already exists: %1",
    "无法创建角色目录：%1": "Cannot create character directory: %1",
    "角色不存在：%1": "Character not found: %1",
    "无法删除角色目录：%1": "Cannot delete character directory: %1",
    "人设输出指针为空": "Persona output pointer is null",
    "角色缺少 card.md：%1": "Character is missing card.md: %1",
    "元数据输出指针为空": "Metadata output pointer is null",
    "无法读取 meta.json：%1": "Cannot read meta.json: %1",
    "meta.json 损坏：%1": "meta.json is corrupted: %1",
    "meta.json 损坏：顶层不是对象": "meta.json is corrupted: top level is not an object",
    "缩放比例无效：%1": "Invalid scale: %1",
    "形象文件不存在：%1": "Avatar file not found: %1",
    "形象文件格式不支持：%1": "Unsupported avatar file format: %1",
    "无法创建形象目录：%1": "Cannot create avatar directory: %1",
    "无法覆盖形象文件：%1": "Cannot overwrite avatar file: %1",
    "复制形象文件失败：%1": "Failed to copy avatar file: %1",
    "角色仓库已销毁": "Character store has been destroyed",
    "人设 id 无效": "Invalid persona id",
    "人设 id 无效：%1": "Invalid persona id: %1",
    "人设不存在：%1": "Persona not found: %1",
    "人设名称不能为空": "Persona name cannot be empty",
    "无法创建人设目录：%1": "Cannot create persona directory: %1",
    "无法删除人设文件：%1": "Cannot delete persona file: %1",

    # WorldStore（data/worldstore.cpp）
    "无法读取文件：%1": "Cannot read file: %1",
    "世界观输出指针为空": "World output pointer is null",
    "世界观文件路径为空": "World file path is empty",
    "世界观 id 无效：%1": "Invalid world id: %1",
    "世界观不存在：%1": "World not found: %1",
    "世界观名不能为空": "World name cannot be empty",
    "世界观名无法用作目录名：%1": "World name cannot be used as a directory name: %1",
    "无法创建世界观目录：%1": "Cannot create world directory: %1",
    "无法删除世界观目录：%1": "Cannot delete world directory: %1",
    "世界观已存在：%1": "World already exists: %1",
    "世界观文件名无法用作目录名：%1":
        "World file name cannot be used as a directory name: %1",
    "世界观仓库已销毁": "World store has been destroyed",

    # PersonaStore（data/personastore.cpp）
    "人设文件名无效": "Invalid persona file name",
    "无法写入人设目录：%1": "Cannot write persona directory: %1",
    "人设仓库已销毁": "Persona store has been destroyed",

    # PersonaFile（services/personafile.cpp）
    "人设文件路径为空": "Persona file path is empty",
    "无法读取人设文件：%1": "Cannot read persona file: %1",
    "人设名称为空：缺少 frontmatter 且文件名为空":
        "Persona name is empty: no frontmatter and the file name is empty",
    "frontmatter 缺少结束的 --- 分隔符": "frontmatter is missing the closing --- separator",
    "frontmatter 第 %1 行格式非法：缺少冒号": "Invalid frontmatter at line %1: missing colon",
    "frontmatter 第 %1 行格式非法：key 为空": "Invalid frontmatter at line %1: empty key",
    "人设名称为空：frontmatter 缺少 name 且文件名为空":
        "Persona name is empty: frontmatter has no name and the file name is empty",

    # IntimacyService（data/intimacyrepository.cpp）
    "读取好感度文件失败：%1": "Failed to read intimacy file: %1",
    "好感度文件损坏且备份失败，已回退默认值":
        "Intimacy file is corrupted and backup failed; reverted to defaults",
    "好感度文件损坏，已备份为 %1": "Intimacy file is corrupted; backed up as %1",
    "好感度文件损坏（%1）且备份失败，已回退默认值":
        "Intimacy file is corrupted (%1) and backup failed; reverted to defaults",
    "好感度文件损坏（%1），已备份为 %2": "Intimacy file is corrupted (%1); backed up as %2",
    "尚未加载角色好感度": "Intimacy for the character is not loaded",
    "创建好感度目录失败：%1": "Failed to create intimacy directory: %1",
    "写入好感度文件失败：%1": "Failed to write intimacy file: %1",
    "保存好感度文件失败：%1": "Failed to save intimacy file: %1",
    "衰减日期无效": "Invalid decay date",
    "跨天衰减（%1 天）": "Daily decay (%1 days)",
    "好感度文件版本 %1 高于程序支持的 %2":
        "Intimacy file version %1 is newer than the supported %2",
    "score 字段不是数字": "score field is not a number",
    "decayEnabled 字段不是布尔值": "decayEnabled field is not a boolean",
    "lastDecayDate 字段不是合法日期": "lastDecayDate field is not a valid date",
    "events 字段不是数组": "events field is not an array",
    "events 条目不是对象": "events entry is not an object",
})

# 新主窗口 MainWindow.qml 的导航 / 引导文案（T5 顺带补齐，源文件仍在 QML 侧维护）。
EN.update({
    "聊天": "Chat", "角色": "Characters", "人设": "Personas", "世界观": "Worlds",
    "记忆": "Memories", "数据": "Data", "关于": "About",
    "桌面桌宠": "Desktop Companion", "宠": "Pet", "中": "中", "系": "Sys",
    "切换到浅色主题": "Switch to light theme",
    "切换到深色主题": "Switch to dark theme",
    "切换语言（需重启生效）": "Switch language (takes effect after restart)",
    "语言已切换，重启应用后生效": "Language switched; restart the app to apply",
    "桌宠运行中": "Pet running", "桌宠已停止": "Pet stopped",
    "稍后": "Later", "开始使用": "Get started",
    "发现新版本 %1，可在「设置 - 更新」中查看":
        "New version %1 found; see Settings - Updates",
    "聊天：主窗口「聊天」页专注对话，Enter 发送、Shift+Enter 换行。":
        "Chat: focus on conversations in the Chat page; Enter to send, Shift+Enter for a new line",
    "角色：在「角色」页新建或导入角色，点「启动为桌宠」让 TA 出现在桌面。":
        "Characters: create or import characters, then click Launch as pet to show them on the desktop",
    "桌宠：左键点击按命中区说台词，拖动移动，右键菜单，双击打开主窗口聊天。":
        "Pet: left-click for hit-area lines, drag to move, right-click for the menu, double-click to open chat",
    "模型：在「模型」页选择预设或填写 OpenAI 兼容接口，测试连接后保存。":
        "Model: pick a preset or fill in an OpenAI-compatible endpoint, test the connection, then save",
    "快捷键：Ctrl+Alt+P 显隐桌宠 · Ctrl+Alt+C 打开聊天 · Ctrl+Alt+M 勿扰。":
        "Hotkeys: Ctrl+Alt+P show/hide pet · Ctrl+Alt+C open chat · Ctrl+Alt+M do not disturb",
})

EN.update({
    # T6 补齐：新 UI（qml/MainWindow.qml、qml/pages/**、qml/components/**、qml/pet/**）的英文翻译。
    # 词典为全局 source -> EN（不区分 QML context）；保留 %1/%2 占位符与标点，专有名词不译。
    # ---- 通用按钮 / 标签 / 占位符 / 状态（InputDialog、PageHeader、Card、窗口标题等）
    "取消": "Cancel", "确定": "OK", "添加": "Add", "打开": "Open", "刷新": "Refresh",
    "清空": "Clear", "导出": "Export", "导入": "Import", "启动": "Start", "重启": "Restart",
    "关": "Off", "低": "Low", "高": "High", "当前": "Current", "名称": "Name",
    "未选择": "None selected", "输入…": "Type…", "输入内容…": "Enter text…",
    "正在输入…": "Typing…", "生成中…": "Generating…", "处理中…": "Processing…",
    "悬浮球": "Floating ball", "外观": "Appearance", "桌宠": "Pet", "桌宠气泡": "Pet bubble",
    "版本 %1": "Version %1", "中文": "Chinese",
    "1 只": "1 pet", "2 只": "2 pets", "3 只": "3 pets",

    # ---- 关于页 AboutPage
    "功能概览": "Feature overview",
    "开源许可": "Open-source licenses",
    "本项目使用的第三方组件": "Third-party components used in this project",
    "暂无更新信息": "No update information",
    "更新与数据": "Updates & data",
    "更新地址：%1": "Update URL: %1",
    "桌面桌宠 · 版本 %1": "Desktop Companion · Version %1",
    "版本 %1 · MVVM 重构版": "Version %1 · MVVM rewrite",
    "返回设置": "Back to Settings",
    "所有数据保存在本机，模型通过 OpenAI 兼容接口接入":
        "All data stays on this device; models connect through an OpenAI-compatible API",
    "专注聊天：760px 居中消息区、流式回复、卡片折叠、👍👎 与回溯":
        "Focused chat: 760px centered message area, streaming replies, collapsible cards, 👍👎 and rewind",
    "角色与人设：卡片网格管理角色，启动为桌宠；人设与世界观注入提示词":
        "Characters & personas: manage characters in a card grid and launch as a pet; personas and worlds are injected into the prompt",
    "记忆与好感度：长期记忆抽取、置顶、跨天好感度衰减":
        "Memories & affection: long-term memory extraction, pinning, and daily affection decay",
    "多模型：预设 / 自定义 Base URL / 多套配置 / 连接测试":
        "Multiple models: presets / custom Base URL / multiple profiles / connection tests",
    "桌宠：多桌宠、拖拽、贴边、点击穿透、命中区调试、悬浮球与气泡":
        "Pet: multiple pets, dragging, edge snapping, click-through, hit-area debug, floating ball and bubble",
    "数据：聊天与日志导出、数据目录管理、新手引导重置":
        "Data: chat and log export, data folder management, onboarding reset",

    # ---- 角色页 / 角色卡片
    "删除角色": "Delete character", "重命名角色": "Rename character",
    "启动为桌宠": "Launch as pet", "已启用": "Enabled", "已禁用": "Disabled",
    "桌宠中": "Pet active",
    "管理桌宠角色": "Manage pet characters",
    "管理桌宠角色：形象、世界观与启用状态":
        "Manage pet characters: avatar, world, and enabled state",
    "新建一个角色，或导入形象 / 角色包开始陪伴。":
        "Create a character or import an avatar / character pack to get started.",
    "确定删除该角色及其聊天记录吗？": "Delete this character and its chat history?",

    # ---- 聊天页 ChatPage
    "Enter 发送 · Shift+Enter 换行": "Enter to send · Shift+Enter for a new line",
    "刷新对话": "Refresh chat", "去配置": "Set up", "回溯到这里": "Rewind to here",
    "好感度 %1（%2）": "Affection %1 (%2)",
    "未配置模型，当前使用本地 mock 回复。": "No model configured; using local mock replies.",
    "模型设置": "Model settings",
    "说点什么…（Enter 发送，Shift+Enter 换行）":
        "Say something… (Enter to send, Shift+Enter for a new line)",
    "说点什么开始对话吧": "Say something to start the chat",

    # ---- 人设页 PersonasPage
    "人设列表": "Persona list", "删除人设": "Delete persona",
    "当前人设：%1 · 好感度 %2": "Current persona: %1 · Affection %2",
    "人设会作为提示词的一部分注入对话，影响角色的说话方式。":
        "The persona is injected into the conversation prompt and shapes how the character speaks.",
    "保存后立即生效；「使用」可切换当前生效人设":
        "Applies immediately; click Use to switch the active persona",
    "如：小柚": "e.g. Xiaoyou",
    "描述性格、说话习惯、背景故事…": "Describe personality, speech habits, and background…",
    "提示：填写名称与正文后点击「保存」创建新人设。":
        "Tip: fill in the name and body, then click Save to create a persona.",
    "暂无人设": "No personas yet",
    "确定删除当前人设吗？": "Delete the current persona?",
    "选择或新建一个人设": "Select or create a persona",

    # ---- 世界观页 WorldsPage
    "世界观列表": "World list", "删除世界观": "Delete world", "重命名世界观": "Rename world",
    "%1 角色": "%1 characters",
    "世界观为角色提供共享背景；保存后新对话立即生效":
        "Worlds give characters shared background; changes apply to new chats immediately",
    "世界观正文会作为提示词的世界观段落，与角色的人设一起注入。":
        "The world body is injected as the world section of the prompt together with the character persona.",
    "为角色提供共享的背景设定，注入对话提示词":
        "Shared background for characters, injected into the chat prompt",
    "如：魔法学院": "e.g. Magic Academy",
    "地理、历史、势力、规则…": "Geography, history, factions, rules…",
    "暂无世界观": "No worlds yet",
    "确定删除该世界观吗？": "Delete this world?",
    "选择或新建一个世界观": "Select or create a world",

    # ---- 记忆页 MemoriesPage
    "已保存的记忆": "Saved memories", "删除记忆": "Delete memory", "编辑记忆": "Edit memory",
    "立即抽取": "Extract now",
    "手动记录一条希望角色长期记住的信息":
        "Manually record something you want the character to remember long-term",
    "例如：主人最喜欢雨天和热可可": "e.g. Master loves rainy days and hot cocoa",
    "%1 条记忆 · 注入当前角色与人设的提示词":
        "%1 memory entries · injected into the current character/persona prompt",
    "置顶的记忆会优先注入提示词": "Pinned memories are injected into the prompt first",
    "聊天后角色会逐渐记住你们之间的事，也可以在这里手动添加。":
        "The character gradually remembers your conversations; you can also add memories manually here.",
    "确定删除这条记忆吗？": "Delete this memory entry?", "还没有记忆": "No memories yet",

    # ---- 记录页 RecordsPage
    "导出记录": "Export history", "清空记录": "Clear history", "我": "Me",
    "确定清空当前角色/人设的聊天记录吗？":
        "Clear the chat history of the current character/persona?",
    "和角色聊天后，这里会展示最近的对话与赞踩反馈。":
        "Recent conversations and likes/dislikes appear here after you chat with a character.",

    # ---- 模型页 ModelsPage
    "Max Tokens": "Max Tokens", "Temperature": "Temperature",
    "保存多套供应商/模型组合，一键切换":
        "Save multiple provider/model combinations and switch in one click",
    "保存当前为配置": "Save current as profile",
    "兼容 OpenAI 接口：Ollama / LM Studio / DeepSeek / OpenAI / 自定义":
        "OpenAI-compatible API: Ollama / LM Studio / DeepSeek / OpenAI / Custom",
    "刷新配置": "Refresh profiles",
    "密钥仅保存在本机配置中": "The API key is stored only in the local config",
    "已保存 %1 套配置": "%1 profiles saved",
    "暂无保存的配置；配置好上方连接后点「保存当前为配置」。":
        "No saved profiles yet. Configure the connection above, then click “Save current as profile”.",
    "未配置模型，当前回退为本地 mock 回复":
        "No model configured; falling back to local mock replies",
    "本地服务可留空": "Leave empty for local services",
    "模型已就绪": "Model ready",
    "正在测试连接…": "Testing connection…", "测试中…": "Testing…",
    "连接配置": "Connection",

    # ---- 数据页 DataPage
    "占用空间": "Space used", "存储位置": "Storage", "导出聊天": "Export chat",
    "尚未导出": "Not exported yet", "最近导出": "Recent exports", "维护": "Maintenance",
    "导出格式为 JSON / Markdown，可用于备份或迁移":
        "Export as JSON / Markdown for backup or migration",
    "所有配置与聊天记录均保存在本机，不上传云端":
        "All settings and chat history stay on this device; nothing is uploaded",
    "提示：导出文件默认保存在数据目录下的 exports/ 中。":
        "Tip: exported files are saved to exports/ under the data folder by default.",
    "聊天记录、日志与本地数据的导出和位置管理":
        "Export and location management for chats, logs, and local data",
    "重置新手引导": "Reset onboarding",

    # ---- 设置页 SettingsPage：外观 / 语言
    "外观、行为、快捷键、更新与数据": "Appearance, behavior, hotkeys, updates, and data",
    "主题与语言即时切换；语言由系统托盘重启策略决定":
        "Theme and language apply instantly; language changes follow the system tray restart policy",

    # ---- 设置页 SettingsPage：桌宠与行为
    "桌宠与行为": "Pet & behavior",
    "桌宠窗口独立于主窗口，可随时启动或停止":
        "The pet window is separate from the main window and can be started or stopped at any time",
    "桌宠运行": "Pet running", "桌宠角色": "Pet character",
    "开机自动启动与「启动桌宠」使用的角色":
        "Character used for auto-start and when launching the pet",
    "暂无角色": "No characters yet",
    "开机自动启动桌宠": "Start pet with Windows",
    "贴边隐藏": "Hide at screen edge",
    "拖到屏幕左右边缘时收起（顶部不隐藏）":
        "Collapse when dragged to the left or right screen edge (not the top)",
    "按形象 alpha 生成窗口区域，透明处不挡鼠标":
        "Build the window region from the avatar alpha; transparent areas ignore the mouse",
    "命中区调试": "Hit-area debug",
    "在桌宠窗口叠加命中区域，便于核对形象 meta":
        "Overlay hit areas on the pet window to verify the avatar meta",
    "隐藏桌宠时显示悬浮球，点击恢复桌宠":
        "Show a floating ball while the pet is hidden; click it to restore the pet",
    "好感度跨天衰减": "Affection daily decay",

    # ---- 设置页 SettingsPage：模式与主动对话
    "模式与主动对话": "Mode & proactive chat",
    "勿扰模式会暂停主动互动与待机动效": "Do-not-disturb pauses proactive chat and idle motion",
    "前台自动模式": "Auto mode by foreground app",
    "按前台应用自动切换工作 / 休闲模式": "Auto switch work / casual mode by foreground app",

    # ---- 设置页 SettingsPage：快捷键
    "快捷键": "Hotkeys",
    "聊天输入：Enter 发送，Shift+Enter 换行":
        "Chat input: Enter to send, Shift+Enter for a new line",
    "打开主窗口聊天页": "Open the chat page in the main window",
    "切换勿扰模式": "Toggle do-not-disturb",

    # ---- 设置页 SettingsPage：更新
    "更新": "Updates", "更新地址": "Update URL", "保存地址": "Save URL",
    "更新清单为 JSON：version / url / notes":
        "Update manifest is JSON: version / url / notes",
    "留空则关闭自动更新检查": "Leave empty to disable automatic update checks",

    # ---- 设置页 SettingsPage：系统与数据
    "系统与数据": "System & data",
    "应用级设置与本地数据入口": "App-level settings and local data",
    "应用开机自启": "Start app with Windows",
    "新手引导": "Onboarding", "重新显示": "Show again",
    "重启应用": "Restart app", "立即重启": "Restart now",
    "确定立即重启应用吗？": "Restart the app now?",
    "语言切换": "Switch language",
    "语言切换需要重启应用后生效，是否立即重启？":
        "Language changes take effect after a restart. Restart now?",
    "关于本软件": "About this app",
})

EN.update({
    "陪伴桌面的桌宠应用：主窗口管理角色、人设、世界观、记忆与模型，桌宠窗口可独立运行，支持 1~3 只桌宠、点击穿透、贴边隐藏与悬浮球。":
        "A desktop companion app: manage characters, personas, worlds, memories and models in the main window; pet windows run independently with 1-3 pets, click-through, edge hiding and a floating ball.",
})

EN.update({
    # T7 补齐：ViewModel 层（src/viewmodels/*.cpp）与 src/app/application.cpp 的用户可见文案。
    # context 为 QObject 类名（如 AppViewModel）；保留 %1/%2 占位符，专有名词不译。
    # ---- AppViewModel（桌宠状态、更新检查、日志导出）
    "桌宠已启动": "Pet started", "桌宠已隐藏": "Pet hidden",
    "桌宠运行中（%1 只）": "Pet running (%1)",
    "未配置更新地址（config.json 的 updateUrl 可填版本清单 URL）":
        "No update URL configured (set updateUrl in config.json to a version manifest URL)",
    "正在检查更新…": "Checking for updates…",
    "检查失败：%1": "Check failed: %1",
    "检查失败：版本清单格式错误": "Check failed: invalid version manifest",
    "发现新版本 %1": "New version %1 available",
    "发现新版本 %1：%2": "New version %1 available: %2",
    "已是最新版本（%1）": "Up to date (%1)",
    "选择日志导出目录": "Select log export folder",

    # ---- Application（系统托盘菜单）
    "打开主窗口": "Open main window",

    # ---- CharactersViewModel
    "选择角色形象": "Select character avatar",
    "图片 (*.png *.jpg *.jpeg *.webp *.gif *.bmp)":
        "Images (*.png *.jpg *.jpeg *.webp *.gif *.bmp)",
    "Zip (*.zip)": "Zip (*.zip)",
    "已设为桌宠角色：%1": "Pet character set: %1",
    "还没有角色，点击「新建角色」开始。":
        "No characters yet. Click “New character” to get started.",

    # ---- ChatViewModel（欢迎语 / 推荐问句 / 导出 / 反馈）
    "今天过得怎么样？": "How is your day going?",
    "陪我聊一会儿吧": "Chat with me for a while",
    "给我讲个故事": "Tell me a story",
    "夸夸我": "Praise me",
    "和 %1 聊点什么吧。": "Say something to %1.",
    "本地 mock（未配置模型）": "Local mock (no model configured)",
    "爱好：%1；最爱的食物：%2": "Hobbies: %1; favorite food: %2",
    "已清空对话": "Chat cleared",
    "消息尚未落盘，暂时无法评价": "Message is not saved yet; cannot rate it now",
    "消息记录缺少数据库 id，无法安全回溯（等待数据层修复）":
        "Message record has no database id; cannot rewind safely (waiting for a data-layer fix)",
    "# 与 %1 的聊天记录\n\n": "# Chat history with %1\n\n",

    # ---- DataViewModel / RecordsViewModel（Markdown 导出）
    "# 聊天记录\n\n": "# Chat history\n\n",
    "助手": "Assistant",
    "Markdown (*.md)": "Markdown (*.md)",
    "已重置新手引导，下次启动时重新展示":
        "Onboarding reset; it will show again on next launch",
    "清空聊天记录失败": "Failed to clear chat history",
    "聊天记录已清空": "Chat history cleared",

    # ---- MemoriesViewModel
    "正在抽取记忆…": "Extracting memories…",
    "记忆已删除": "Memory deleted",
    "请填写记忆内容": "Enter memory content",

    # ---- ModelSettingsViewModel
    "模型设置已保存": "Model settings saved",
    "已删除模型配置": "Model profile deleted",

    # ---- PersonasViewModel
    "未命名人设": "Untitled persona", "默认人设": "Default persona",
    "导入人设（Markdown）": "Import persona (Markdown)", "人设已删除": "Persona deleted",

    # ---- WorldsViewModel
    "世界观名称不能为空": "World name cannot be empty",
    "导入世界观（Markdown）": "Import world (Markdown)",
    "未注入配置仓库，无法确定当前角色":
        "Config store not injected; cannot determine the current character",
    "已取消当前角色的世界观归属": "Removed the current character's world binding",
    "已把当前角色归属到该世界观": "Bound the current character to this world",
})

CPP_CONTEXTS = {
    "src/data/intimacyrepository.cpp": ("IntimacyService", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/main.cpp": ("Main", r'translate\("Main",\s*"((?:[^"\\]|\\.)*)"\)'),
    # T5 C++ 文案收尾：错误码与 data/* 用户可见错误统一走翻译接口。
    "src/services/errorcodes.cpp": ("errors", r'trText\("((?:[^"\\]|\\.)*)"\)'),
    "src/services/personafile.cpp": ("PersonaFile", r'trText\("((?:[^"\\]|\\.)*)"\)'),
    "src/data/characterstore.cpp": ("CharacterStore", r'(?:trText|tr)\("((?:[^"\\]|\\.)*)"\)'),
    "src/data/worldstore.cpp": ("WorldStore", r'(?:trText|tr)\("((?:[^"\\]|\\.)*)"\)'),
    "src/data/personastore.cpp": ("PersonaStore", r'(?:trText|tr)\("((?:[^"\\]|\\.)*)"\)'),
    "src/data/memorystore.cpp": ("MemoryStore", r'trText\("((?:[^"\\]|\\.)*)"\)'),
    "src/data/chatstore.cpp": ("ChatStore", r'trText\("((?:[^"\\]|\\.)*)"\)'),

    # T7 ViewModel / Application 层用户可见文案（QObject::tr，context 为类名）。
    "src/viewmodels/appviewmodel.cpp": ("AppViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/chatviewmodel.cpp": ("ChatViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/charactersviewmodel.cpp": ("CharactersViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/personasviewmodel.cpp": ("PersonasViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/worldsviewmodel.cpp": ("WorldsViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/memoriesviewmodel.cpp": ("MemoriesViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/modelsettingsviewmodel.cpp": ("ModelSettingsViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/recordsviewmodel.cpp": ("RecordsViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/dataviewmodel.cpp": ("DataViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/petviewmodel.cpp": ("PetViewModel", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/viewmodels/dialogservice.cpp": ("DialogService", r'tr\("((?:[^"\\]|\\.)*)"\)'),
    "src/app/application.cpp": ("Application", r'tr\("((?:[^"\\]|\\.)*)"\)'),
}

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    # QML 的 qsTr 上下文是组件文件名（如 Manager.qml → Manager），必须按文件分组。
    contexts = {}
    for path in glob.glob(os.path.join(root, "qml", "**", "*.qml"), recursive=True):
        context = os.path.splitext(os.path.basename(path))[0]
        text = open(path, encoding="utf-8").read()
        for match in re.finditer(r'qsTr\("((?:[^"\\]|\\.)*)"\)', text):
            contexts.setdefault(context, set()).add(match.group(1).replace('\\n', '\n'))

    for path, (context, pattern) in CPP_CONTEXTS.items():
        text = open(os.path.join(root, path), encoding="utf-8").read()
        for match in re.finditer(pattern, text):
            contexts.setdefault(context, set()).add(match.group(1).replace('\\n', '\n'))

    lines = ['<?xml version="1.0" encoding="utf-8"?>', '<!DOCTYPE TS>', '<TS version="2.1" language="en_US">']
    missing = []
    total = 0
    for context in sorted(contexts):
        lines.append('<context>')
        lines.append('    <name>%s</name>' % sax.escape(context))
        for source in sorted(contexts[context]):
            total += 1
            translation = EN.get(source, "")
            if not translation:
                missing.append(source)
            lines.append('    <message>')
            lines.append('        <source>%s</source>' % sax.escape(source))
            lines.append('        <translation>%s</translation>' % sax.escape(translation))
            lines.append('    </message>')
        lines.append('</context>')
    lines.append('</TS>')
    out = os.path.join(root, "translations", "desktop_companion_en.ts")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("wrote %s (%d strings, %d untranslated)" % (out, total, len(missing)))
    for item in missing:
        print("  untranslated:", item)

if __name__ == "__main__":
    main()
