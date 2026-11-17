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

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    # QML 的 qsTr 上下文是组件文件名（如 Manager.qml → Manager），必须按文件分组。
    contexts = {}
    for path in glob.glob(os.path.join(root, "qml", "*.qml")):
        context = os.path.splitext(os.path.basename(path))[0]
        text = open(path, encoding="utf-8").read()
        for match in re.finditer(r'qsTr\("((?:[^"\\]|\\.)*)"\)', text):
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
    strings = {"total": total}
    lines.append('</TS>')
    out = os.path.join(root, "translations", "desktop_companion_en.ts")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("wrote %s (%d strings, %d untranslated)" % (out, len(strings), len(missing)))
    for item in missing:
        print("  untranslated:", item)

if __name__ == "__main__":
    main()
