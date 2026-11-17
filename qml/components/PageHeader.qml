//! \file PageHeader.qml
//! \brief 页内顶栏：52 高（含 1px 底边框）、标题 + 描述 + 右侧主操作插槽。
//!
//! \param title       页面标题（18/26/600）
//! \param description 页面描述（12/18，text3），留空不占位
//! \param iconName    可选前置图标（22×22 线性图标）
//!
//! 用法：直接声明的按钮进入右侧操作区，主操作按钮放最后：
//!   PageHeader { title: qsTr("角色"); description: qsTr("管理桌宠角色")
//!       SecondaryButton { text: qsTr("导入") }
//!       PrimaryButton { text: qsTr("新建角色") }
//!   }

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string title: ""
    property string description: ""
    property string iconName: ""

    //! 右侧操作区插槽
    default property alias actions: actionsLayout.data

    implicitWidth: 800
    implicitHeight: Theme.hPageHeader

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.sp3

        Icon {
            visible: root.iconName.length > 0
            name: root.iconName
            size: Theme.iconHeader
            color: Theme.text1
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: Theme.spHalf
            Layout.maximumWidth: Math.max(0, parent.width * 0.55)

            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.text1
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsPage
                font.weight: 600
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: root.description.length > 0
                text: root.description
                color: Theme.text3
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsCaption
                elide: Text.ElideRight
            }
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            id: actionsLayout
            spacing: Theme.sp2
            Layout.alignment: Qt.AlignVCenter
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: Theme.border
    }
}
