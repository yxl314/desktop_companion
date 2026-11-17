//! \file PetWindow.qml
//! \brief 桌宠窗口（每个桌宠一个实例，petIndex 由 C++ 创建时注入）。
//!
//! 数据绑定（§7.11 / §7.14）：
//!   petVM.pets[petIndex]（index,characterId,name,avatar,x,y,visible,scale）、
//!   petVM.petCount / idleMotion / idleAmplitude / clickThrough / edgeHide / debugHitAreas；
//!   appVM.petVisible（总开关）、appVM.mode（勿扰模式暂停待机动效）。
//! 命令：movePet(index,x,y) / tapPet(index,x,y) / doubleTapPet(index) /
//!       contextMenu(index) / dragFinished(index)。
//!
//! 保留行为：
//!   - 拖拽三态（按下 / 拖动 / 点击），拖动结束回写位置并触发 dragFinished；
//!   - 贴边隐藏（拖到左右边缘收起，顶部不隐藏），悬停恢复；
//!   - 透明区域点击穿透由 C++（petwindowhelper 按 alpha 生成窗口区域）处理；
//!   - 命中区调试叠加（依赖 PetViewModel 提供 hitAreaRects，见契约缺项报告）；
//!   - 右键 / 长按弹 C++ 菜单，双击打开主窗口聊天。
//!
//! \note objectName 规则：第一只 = "petWindow"，其余 = "petWindow1"/"petWindow2"，
//!       供 WindowRegistry / 命中区遮罩按名查找。


import QtQuick
import "../theme"
import "../components"

