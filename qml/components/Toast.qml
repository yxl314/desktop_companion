//! \file Toast.qml
//! \brief 轻提示：340 宽、10 圆角、边框 + 轻阴影，3.2s 自动淡出。
//!
//! \param message 提示文本
//!
//! 用法：`Toast { id: toast }` 后调用 `toast.show("已保存")`。

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string message: ""
    property int duration: 3200

    width: box.width
    height: box.height

    opacity: 0.0
    visible: opacity > 0.01
    scale: opacity > 0.5 ? 1.0 : 0.96
    Behavior on opacity {
        NumberAnimation {
            duration: Theme.durationFast
            easing.type: Easing.OutCubic
        }
    }
    Behavior on scale {
        NumberAnimation {
            duration: Theme.durationFast
            easing.type: Easing.OutCubic
        }
    }

    //! 显示一条提示（重复调用会重置计时）
    function show(text) {
        if (!text || text.length === 0)
            return
        root.message = text
        root.opacity = 1.0
        hideTimer.restart()
    }

    function hide() {
        root.opacity = 0.0
        hideTimer.stop()
    }

    Timer {
        id: hideTimer
        interval: root.duration
        onTriggered: root.opacity = 0.0
    }

    Rectangle {
        id: box
        width: 340
        height: Math.max(40, textItem.implicitHeight + Theme.sp5)
        radius: Theme.radiusPanel
        color: Theme.bgPopover
        border.width: 1
        border.color: Theme.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp3
            anchors.rightMargin: Theme.sp4
            spacing: Theme.sp2

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: Theme.accent
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                id: textItem
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: root.message
                color: Theme.text1
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsBody
                wrapMode: Text.Wrap
            }
        }
    }
}
