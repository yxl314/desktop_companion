//! \file BubbleWindow.qml
//! \brief 桌宠气泡窗口：显示 petVM.bubbleText，位于当前桌宠上方，点击打开聊天。
//!
//! 数据绑定（§7.14）：
//!   petVM.bubbleVisible / petVM.bubbleText / petVM.pets / petVM.activePetIndex
//! 命令：hideBubble()
//!
//! 交互：点击气泡 = 关闭气泡 + 切到聊天页 + 显示主窗口。
//!
//! \note 窗口透明、无边框、置顶；不做点击穿透（气泡本身需要可点击）。
//! \note objectName = "bubbleWindow"，供 WindowRegistry 按名查找。

import QtQuick
import "../theme"

Window {
    id: root

    objectName: "bubbleWindow"
    flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 300
    height: 110
    title: qsTr("桌宠气泡")

    readonly property var activePet: (petVM.pets.length > 0
                                      && petVM.activePetIndex >= 0
                                      && petVM.activePetIndex < petVM.pets.length)
                                     ? petVM.pets[petVM.activePetIndex] : null

    visible: petVM.bubbleVisible && petVM.bubbleText.length > 0

    //! 屏幕可用区域（离屏/早期初始化时回退到 1920×1080）
    function screenGeometry() {
        if (typeof screen !== "undefined" && screen !== null
                && screen.availableGeometry !== undefined
                && screen.availableGeometry.width > 0)
            return screen.availableGeometry
        return Qt.rect(0, 0, 1920, 1080)
    }

    x: {
        const geometry = root.screenGeometry()
        const pet = root.activePet
        const base = (pet && pet.x !== undefined)
                     ? pet.x + 16
                     : geometry.x + geometry.width - root.width - 120
        return Math.max(geometry.x + 8,
                        Math.min(base, geometry.x + geometry.width - root.width - 8))
    }
    y: {
        const geometry = root.screenGeometry()
        const pet = root.activePet
        const base = (pet && pet.y !== undefined)
                     ? pet.y - root.height + 28
                     : geometry.y + geometry.height - root.height - 160
        return Math.max(geometry.y + 8,
                        Math.min(base, geometry.y + geometry.height - root.height - 8))
    }

    // ---- 气泡卡片 + 下方小尾巴 -------------------------------------------
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 6
        radius: Theme.radiusPanel
        color: Theme.bgPopover
        border.width: 1
        border.color: Theme.border

        Text {
            anchors.fill: parent
            anchors.margins: Theme.sp3
            text: petVM.bubbleText
            color: Theme.text1
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsBody
            lineHeight: 1.4
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
        }
    }

    Rectangle {
        width: 14
        height: 14
        rotation: 45
        color: Theme.bgPopover
        border.width: 1
        border.color: Theme.border
        anchors.horizontalCenter: card.horizontalCenter
        anchors.bottom: card.bottom
        anchors.bottomMargin: -6
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            petVM.hideBubble()
            appVM.setPageIndex(0)
            appVM.showMainWindow()
        }
    }
}
