import QtQuick

Window {
    id: root
    property int petIndex: 0
    objectName: petIndex === 0 ? "petWindow" : "petWindow" + petIndex
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 240
    height: 320
    visible: bridge.petVisible && petIndex < bridge.petCount
    x: {
        const list = bridge.pets
        return petIndex < list.length ? list[petIndex].x : 0
    }
    y: {
        const list = bridge.pets
        return petIndex < list.length ? list[petIndex].y : 0
    }

    property bool edgeHidden: false
    property int hiddenSide: 0
    property real restoreX: 0
    property real restoreY: 0

    function checkEdgeHide() {
        if (!bridge.edgeHide)
            return
        const g = screen.availableGeometry
        // 顶部不隐藏，避免与全屏/最大化窗口误触。
        if (root.y <= g.y + 6)
            return
        if (root.x < g.x + 8) {
            hiddenSide = -1
            restoreX = g.x
            edgeHidden = true
            root.x = g.x - root.width * 0.72
        } else if (root.x + root.width > g.x + g.width - 8) {
            hiddenSide = 1
            restoreX = g.x + g.width - root.width
            edgeHidden = true
            root.x = g.x + g.width - root.width * 0.28
        }
    }

    function restoreFromEdge() {
        if (!edgeHidden)
            return
        edgeHidden = false
        root.x = restoreX
    }

    Item {
        id: bob
        width: parent.width
        height: parent.height
        y: 0

        Image {
            id: petImage
            anchors.fill: parent
            source: bridge.pets.length > petIndex ? bridge.pets[petIndex].avatar : bridge.avatarPath
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
        running: bridge.idleMotion && bridge.mode !== 2
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
        hoverEnabled: true
        property bool dragging: false
        property real pressX: 0
        property real pressY: 0

        onEntered: root.restoreFromEdge()

        onPressed: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                bridge.showPetMenu(root.petIndex)
                return
            }
            if (root.edgeHidden)
                root.restoreFromEdge()
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
                bridge.setPetPosition(root.petIndex, root.x, root.y)
                dragging = false
                root.checkEdgeHide()
            } else {
                tapAnimation.restart()
                bridge.petClicked(root.petIndex, pressX, pressY, root.width, root.height)
            }
        }

        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                bridge.openChat()
        }
    }
}
