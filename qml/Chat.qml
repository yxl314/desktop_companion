import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
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
    color: bridge.windowColor
    Material.theme: bridge.darkTheme ? Material.Dark : Material.Light
    Material.accent: "#6cbf8a"
    onClosing: (close) => {
        close.accepted = false
        bridge.chatVisible = false
    }

    ListModel { id: messages }

    Dialog {
        id: clearConfirm
        title: qsTr("清空对话")
        width: 360
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: bridge.clearConversation()
        contentItem: Label { text: qsTr("确定清空当前对话的聊天记录吗？") }
    }

    Dialog {
        id: restartDialog
        title: qsTr("重启对话")
        width: 380
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: bridge.restartConversation()
        contentItem: Label {
            wrapMode: Text.Wrap
            text: qsTr("重启对话会保留聊天记录，但重置当前人设的记忆与上下文，确定继续吗？")
        }
    }

    Dialog {
        id: backtrackDialog
        property int targetIndex: -1
        title: qsTr("回溯对话")
        width: 380
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: bridge.backtrackToIndex(targetIndex)
        contentItem: Label {
            wrapMode: Text.Wrap
            text: qsTr("回溯到这条消息，其后的消息与关联记忆会被清除，确定继续吗？")
        }
    }

    Component.onCompleted: {
        const history = bridge.initialMessages()
        for (let i = 0; i < history.length; ++i)
            messages.append({ "role": history[i].role, "text": history[i].text,
                              "id": history[i].id, "feedback": history[i].feedback })
    }

    ColumnLayout {
        width: root.width - 28
        height: root.height - 28
        x: 14
        y: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: bridge.personaName
                font.pixelSize: 16
                font.bold: true
                color: bridge.textColor
            }
            Item { Layout.fillWidth: true }
            Label {
                text: bridge.modelSummary
                font.pixelSize: 12
                color: bridge.secondaryTextColor
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                text: qsTr("清空对话")
                flat: true
                onClicked: clearConfirm.open()
            }
            Button {
                text: qsTr("重启对话")
                flat: true
                onClicked: restartDialog.open()
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
            color: bridge.secondaryTextColor
            font.pixelSize: 12
        }

        Label {
            text: bridge.statusText
            visible: bridge.statusText !== ""
            color: bridge.secondaryTextColor
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

                delegate: Item {
                    width: list.width
                    height: bubbleColumn.height

                    Column {
                        id: bubbleColumn
                        width: parent.width
                        spacing: 2

                    Row {
                        id: bubbleRow
                        width: parent.width
                        layoutDirection: model.role === "user" ? Qt.RightToLeft : Qt.LeftToRight

                        Rectangle {
                            width: Math.min(bubbleText.implicitWidth + 20, list.width * 0.72)
                            height: bubbleText.implicitHeight + 16
                            radius: 10
                            color: model.role === "user" ? "#8fd3a8" : bridge.cardColor
                            border.color: model.role === "user" ? "#6cbf8a" : bridge.borderColor
                            border.width: 1

                            Text {
                                id: bubbleText
                                width: parent.width - 20
                                anchors.centerIn: parent
                                text: model.text
                                wrapMode: Text.Wrap
                                color: bridge.textColor
                                font.pixelSize: 14
                            }
                        }
                    }

                    Row {
                        visible: model.role !== "user"
                        spacing: 2
                        Button {
                            text: "👍"
                            flat: true
                            font.pixelSize: 11
                            opacity: model.feedback > 0 ? 1.0 : 0.5
                            onClicked: {
                                messages.setProperty(index, "feedback", 1)
                                bridge.setMessageFeedback(messages.get(index).id, 1)
                            }
                        }
                        Button {
                            text: "👎"
                            flat: true
                            font.pixelSize: 11
                            opacity: model.feedback < 0 ? 1.0 : 0.5
                            onClicked: {
                                messages.setProperty(index, "feedback", -1)
                                bridge.setMessageFeedback(messages.get(index).id, -1)
                            }
                        }
                    }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: {
                            backtrackDialog.targetIndex = index
                            backtrackDialog.open()
                        }
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 10
                visible: messages.count === 0
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("和小柚说点什么吧～")
                    color: bridge.secondaryTextColor
                    font.pixelSize: 14
                }
                Row {
                    spacing: 6
                    Repeater {
                        model: [qsTr("介绍一下你自己"), qsTr("今天有什么好玩的"), qsTr("讲个冷笑话")]
                        delegate: Button {
                            text: modelData
                            flat: true
                            onClicked: input.text = modelData
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: errorLabel.text !== ""
            Label {
                id: errorLabel
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: "#c0392b"
                font.pixelSize: 12
            }
            Button {
                text: qsTr("重试")
                visible: errorLabel.text !== ""
                onClicked: {
                    errorLabel.text = ""
                    bridge.retryChat()
                }
            }
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
            messages.append({ "role": role, "text": text, "id": -1, "feedback": 0 })
        }
        function onAssistantStored(messageId) {
            if (messages.count > 0 && messages.get(messages.count - 1).role === "assistant")
                messages.setProperty(messages.count - 1, "id", messageId)
        }
        function onChatDelta(text) {
            if (messages.count === 0 || messages.get(messages.count - 1).role !== "assistant")
                messages.append({ "role": "assistant", "text": "", "id": -1, "feedback": 0 })
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
            const history = bridge.initialMessages()
            for (let i = 0; i < history.length; ++i)
                messages.append({ "role": history[i].role, "text": history[i].text,
                              "id": history[i].id, "feedback": history[i].feedback })
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
