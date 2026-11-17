//! \file BallWindow.qml
//! \brief 悬浮球：隐藏桌宠时显示，点击恢复桌宠，可拖动。
//!
//! 数据绑定：appVM.ballVisible / appVM.petVisible；petVM.pets/activePetIndex（头像）。
//! 命令：appVM.setPetVisible(true)
//!
//! \note 位置通过 appVM.ballX / appVM.ballY 持久化；拖动结束后调用
//!       appVM.setBallPosition(x, y)。
//! \note objectName = "ballWindow"，供 WindowRegistry 按名查找。

import QtQuick
import "../theme"

Window {
    id: root

    objectName: "ballWindow"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 64
    height: 64
    title: qsTr("悬浮球")

    visible: appVM.ballVisible && !appVM.petVisible

    property real posX: 0
    property real posY: 0
    property bool positionReady: false
    property bool dragging: false

    readonly property var activePet: (petVM.pets.length > 0
                                      && petVM.activePetIndex >= 0
                                      && petVM.activePetIndex < petVM.pets.length)
                                     ? petVM.pets[petVM.activePetIndex] : null
    readonly property string ballAvatar: activePet !== null
                                         && activePet.avatar !== undefined
                                         ? activePet.avatar : ""

    x: posX
    y: posY

    function resolveSource(path) {
        if (!path || path.length === 0)
            return "qrc:/pet.png"
        if (path.indexOf("qrc:/") === 0 || path.indexOf("file://") === 0
                || path.indexOf("http://") === 0 || path.indexOf("https://") === 0)
            return path
        if (path.charAt(0) === "/")
            return "file://" + path
        if (/^[A-Za-z]:[\\/]/.test(path))
            return "file:///" + path.replace(/\\/g, "/")
        return "file:///" + path.replace(/\\/g, "/")
    }

    //! 屏幕可用区域（离屏/早期初始化时回退到 1920×1080）
    function screenGeometry() {
        if (typeof screen !== "undefined" && screen !== null
                && screen.availableGeometry !== undefined
                && screen.availableGeometry.width > 0)
            return screen.availableGeometry
        return Qt.rect(0, 0, 1920, 1080)
    }

    function placeDefault() {
        if (root.positionReady)
            return
        if (appVM.ballX > 0 || appVM.ballY > 0) {
            root.posX = appVM.ballX
            root.posY = appVM.ballY
        } else {
            const geometry = root.screenGeometry()
            root.posX = geometry.x + geometry.width - root.width - 24
            root.posY = geometry.y + geometry.height - root.height - 88
        }
        root.positionReady = true
    }

    //! 拖动结束后把位置写回 ViewModel（持久化）。
    function persistPosition() {
        appVM.setBallPosition(root.posX, root.posY)
    }

    Component.onCompleted: placeDefault()
    onVisibleChanged: placeDefault()

    // ---- 悬浮球外观 -------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.bgCard
        border.width: 1
        border.color: Theme.borderStrong
        opacity: ballHover.hovered ? 1.0 : 0.92

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.durationFast
            }
        }

        Image {
            id: ballImage
            anchors.fill: parent
            anchors.margins: 4
            source: root.resolveSource(root.ballAvatar)
            sourceSize: Qt.size(root.width * 2, root.height * 2)
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: status === Image.Ready
        }

        Text {
            anchors.centerIn: parent
            visible: ballImage.status !== Image.Ready
            text: qsTr("宠")
            color: Theme.accentText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsTitle
            font.weight: 600
        }
    }

    HoverHandler {
        id: ballHover
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor

        property real pressX: 0
        property real pressY: 0
        property real startX: 0
        property real startY: 0

        onPressed: function(mouse) {
            root.dragging = false
            pressX = mouse.x
            pressY = mouse.y
            startX = root.x
            startY = root.y
        }

        onPositionChanged: function(mouse) {
            if (!pressed)
                return
            if (!root.dragging
                    && Math.abs(mouse.x - pressX) + Math.abs(mouse.y - pressY) > 6)
                root.dragging = true
            if (root.dragging) {
                root.posX = startX + (mouse.x - pressX)
                root.posY = startY + (mouse.y - pressY)
            }
        }

        onReleased: {
            if (root.dragging) {
                root.dragging = false
                root.persistPosition()
            } else {
                appVM.setPetVisible(true)
            }
        }
    }
}
