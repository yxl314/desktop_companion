import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    objectName: "chatWindow"
    width: 460
    height: 640
    visible: bridge.chatVisible
    title: qsTr("聊天 · ") + bridge.personaName
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: "#faf9f6"
    onClosing: (close) => {
        close.accepted = false
        bridge.chatVisible = false
    }

    ListModel { id: messages }

    Component.onCompleted: {
        const history = bridge.initialMessages()
        for (let i = 0; i < history.length; ++i)
            messages.append({ "role": history[i].role, "text": history[i].text })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: bridge.personaName
                font.pixelSize: 16
                font.bold: true
                color: "#4a3b2e"
            }
            Item { Layout.fillWidth: true }
            Label {
                text: bridge.modelSummary
                font.pixelSize: 12
                color: "#8a7a60"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                text: qsTr("清空对话")
                flat: true
                onClicked: {
                    bridge.clearConversation()
                    messages.clear()
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("设置")
                flat: true
                onClicked: bridge.openManager()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("未配置模型，当前为本地 mock 回复。点「设置」接入 OpenAI 兼容 API。")
            visible: !bridge.llmReady
            wrapMode: Text.Wrap
            color: "#a08a66"
            font.pixelSize: 12
        }

        Label {
            text: bridge.statusText
            visible: bridge.statusText !== ""
            color: "#7a6a50"
            font.pixelSize: 12
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                anchors.fill: parent
                model: messages
                clip: true
                spacing: 8
                onCountChanged: positionViewAtEnd()

                delegate: Row {
                    width: list.width
                    layoutDirection: model.role === "user" ? Qt.RightToLeft : Qt.LeftToRight

                    Rectangle {
                        width: Math.min(bubbleText.implicitWidth + 20, list.width * 0.72)
                        height: bubbleText.implicitHeight + 16
                        radius: 10
                        color: model.role === "user" ? "#8fd3a8" : "#ffffff"
                        border.color: model.role === "user" ? "#6cbf8a" : "#e2ded6"
                        border.width: 1

                        Text {
                            id: bubbleText
                            width: parent.width - 20
                            anchors.centerIn: parent
                            text: model.text
                            wrapMode: Text.Wrap
                            color: "#333333"
                            font.pixelSize: 14
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: messages.count === 0
                text: qsTr("和小柚说点什么吧～")
                color: "#b5a88e"
                font.pixelSize: 14
            }
        }

        Label {
            id: errorLabel
            Layout.fillWidth: true
            visible: text !== ""
            wrapMode: Text.Wrap
            color: "#c0392b"
            font.pixelSize: 12
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextArea {
                id: input
                Layout.fillWidth: true
                placeholderText: qsTr("说点什么…（Enter 发送）")
                wrapMode: TextArea.Wrap
                font.pixelSize: 14
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                        root.sendPending()
                        event.accepted = true
                    }
                }
            }

            Button {
                text: qsTr("发送")
                enabled: input.text.trim().length > 0 && !bridge.busy
                onClicked: root.sendPending()
            }

            Button {
                text: qsTr("停止")
                visible: bridge.busy
                onClicked: bridge.stopReply()
            }
        }
    }

    Connections {
        target: bridge
        function onChatMessage(role, text) {
            messages.append({ "role": role, "text": text })
        }
        function onChatDelta(text) {
            if (messages.count === 0 || messages.get(messages.count - 1).role !== "assistant")
                messages.append({ "role": "assistant", "text": "" })
            const index = messages.count - 1
            messages.setProperty(index, "text", messages.get(index).text + text)
        }
        function onChatError(message) {
            errorLabel.text = message
        }
        function onChatDone() {
            if (errorLabel.text === "")
                errorLabel.text = ""
        }
        function onConversationCleared() {
            messages.clear()
            errorLabel.text = ""
        }
        function onChatStateChanged(state) {
            if (state === "thinking" || state === "streaming")
                errorLabel.text = ""
        }
    }

    function sendPending() {
        const text = input.text.trim()
        if (text.length === 0)
            return
        input.clear()
        bridge.sendChat(text)
    }
}
