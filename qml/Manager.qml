import QtQuick
import QtQuick.Controls
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
    color: "#faf9f6"

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
            Label { text: inputDialog.prompt; color: "#4a3b2e" }
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: pageBox
                Layout.preferredWidth: 200
                model: ["模型", "角色", "人设", "世界观", "数据", "关于"]
                currentIndex: bridge.managerPage
                onActivated: bridge.managerPage = currentIndex
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("当前角色：") + bridge.currentCharacterName
                color: "#8a7a60"
                font.pixelSize: 12
            }
        }

        Label {
            Layout.fillWidth: true
            visible: bridge.statusText !== ""
            text: bridge.statusText
            color: "#7a6a50"
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
                    color: "#7a6a50"
                    font.pixelSize: 12
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: "#a08a66"
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
                        color: modelData.current ? "#e8f5ec" : "#ffffff"
                        border.color: modelData.current ? "#8fd3a8" : "#e2ded6"
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
                                Label { text: modelData.name; color: "#4a3b2e"; font.pixelSize: 15 }
                                Label {
                                    text: modelData.worldId ? qsTr("世界观：") + modelData.worldId : qsTr("未绑定世界观")
                                    color: "#8a7a60"; font.pixelSize: 11
                                }
                            }
                            Label { text: qsTr("使用中"); visible: modelData.current; color: "#4f9e6d"; font.pixelSize: 12 }
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
                Label {
                    Layout.fillWidth: true
                    text: qsTr("当前人设：") + bridge.personaName
                    color: "#4a3b2e"
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
                        color: modelData.current ? "#e8f5ec" : "#ffffff"
                        border.color: modelData.current ? "#8fd3a8" : "#e2ded6"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: "#4a3b2e"
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
                        color: "#ffffff"
                        border.color: "#e2ded6"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: "#4a3b2e"
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

            // ================= 数据 =================
            ColumnLayout {
                spacing: 10
                Label { text: qsTr("数据目录"); color: "#4a3b2e" }
                Label { text: bridge.dataDir; color: "#8a7a60"; font.pixelSize: 12; wrapMode: Text.Wrap }
                Label { text: qsTr("日志目录"); color: "#4a3b2e" }
                Label { text: bridge.logDir; color: "#8a7a60"; font.pixelSize: 12; wrapMode: Text.Wrap }
                RowLayout {
                    Button { text: qsTr("导出聊天记录"); onClicked: bridge.exportChatHistory() }
                    Button { text: qsTr("导出日志"); onClicked: bridge.exportLogs() }
                    Button { text: qsTr("打开数据目录"); onClicked: bridge.openDataFolder() }
                    Button { text: qsTr("打开日志目录"); onClicked: bridge.openLogFolder() }
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
                    color: "#4a3b2e"
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: "#6b5c48"
                    text: qsTr("Windows 单平台 · Qt 6.12 + C++23 · stacklog 日志 · singleproc 单实例\n"
                               + "聊天支持 OpenAI 兼容接口（Ollama / LM Studio / DeepSeek / OpenAI / OpenCode Go / 自定义）。")
                }
                CheckBox {
                    text: qsTr("开机自动启动")
                    checked: bridge.autoStart
                    onToggled: bridge.autoStart = checked
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    Component.onCompleted: refreshFields()

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
