//! \file SidebarItem.qml
//! \brief 侧栏导航行：28 高、6 圆角、选中态 bgSelected + 加粗。
//!
//! \param iconName 线性图标名（16×16）
//! \param label    导航文字（13/400，选中 500）
//! \param selected 是否当前页
//! \param badgeText 行尾小字（可选，如桌宠数量）
//! \param badgeTone 行尾小字语义色：neutral / accent / success / warning / danger
//!
//! \signal clicked() 点击导航行

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string iconName: ""
    property string label: ""
    property bool selected: false
    property string badgeText: ""
    property string badgeTone: "neutral"

    signal clicked()

    implicitWidth: Theme.sidebarWidth - Theme.sp4
    implicitHeight: Theme.hNavRow

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusControl
        color: root.selected ? Theme.bgSelected : (hoverHandler.hovered ? Theme.bgSubtle : "transparent")

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp2
            anchors.rightMargin: Theme.sp2
            spacing: Theme.sp2

            Icon {
                name: root.iconName
                size: Theme.iconMd
                color: root.selected ? Theme.text1 : Theme.text2
            }
            Text {
                Layout.fillWidth: true
                text: root.label
                color: root.selected ? Theme.text1 : Theme.text2
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsBody
                font.weight: root.selected ? 500 : 400
                elide: Text.ElideRight
            }
            Tag {
                visible: root.badgeText.length > 0
                text: root.badgeText
                tone: root.badgeTone
            }
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
