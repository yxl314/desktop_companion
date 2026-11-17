import QtQuick

Window {
    id: root
    objectName: "ballWindow"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 64
    height: 64
    visible: bridge.ballVisible && !bridge.petVisible
    x: bridge.ballX
    y: bridge.ballY

    Image {
        anchors.fill: parent
        source: bridge.avatarPath
        fillMode: Image.PreserveAspectFit
        smooth: true
        opacity: 0.92
    }

    MouseArea {
        id: dragArea
        width: root.width
        height: root.height
        property bool dragging: false
        property real pressX: 0
        property real pressY: 0

        onPressed: (mouse) => {
            dragging = false
            pressX = mouse.x
            pressY = mouse.y
        }
        onPositionChanged: (mouse) => {
            if (!pressed)
                return
            if (!dragging && Math.abs(mouse.x - pressX) + Math.abs(mouse.y - pressY) > 6)
                dragging = true
            if (dragging) {
                root.x += mouse.x - pressX
                root.y += mouse.y - pressY
            }
        }
        onClicked: {
            if (!dragging)
                bridge.setPetVisible(true)
        }
        onReleased: {
            if (dragging)
                bridge.setBallPosition(root.x, root.y)
        }
    }
}
