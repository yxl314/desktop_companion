import QtQuick

Window {
    id: root
    objectName: "petWindow"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 240
    height: 320
    visible: bridge.petVisible
    x: bridge.petX
    y: bridge.petY

    Item {
        id: bob
        width: parent.width
        height: parent.height
        y: 0

        Image {
            id: petImage
            anchors.fill: parent
            source: bridge.avatarPath
            fillMode: Image.PreserveAspectFit
            smooth: true
            transformOrigin: Item.Bottom
            rotation: dragArea.dragging ? -8 : 0
            scale: dragArea.dragging ? 1.04 : 1.0
            Behavior on rotation { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
        }
    }

    SequentialAnimation {
        running: bridge.idleMotion
        loops: Animation.Infinite
        NumberAnimation {
            target: bob
            property: "y"
            to: -5 * bridge.idleAmplitude
            duration: 1400
            easing.type: Easing.InOutSine
        }
        NumberAnimation {
            target: bob
            property: "y"
            to: 0
            duration: 1400
            easing.type: Easing.InOutSine
        }
    }

    SequentialAnimation {
        id: tapAnimation
        NumberAnimation { target: petImage; property: "scale"; from: 1.0; to: 1.1; duration: 90; easing.type: Easing.OutQuad }
        NumberAnimation { target: petImage; property: "scale"; to: 1.0; duration: 140; easing.type: Easing.OutBack }
    }

    // 命中区调试：叠加 meta.json 的归一化矩形。
    Item {
        width: root.width
        height: root.height
        visible: bridge.debugHitAreas
        Rectangle {
            anchors.fill: parent
            color: "#2200ff00"
            border.color: "#cc00aa00"
            border.width: 2
        }
        Component.onCompleted: console.log("debug hit areas visible=" + bridge.debugHitAreas
                                            + " rects=" + bridge.hitAreaRects().length)
        Repeater {
            model: bridge.hitAreaRects()
            delegate: Rectangle {
                x: modelData.x * root.width
                y: modelData.y * root.height
                width: modelData.w * root.width
                height: modelData.h * root.height
                color: "#33e74c3c"
                border.color: "#e74c3c"
                border.width: 2
                z: 100
                Text {
                    text: modelData.id
                    color: "#e74c3c"
                    font.pixelSize: 10
                    x: 2
                    y: 2
                }
            }
        }
    }

    MouseArea {
        id: dragArea
        width: root.width
        height: root.height
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property bool dragging: false
        property real pressX: 0
        property real pressY: 0

        onPressed: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                bridge.showPetMenu()
                return
            }
            dragging = false
            pressX = mouse.x
            pressY = mouse.y
        }

        onPositionChanged: (mouse) => {
            if (!pressed)
                return
            if (!dragging && Math.abs(mouse.x - pressX) + Math.abs(mouse.y - pressY) > 8)
                dragging = true
            if (dragging) {
                root.x += mouse.x - pressX
                root.y += mouse.y - pressY
            }
        }

        onReleased: (mouse) => {
            if (mouse.button === Qt.RightButton)
                return
            if (dragging) {
                bridge.setPetPosition(root.x, root.y)
                dragging = false
            } else {
                tapAnimation.restart()
                bridge.petClicked(pressX, pressY, root.width, root.height)
            }
        }

        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                bridge.openChat()
        }
    }
}
