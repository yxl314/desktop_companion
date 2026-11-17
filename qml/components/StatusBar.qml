//! \file StatusBar.qml
//! \brief 主窗口底部状态栏：左侧全局状态文本，右侧桌宠运行状态点 + 版本。
//!
//! \param message   左侧状态文本（绑定 appVM.appStatus）
//! \param rightText 右侧文本（如「桌宠运行中（1 只）」）
//! \param dotColor  右侧状态点颜色（运行=success，停止=text3）
//! \param version   版本号（右侧最末）

import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string message: ""
    property string rightText: ""
    property color dotColor: Theme.text3
    property string version: ""

    implicitHeight: Theme.statusBarHeight
    color: Theme.bgApp

    Rectangle {
        width: parent.width
        height: 1
        anchors.top: parent.top
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.sp2

        Text {
            Layout.fillWidth: true
            text: root.message
            color: Theme.text3
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
            elide: Text.ElideRight
        }

        Rectangle {
            visible: root.rightText.length > 0
            Layout.preferredWidth: 6
            Layout.preferredHeight: 6
            radius: 3
            color: root.dotColor
        }
        Text {
            text: root.rightText
            color: Theme.text3
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
        }
        Text {
            visible: root.version.length > 0
            text: root.version
            color: Theme.text3
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
        }
    }
}
