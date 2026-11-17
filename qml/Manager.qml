import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Window {
    id: root
    objectName: "managerWindow"
    width: 640
    height: 720
    visible: bridge.managerVisible
    title: qsTr("设置 · 桌面桌宠 ") + bridge.appVersion
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: bridge.windowColor
    Material.theme: bridge.darkTheme ? Material.Dark : Material.Light
    Material.accent: "#6cbf8a"

    property int page: bridge.managerPage
    onClosing: (close) => {
        close.accepted = false
        bridge.managerVisible = false
    }

    // ---- 通用输入弹窗 ----
    Dialog {
        id: inputDialog
        property string prompt: ""
        property string value: ""
        property var onAccept: function(text) {}
        title: qsTr("输入")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: root.onDialogAccept(inputDialog.value)
        contentItem: ColumnLayout {
            Label { text: inputDialog.prompt; color: bridge.textColor }
            TextField {
                id: inputField
                Layout.preferredWidth: 360
                text: inputDialog.value
                onTextChanged: inputDialog.value = text
            }
        }
        onOpened: {
            inputField.text = inputDialog.value
            inputField.forceActiveFocus()
        }
    }

    // ---- 人设编辑弹窗 ----
    Dialog {
        id: personaDialog
        title: qsTr("编辑人设")
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 560
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string personaId: ""

        function openFor(id) {
            personaId = id
            const fields = bridge.personaFields(id)
            nameField.text = fields.name || ""
            birthdayField.text = fields.birthday || ""
            zodiacField.text = fields.zodiac || ""
            hobbiesField.text = fields.hobbies || ""
            foodField.text = fields.favoriteFood || ""
            backgroundField.text = fields.background || ""
            bodyArea.text = fields.body || ""
            open()
        }

        onAccepted: {
            bridge.savePersonaFields({
                "id": personaDialog.personaId,
                "name": nameField.text,
                "birthday": birthdayField.text,
                "zodiac": zodiacField.text,
                "hobbies": hobbiesField.text,
                "favoriteFood": foodField.text,
                "background": backgroundField.text,
                "body": bodyArea.text
            })
        }

        contentItem: ColumnLayout {
            spacing: 6
            GridLayout {
                columns: 2
                columnSpacing: 8
                rowSpacing: 6
                Label { text: qsTr("称呼") }
                TextField { id: nameField; Layout.fillWidth: true }
                Label { text: qsTr("生日") }
                TextField { id: birthdayField; Layout.fillWidth: true }
                Label { text: qsTr("星座") }
                TextField { id: zodiacField; Layout.fillWidth: true }
                Label { text: qsTr("爱好") }
                TextField { id: hobbiesField; Layout.fillWidth: true }
                Label { text: qsTr("最爱的食物") }
                TextField { id: foodField; Layout.fillWidth: true }
                Label { text: qsTr("背景故事") }
                TextField { id: backgroundField; Layout.fillWidth: true }
            }
            Label { text: qsTr("正文（注入提示词）") }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                TextArea { id: bodyArea; wrapMode: TextArea.Wrap }
            }
        }
    }

    // ---- 世界观正文弹窗 ----
    Dialog {
        id: worldDialog
        title: qsTr("编辑世界观")
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 560
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string worldId: ""
        property string worldBody: ""

        function openFor(id) {
            worldId = id
            const worlds = bridge.worlds
            for (let i = 0; i < worlds.length; ++i) {
                if (worlds[i].id === id)
                    worldBody = worlds[i].body || ""
            }
            bodyArea2.text = worldBody
            open()
        }

        onAccepted: bridge.saveWorldBody(worldDialog.worldId, bodyArea2.text)

        contentItem: ColumnLayout {
            Label { text: qsTr("世界观正文（作为提示词的世界观段落）") }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                TextArea { id: bodyArea2; wrapMode: TextArea.Wrap }
            }
        }
    }

    function askText(prompt, value, callback) {
        inputDialog.prompt = prompt
        inputDialog.value = value || ""
        inputDialog.onAccept = callback
        inputDialog.open()
    }

    function onDialogAccept(text) {
        if (inputDialog.onAccept)
            inputDialog.onAccept(text)
    }

    Dialog {
        id: onboarding
        title: qsTr("欢迎使用桌面桌宠")
        modal: true
        width: 460
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        onAccepted: bridge.onboardingDone = true
        contentItem: ColumnLayout {
            spacing: 8
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("1. 左键点击桌宠：按命中区说不同台词；双击打开聊天。\n"
                           + "2. 拖动可移动桌宠，拖到左右屏幕边缘会贴边收起。\n"
                           + "3. 右键桌宠：切换角色、设置、点击穿透。\n"
                           + "4. 控制中心可配置模型、角色、人设、世界观、记忆与导出数据。\n"
                           + "5. 全局快捷键：Ctrl+Alt+P 显隐、Ctrl+Alt+C 聊天、Ctrl+Alt+M 勿扰。")
                color: bridge.textColor
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: pageBox
                Layout.preferredWidth: 200
                model: ["模型", "角色", "人设", "记忆", "世界观", "记录", "数据", "系统", "关于"]
                currentIndex: bridge.managerPage
                onActivated: bridge.managerPage = currentIndex
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("当前角色：") + bridge.currentCharacterName
                color: bridge.secondaryTextColor
                font.pixelSize: 12
            }
        }

        Label {
            Layout.fillWidth: true
            visible: bridge.statusText !== ""
            text: bridge.statusText
            color: bridge.secondaryTextColor
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: bridge.managerPage

            // ================= 模型 =================
            ColumnLayout {
                spacing: 10
                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10
                    Layout.fillWidth: true
                    Label { text: qsTr("预设") }
                    ComboBox {
                        id: presetBox
                        Layout.fillWidth: true
                        model: bridge.presets
                        onActivated: (index) => {
                            const defaults = bridge.presetDefaults(currentText)
                            baseUrlField.text = defaults.baseUrl
                            modelField.text = defaults.model
                            if (defaults.apiKey)
                                apiKeyField.text = defaults.apiKey
                        }
                    }
                    Label { text: qsTr("Base URL") }
                    TextField { id: baseUrlField; Layout.fillWidth: true; placeholderText: "http://127.0.0.1:11434/v1" }
                    Label { text: qsTr("API Key") }
                    TextField { id: apiKeyField; Layout.fillWidth: true; echoMode: TextInput.Password }
                    Label { text: qsTr("模型") }
                    TextField { id: modelField; Layout.fillWidth: true; placeholderText: "qwen2.5:7b / deepseek-chat" }
                    Label { text: qsTr("Temperature") }
                    RowLayout {
                        Slider {
                            id: temperatureSlider
                            Layout.preferredWidth: 220
                            from: 0.0
                            to: 2.0
                            stepSize: 0.05
                            value: bridge.temperature
                            onMoved: bridge.temperature = value
                        }
                        Label { text: Number(bridge.temperature).toFixed(2) }
                    }
                    Label { text: qsTr("Max tokens") }
                    SpinBox {
                        id: maxTokensBox
                        Layout.preferredWidth: 160
                        from: 0
                        to: 8192
                        stepSize: 64
                        value: bridge.maxTokens
                        onValueModified: bridge.maxTokens = value
                    }
                }
                RowLayout {
                    Label { text: qsTr("模型配置") }
                    ComboBox {
                        id: profileBox
                        Layout.preferredWidth: 200
                        model: bridge.modelProfiles
                        textRole: "name"
                        onActivated: profileBox.currentIndex = currentIndex
                    }
                    Button { text: qsTr("应用"); onClicked: bridge.applyModelProfile(profileBox.currentIndex) }
                    Button {
                        text: qsTr("保存为配置")
                        onClicked: root.askText(qsTr("配置名称"), "",
                                                (text) => bridge.saveModelProfile(text))
                    }
                    Button { text: qsTr("删除配置"); onClicked: bridge.deleteModelProfile(profileBox.currentIndex) }
                    Item { Layout.fillWidth: true }
                }
                RowLayout {
                    Button {
                        text: qsTr("测试连接")
                        onClicked: bridge.testLlmConnection(baseUrlField.text, apiKeyField.text, modelField.text)
                    }
                    Button {
                        text: qsTr("保存")
                        onClicked: {
                            bridge.saveLlmSettings(presetBox.currentText, baseUrlField.text,
                                                   apiKeyField.text, modelField.text)
                            saveHint.text = qsTr("已保存")
                        }
                    }
                    Label { id: saveHint; text: ""; color: "#6cbf8a"; font.pixelSize: 12 }
                }
                Label {
                    Layout.fillWidth: true
                    text: bridge.testResult
                    visible: bridge.testResult !== ""
                    wrapMode: Text.Wrap
                    color: bridge.secondaryTextColor
                    font.pixelSize: 12
                }
                RowLayout {
                    Label { text: qsTr("主动对话频率") }
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: ["关", "低", "中", "高"]
                        currentIndex: bridge.proactiveFrequency
                        onActivated: bridge.proactiveFrequency = currentIndex
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: bridge.secondaryTextColor
                    font.pixelSize: 12
                    text: qsTr("提示：Ollama 需先 ollama serve；LM Studio 需开启本地服务；云端 API 需填 Key。\n"
                               + "保存后立即生效；不配置时回退到本地 mock。")
                }
                Item { Layout.fillHeight: true }
            }

            // ================= 角色 =================
            ColumnLayout {
                spacing: 8
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.characters
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 64
                        radius: 8
                        color: modelData.current ? "#e8f5ec" : bridge.cardColor
                        border.color: modelData.current ? "#8fd3a8" : bridge.borderColor
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10
                            Image {
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 48
                                fillMode: Image.PreserveAspectFit
                                source: modelData.avatar ? "file:///" + modelData.avatar : ""
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Label { text: modelData.name; color: bridge.textColor; font.pixelSize: 15 }
                                Label {
                                    text: modelData.worldId ? qsTr("世界观：") + modelData.worldId : qsTr("未绑定世界观")
                                    color: bridge.secondaryTextColor; font.pixelSize: 11
                                }
                            }
                            Label { text: qsTr("使用中"); visible: modelData.current; color: "#4f9e6d"; font.pixelSize: 12 }
                            CheckBox {
                                text: qsTr("启用")
                                checked: modelData.enabled
                                onToggled: bridge.setCharacterEnabled(modelData.id, checked)
                            }
                            Button {
                                text: qsTr("使用"); enabled: !modelData.current
                                onClicked: bridge.selectCharacter(modelData.id)
                            }
                            Button {
                                text: qsTr("重命名")
                                onClicked: root.askText(qsTr("角色新名字"), modelData.name,
                                                        (text) => bridge.renameCharacter(modelData.id, text))
                            }
                            Button {
                                text: qsTr("删除")
                                onClicked: bridge.removeCharacter(modelData.id)
                            }
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: qsTr("新建角色")
                        onClicked: root.askText(qsTr("角色名字"), qsTr("小柚"),
                                                (text) => bridge.createCharacter(text))
                    }
                    Button { text: qsTr("导入形象"); onClicked: bridge.importAvatarFile() }
                    Button { text: qsTr("导入角色包"); onClicked: bridge.importCharacterPack() }
                    Button { text: qsTr("导出角色包"); onClicked: bridge.exportCharacterPack() }
                    Item { Layout.fillWidth: true }
                }
            }

            // ================= 人设 =================
            ColumnLayout {
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("当前人设：") + bridge.personaName
                        color: bridge.textColor
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: qsTr("好感度：") + bridge.intimacyScore + qsTr("（") + bridge.intimacyLevel + qsTr("）")
                        color: bridge.secondaryTextColor
                        font.pixelSize: 12
                    }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.personas
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 44
                        radius: 8
                        color: modelData.current ? "#e8f5ec" : bridge.cardColor
                        border.color: modelData.current ? "#8fd3a8" : bridge.borderColor
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: bridge.textColor
                                elide: Text.ElideRight
                            }
                            Label { text: qsTr("使用中"); visible: modelData.current; color: "#4f9e6d"; font.pixelSize: 12 }
                            Button {
                                text: qsTr("使用"); enabled: !modelData.current
                                onClicked: bridge.selectPersona(modelData.id)
                            }
                            Button {
                                text: qsTr("编辑")
                                onClicked: personaDialog.openFor(modelData.id)
                            }
                            Button {
                                text: qsTr("删除"); visible: modelData.id !== ""
                                onClicked: bridge.removePersona(modelData.id)
                            }
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: qsTr("新建人设")
                        onClicked: root.askText(qsTr("人设名字"), qsTr("新人设"),
                                                (text) => bridge.createPersona(text))
                    }
                    Button { text: qsTr("导入 .md"); onClicked: bridge.importPersonaFile() }
                    Item { Layout.fillWidth: true }
                }
            }

            // ================= 记忆 =================
            ColumnLayout {
                spacing: 8
                Label {
                    Layout.fillWidth: true
                    text: qsTr("记忆（当前角色 / 人设）")
                    color: bridge.textColor
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.memories
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 48
                        radius: 8
                        color: modelData.pinned ? "#fff7e0" : bridge.cardColor
                        border.color: modelData.pinned ? "#e6c86e" : bridge.borderColor
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 6
                            Label {
                                Layout.fillWidth: true
                                text: (modelData.pinned ? "📌 " : "") + modelData.content
                                color: bridge.textColor
                                elide: Text.ElideRight
                            }
                            Button {
                                text: modelData.pinned ? qsTr("取消置顶") : qsTr("置顶")
                                font.pixelSize: 11
                                onClicked: bridge.toggleMemoryPin(modelData.id)
                            }
                            Button {
                                text: qsTr("编辑")
                                font.pixelSize: 11
                                onClicked: root.askText(qsTr("记忆内容"), modelData.content,
                                                        (text) => bridge.updateMemory(modelData.id, text))
                            }
                            Button {
                                text: qsTr("删除")
                                font.pixelSize: 11
                                onClicked: bridge.removeMemory(modelData.id)
                            }
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: qsTr("添加记忆")
                        onClicked: root.askText(qsTr("记忆内容"), "",
                                                (text) => bridge.addMemory(text))
                    }
                    CheckBox {
                        text: qsTr("自动抽取记忆")
                        checked: bridge.autoExtractMemories
                        onToggled: bridge.autoExtractMemories = checked
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            // ================= 世界观 =================
            ColumnLayout {
                spacing: 8
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.worlds
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 44
                        radius: 8
                        color: bridge.cardColor
                        border.color: bridge.borderColor
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: bridge.textColor
                                elide: Text.ElideRight
                            }
                            Button {
                                text: qsTr("绑定到当前角色")
                                onClicked: bridge.assignCharacterWorld(modelData.id)
                            }
                            Button {
                                text: qsTr("编辑正文")
                                onClicked: worldDialog.openFor(modelData.id)
                            }
                            Button {
                                text: qsTr("重命名")
                                onClicked: root.askText(qsTr("世界观新名字"), modelData.name,
                                                        (text) => bridge.renameWorld(modelData.id, text))
                            }
                            Button {
                                text: qsTr("删除")
                                onClicked: bridge.removeWorld(modelData.id)
                            }
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: qsTr("新建世界观")
                        onClicked: root.askText(qsTr("世界观名字"), qsTr("默认世界"),
                                                (text) => bridge.createWorld(text))
                    }
                    Button { text: qsTr("导入 .md"); onClicked: bridge.importWorldFile() }
                    Item { Layout.fillWidth: true }
                }
            }

            // ================= 记录 =================
            ColumnLayout {
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("当前角色/人设的最近聊天记录") }
                    Item { Layout.fillWidth: true }
                    Label { text: qsTr("赞/踩：") + bridge.feedbackSummary; color: bridge.secondaryTextColor; font.pixelSize: 12 }
                    Button { text: qsTr("导出聊天记录"); onClicked: bridge.exportChatHistory() }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.records()
                    spacing: 4
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: Math.max(28, textItem.implicitHeight + 10)
                        radius: 6
                        color: modelData.role === "user" ? "#2f6b4a" : bridge.cardColor
                        border.color: bridge.borderColor
                        Text {
                            id: textItem
                            anchors.fill: parent
                            anchors.margins: 5
                            text: (modelData.role === "user" ? qsTr("我：") : qsTr("桌宠：")) + modelData.text
                            wrapMode: Text.Wrap
                            color: bridge.textColor
                            font.pixelSize: 12
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: bridge.records().length === 0
                    text: qsTr("暂无记录")
                    color: bridge.secondaryTextColor
                    font.pixelSize: 12
                }
            }

            // ================= 数据 =================
            ColumnLayout {
                spacing: 10
                Label { text: qsTr("数据目录"); color: bridge.textColor }
                Label { text: bridge.dataDir; color: bridge.secondaryTextColor; font.pixelSize: 12; wrapMode: Text.Wrap }
                Label { text: qsTr("日志目录"); color: bridge.textColor }
                Label { text: bridge.logDir; color: bridge.secondaryTextColor; font.pixelSize: 12; wrapMode: Text.Wrap }
                RowLayout {
                    Button { text: qsTr("导出聊天记录"); onClicked: bridge.exportChatHistory() }
                    Button { text: qsTr("导出日志"); onClicked: bridge.exportLogs() }
                    Button { text: qsTr("打开数据目录"); onClicked: bridge.openDataFolder() }
                    Button { text: qsTr("打开日志目录"); onClicked: bridge.openLogFolder() }
                }
                Item { Layout.fillHeight: true }
            }

            // ================= 系统 =================
            ColumnLayout {
                spacing: 12
                RowLayout {
                    Label { text: qsTr("语言"); Layout.preferredWidth: 70 }
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: ["中文", "English", qsTr("跟随系统")]
                        currentIndex: bridge.language
                        onActivated: {
                            bridge.language = currentIndex
                            bridge.restartApp()
                        }
                    }
                }
                RowLayout {
                    Label { text: qsTr("主题"); Layout.preferredWidth: 70 }
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: [qsTr("浅色"), qsTr("深色"), qsTr("跟随系统")]
                        currentIndex: bridge.theme
                        onActivated: bridge.theme = currentIndex
                    }
                }
                RowLayout {
                    Label { text: qsTr("模式"); Layout.preferredWidth: 70 }
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: [qsTr("休闲"), qsTr("工作"), qsTr("勿扰")]
                        currentIndex: bridge.mode
                        onActivated: bridge.mode = currentIndex
                    }
                    Label {
                        text: bridge.mode === 2 ? qsTr("勿扰：暂停主动互动与待机动效")
                                                : qsTr("休闲模式会按频率主动说话")
                        color: bridge.secondaryTextColor
                        font.pixelSize: 12
                    }
                }
                CheckBox {
                    text: qsTr("隐藏桌宠时显示悬浮球")
                    checked: bridge.ballVisible
                    onToggled: bridge.ballVisible = checked
                }
                CheckBox {
                    text: qsTr("按前台应用自动切换模式（工作/休闲）")
                    checked: bridge.autoMode
                    onToggled: bridge.autoMode = checked
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("全局快捷键：Ctrl+Alt+P 呼出/隐藏桌宠 · Ctrl+Alt+C 打开聊天 · Ctrl+Alt+M 勿扰模式")
                    color: bridge.secondaryTextColor
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Button { text: qsTr("检查更新"); onClicked: bridge.checkUpdate() }
                    Button {
                        text: qsTr("重新显示新手引导")
                        onClicked: onboarding.open()
                    }
                    Item { Layout.fillWidth: true }
                }
                Label {
                    Layout.fillWidth: true
                    text: bridge.updateResult
                    visible: bridge.updateResult !== ""
                    color: bridge.secondaryTextColor
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("提示：更新地址在 config.json 的 updateUrl 填写（JSON：version/url/notes）。")
                    color: bridge.secondaryTextColor
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }
                Item { Layout.fillHeight: true }
            }

            // ================= 关于 =================
            ColumnLayout {
                spacing: 10
                Label {
                    Layout.fillWidth: true
                    text: qsTr("桌面桌宠 %1（原型）").arg(bridge.appVersion)
                    font.pixelSize: 18
                    font.bold: true
                    color: bridge.textColor
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: bridge.secondaryTextColor
                    text: qsTr("Windows 单平台 · Qt 6.12 + C++23 · stacklog 日志 · singleproc 单实例\n"
                               + "聊天支持 OpenAI 兼容接口（Ollama / LM Studio / DeepSeek / OpenAI / OpenCode Go / 自定义）。")
                }
                RowLayout {
                    Label { text: qsTr("桌宠数量") }
                    ComboBox {
                        Layout.preferredWidth: 120
                        model: ["1 只", "2 只", "3 只"]
                        currentIndex: bridge.petCount - 1
                        onActivated: bridge.petCount = currentIndex + 1
                    }
                }
                CheckBox {
                    text: qsTr("贴边隐藏（顶部不隐藏）")
                    checked: bridge.edgeHide
                    onToggled: bridge.edgeHide = checked
                }
                CheckBox {
                    text: qsTr("开机自动启动")
                    checked: bridge.autoStart
                    onToggled: bridge.autoStart = checked
                }
                CheckBox {
                    text: qsTr("透明区域点击穿透")
                    checked: bridge.clickThrough
                    onToggled: bridge.clickThrough = checked
                }
                CheckBox {
                    text: qsTr("待机浮动")
                    checked: bridge.idleMotion
                    onToggled: bridge.idleMotion = checked
                }
                RowLayout {
                    Label { text: qsTr("浮动幅度") }
                    Slider {
                        Layout.preferredWidth: 200
                        from: 0.0
                        to: 3.0
                        stepSize: 0.1
                        value: bridge.idleAmplitude
                        onMoved: bridge.idleAmplitude = value
                    }
                    Label { text: Number(bridge.idleAmplitude).toFixed(1) }
                }
                CheckBox {
                    text: qsTr("显示命中区（调试）")
                    checked: bridge.debugHitAreas
                    onToggled: bridge.debugHitAreas = checked
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: bridge.secondaryTextColor
                    font.pixelSize: 11
                    text: qsTr("许可：Qt 6（LGPLv3）· SQLite（Public Domain）· stacklog / singleproc / cxx 工具（MIT）")
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    Component.onCompleted: {
        refreshFields()
        if (!bridge.onboardingDone)
            onboarding.open()
    }

    function refreshFields() {
        baseUrlField.text = bridge.baseUrl
        apiKeyField.text = bridge.apiKey
        modelField.text = bridge.modelName
        const index = bridge.presets.indexOf(bridge.presetName)
        presetBox.currentIndex = index >= 0 ? index : bridge.presets.length - 1
    }

    Connections {
        target: bridge
        function onSettingsChanged() { root.refreshFields() }
    }
}
