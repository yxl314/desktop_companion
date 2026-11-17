import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    objectName: "chatWindow"
    width: 420
    height: 560
    visible: bridge.chatVisible
    title: qsTr("聊天 · 小柚")
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: "#faf9f6"
    onClosing: (close) => {
        close.accepted = false
        bridge.chatVisible = false
    }

    ListModel { id: messages }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: qsTr("回复由本地 mock 引擎生成（原型阶段）")
            color: "#999999"
            font.pixelSize: 12
        }

        Label {
            text: bridge.statusText
            color: "#7a6a50"
            font.pixelSize: 12
            visible: bridge.statusText !== ""
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: messages
            clip: true
            spacing: 8
            onCountChanged: positionViewAtEnd()

            delegate: Row {
                width: ListView.view.width
                layoutDirection: model.role === "user" ? Qt.RightToLeft : Qt.LeftToRight

                Rectangle {
                    width: Math.min(bubbleText.implicitWidth + 20, ListView.view.width * 0.72)
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextArea {
                id: input
                Layout.fillWidth: true
                placeholderText: qsTr("和小柚说点什么…（Enter 发送，Shift+Enter 换行）")
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
                onClicked: root.sendPending()
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
    }

    function sendPending() {
        const text = input.text.trim()
        if (text.length === 0)
            return
        input.clear()
        bridge.sendChat(text)
    }
}
