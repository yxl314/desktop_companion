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
            source: "qrc:/pet.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            transformOrigin: Item.Bottom
            rotation: dragArea.dragging ? -6 : 0
        }
    }

    SequentialAnimation {
        running: true
        loops: Animation.Infinite
        NumberAnimation { target: bob; property: "y"; to: -5; duration: 1400; easing.type: Easing.InOutSine }
        NumberAnimation { target: bob; property: "y"; to: 0; duration: 1400; easing.type: Easing.InOutSine }
    }

    SequentialAnimation {
        id: tapAnimation
        NumberAnimation { target: petImage; property: "scale"; from: 1.0; to: 1.1; duration: 90; easing.type: Easing.OutQuad }
        NumberAnimation { target: petImage; property: "scale"; to: 1.0; duration: 140; easing.type: Easing.OutBack }
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
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
                bridge.petClicked(root.x, root.y)
            }
        }
    }
}
