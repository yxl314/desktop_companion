import QtQuick

Window {
    id: root
    objectName: "bubbleWindow"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
    color: "transparent"
    width: 280
    height: 96
    visible: bridge.bubbleVisible
    x: bridge.bubbleX
    y: bridge.bubbleY

    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        radius: 14
        color: "#fffdf7"
        border.color: "#e8ddc8"
        border.width: 1

        Text {
            anchors.fill: parent
            anchors.margins: 12
            text: bridge.bubbleText
            wrapMode: Text.Wrap
            font.pixelSize: 15
            color: "#333333"
            verticalAlignment: Text.AlignVCenter
        }
    }
}