Window {
    id: root

    property int petIndex: 0

    objectName: petIndex === 0 ? "petWindow" : "petWindow" + petIndex
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 240
    height: 320
    title: qsTr("桌宠")

    // ---- 当前桌宠数据行 ---------------------------------------------------
    readonly property var petData: petVM.pets.length > petIndex ? petVM.pets[petIndex] : null
    readonly property bool hasPet: petData !== null && petData !== undefined
    readonly property string petName: hasPet && petData.name !== undefined ? petData.name : ""
    readonly property string petAvatar: hasPet && petData.avatar !== undefined ? petData.avatar : ""

    // ---- 窗口位置（本地维护，避免拖拽破坏 VM 绑定）-------------------------
    property real posX: 0
    property real posY: 0
    property bool edgeHidden: false
    property real restoreX: 0

    // ---- 命中区调试矩形（归一化坐标）--------------------------------------
    property var debugAreas: []

    x: posX
    y: posY

    visible: appVM.petVisible && petIndex < petVM.petCount
             && (!hasPet || petData.visible === undefined || petData.visible !== false)

    //! 把路径转成 Image 可识别的 URL。
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

    //! 从 ViewModel 同步窗口位置（拖拽中 / 贴边隐藏中不打断）。
    function syncPosition() {
        if (dragArea.dragging || edgeHidden || !hasPet)
            return
        if (petData.x !== undefined)
            posX = petData.x
        if (petData.y !== undefined)
            posY = petData.y
    }

    //! 拖动结束后检查是否贴边收起（顶部不隐藏，避免误触）。
    function checkEdgeHide() {
        if (!petVM.edgeHide)
            return
        if (typeof screen === "undefined" || screen === null
                || screen.availableGeometry === undefined)
            return
        const geometry = screen.availableGeometry
        if (root.y <= geometry.y + 6)
            return
        if (root.x < geometry.x + 8) {
            edgeHidden = true
            restoreX = geometry.x
            posX = geometry.x - root.width * 0.72
        } else if (root.x + root.width > geometry.x + geometry.width - 8) {
            edgeHidden = true
            restoreX = geometry.x + geometry.width - root.width
            posX = geometry.x + geometry.width - root.width * 0.28
        }
    }

    //! 鼠标移入时从贴边状态恢复。
    function restoreFromEdge() {
        if (!edgeHidden)
            return
        edgeHidden = false
        posX = restoreX
    }

    //! 刷新命中区调试数据（契约未列 hitAreaRects 时保持为空）。
    function refreshDebugAreas() {
        if (!petVM.debugHitAreas) {
            debugAreas = []
            return
        }
        if (typeof petVM.hitAreaRects === "function") {
            const rects = petVM.hitAreaRects(root.petIndex)
            debugAreas = Array.isArray(rects) ? rects : []
        } else {
            debugAreas = []
        }
    }

    Component.onCompleted: {
        syncPosition()
        refreshDebugAreas()
    }

    // ---- 形象与待机动效 ---------------------------------------------------
    Item {
        id: bob
        width: root.width
        height: root.height
        y: 0

        Image {
            id: petImage
            anchors.fill: parent
            source: root.resolveSource(root.petAvatar)
            sourceSize: Qt.size(root.width * 2, root.height * 2)
            fillMode: Image.PreserveAspectFit
            smooth: true
            transformOrigin: Item.Bottom
            rotation: dragArea.dragging ? -8 : 0
            scale: dragArea.dragging ? 1.04 : 1.0
            Behavior on rotation {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutQuad
                }
            }
            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutQuad
                }
            }
        }

        // 形象缺失时的占位（避免桌宠完全不可见 / 无法点击）
        Rectangle {
            anchors.centerIn: parent
            width: 96
            height: 96
            radius: 48
            color: Theme.accentSoft
            border.width: 1
            border.color: Theme.border
            visible: petImage.status !== Image.Ready

            Text {
                anchors.centerIn: parent
                text: root.petName.length > 0 ? root.petName.charAt(0) : "?"
                color: Theme.accentText
                font.family: Theme.fontFamily
                font.pixelSize: 24
                font.weight: 600
            }
        }
    }

    SequentialAnimation {
        running: petVM.idleMotion && appVM.mode !== 2 && root.visible
        loops: Animation.Infinite

        NumberAnimation {
            target: bob
            property: "y"
            to: -5 * petVM.idleAmplitude
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

        NumberAnimation {
            target: petImage
            property: "scale"
            from: 1.0
            to: 1.1
            duration: 90
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            target: petImage
            property: "scale"
            to: 1.0
            duration: 140
            easing.type: Easing.OutBack
        }
    }

    // ---- 命中区调试叠加 ---------------------------------------------------
    Item {
        anchors.fill: parent
        visible: petVM.debugHitAreas && root.debugAreas.length > 0
        z: 100

        Repeater {
            model: root.debugAreas

            delegate: Rectangle {
                x: (modelData.x === undefined ? 0 : modelData.x) * root.width
                y: (modelData.y === undefined ? 0 : modelData.y) * root.height
                width: (modelData.w === undefined ? 0 : modelData.w) * root.width
                height: (modelData.h === undefined ? 0 : modelData.h) * root.height
                color: "transparent"
                border.width: 2
                border.color: Theme.danger

                Text {
                    x: 2
                    y: 2
                    text: modelData.id === undefined ? "" : modelData.id
                    color: Theme.dangerText
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fsBadge
                }
            }
        }
    }

    // ---- 交互：拖拽三态 / 点击 / 右键 / 双击 ------------------------------
    MouseArea {
        id: dragArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        property bool dragging: false
        property real pressX: 0
        property real pressY: 0
        property real startX: 0
        property real startY: 0

        onEntered: root.restoreFromEdge()

        onPressed: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                petVM.contextMenu(root.petIndex)
                return
            }
            if (root.edgeHidden)
                root.restoreFromEdge()
            dragging = false
            pressX = mouse.x
            pressY = mouse.y
            startX = root.x
            startY = root.y
        }

        onPositionChanged: function(mouse) {
            if (!pressed)
                return
            if (!dragging && Math.abs(mouse.x - pressX) + Math.abs(mouse.y - pressY) > 8)
                dragging = true
            if (dragging) {
                root.posX = startX + (mouse.x - pressX)
                root.posY = startY + (mouse.y - pressY)
            }
        }

        onReleased: function(mouse) {
            if (mouse.button === Qt.RightButton)
                return
            if (dragging) {
                dragging = false
                petVM.movePet(root.petIndex, root.posX, root.posY)
                petVM.dragFinished(root.petIndex)
                root.checkEdgeHide()
            } else {
                tapAnimation.restart()
                petVM.tapPet(root.petIndex, pressX, pressY)
            }
        }

        onPressAndHold: function(mouse) {
            if (mouse.button === Qt.LeftButton)
                petVM.contextMenu(root.petIndex)
        }

        onDoubleClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton)
                petVM.doubleTapPet(root.petIndex)
        }
    }

    Connections {
        target: petVM

        function onPetsChanged() {
            root.syncPosition()
            root.refreshDebugAreas()
        }
        function onDebugHitAreasChanged() {
            root.refreshDebugAreas()
        }
    }
}
