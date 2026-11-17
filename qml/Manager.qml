import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    objectName: "managerWindow"
    width: 560
    height: 640
    visible: bridge.managerVisible
    title: qsTr("设置 · 桌面桌宠 ") + bridge.appVersion
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: "#faf9f6"
    property int page: 0
    onClosing: (close) => {
        close.accepted = false
        bridge.managerVisible = false
    }

    function refreshFields() {
        baseUrlField.text = bridge.baseUrl
        apiKeyField.text = bridge.apiKey
        modelField.text = bridge.modelName
        const index = bridge.presets.indexOf(bridge.presetName)
        presetBox.currentIndex = index >= 0 ? index : bridge.presets.length - 1
    }

    Component.onCompleted: refreshFields()

    Connections {
        target: bridge
        function onSettingsChanged() { root.refreshFields() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Button { text: qsTr("模型"); flat: true; highlighted: root.page === 0; onClicked: root.page = 0 }
            Button { text: qsTr("人设"); flat: true; highlighted: root.page === 1; onClicked: root.page = 1 }
            Button { text: qsTr("关于"); flat: true; highlighted: root.page === 2; onClicked: root.page = 2 }
            Item { Layout.fillWidth: true }
            Label { text: qsTr("当前：") + bridge.modelSummary; color: "#8a7a60"; font.pixelSize: 12 }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.page

            // ---------- 模型设置 ----------
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
                    TextField {
                        id: baseUrlField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如 http://127.0.0.1:11434/v1")
                    }

                    Label { text: qsTr("API Key") }
                    TextField {
                        id: apiKeyField
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        placeholderText: qsTr("本地模型可留空")
                    }

                    Label { text: qsTr("模型") }
                    TextField {
                        id: modelField
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如 qwen2.5:7b / deepseek-chat")
                    }
                }

                RowLayout {
                    spacing: 8
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
                    Label {
                        id: saveHint
                        text: ""
                        color: "#6cbf8a"
                        font.pixelSize: 12
                    }
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
                    text: qsTr("提示：Ollama 需先 `ollama serve`；LM Studio 需开启本地服务；云端 API 需填 Key。\n"
                              + "保存后立即生效，聊天窗口会改用真实模型；不配置时回退到本地 mock。")
                    wrapMode: Text.Wrap
                    color: "#a08a66"
                    font.pixelSize: 12
                }

                Item { Layout.fillHeight: true }
            }

            // ---------- 人设 ----------
            ColumnLayout {
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: qsTr("当前人设：") + bridge.personaName
                    color: "#4a3b2e"
                    font.pixelSize: 14
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
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Label {
                                text: modelData.name
                                color: "#4a3b2e"
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Label {
                                text: qsTr("使用中")
                                visible: modelData.current
                                color: "#4f9e6d"
                                font.pixelSize: 12
                            }
                            Button {
                                text: qsTr("使用")
                                enabled: !modelData.current
                                onClicked: bridge.selectPersona(modelData.id)
                            }
                            Button {
                                text: qsTr("删除")
                                visible: !modelData.builtin
                                onClicked: bridge.removePersona(modelData.id)
                            }
                        }
                    }
                }

                RowLayout {
                    Button {
                        text: qsTr("导入 .md 人设")
                        onClicked: bridge.importPersona()
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: bridge.statusText
                        color: "#7a6a50"
                        font.pixelSize: 12
                    }
                }
            }

            // ---------- 关于 ----------
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
                              + "聊天支持 OpenAI 兼容接口（Ollama / LM Studio / DeepSeek / OpenAI / 自定义）。")
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: "#8a7a60"
                    font.pixelSize: 12
                    text: qsTr("数据目录：") + bridge.dataDir
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: "#8a7a60"
                    font.pixelSize: 12
                    text: qsTr("日志目录：") + bridge.logDir
                }
                RowLayout {
                    Button { text: qsTr("打开数据目录"); onClicked: bridge.openDataFolder() }
                    Button { text: qsTr("打开日志目录"); onClicked: bridge.openLogFolder() }
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
}
