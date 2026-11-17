//! \file EmptyState.qml
//! \brief 空态：56×56 图标框 + 标题 + 说明 + 可选描边按钮（规范 §3.11）。
//!
//! \param iconName    线性图标名（24×24）
//! \param title       空态标题（15/600）
//! \param description 说明文字（13，text2）
//! \param actionText  按钮文字，留空不显示按钮
//!
//! \signal actionClicked() 点击空态按钮

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string iconName: "sparkles"
    property string title: ""
    property string description: ""
    property string actionText: ""

    signal actionClicked()

    implicitWidth: 420
    implicitHeight: column.implicitHeight

    ColumnLayout {
        id: column
        anchors.centerIn: parent
        width: Math.min(root.width, 460)
        spacing: Theme.sp3

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56
            Layout.preferredHeight: 56
            radius: Theme.sp4
            color: Theme.bgSubtle
            border.width: 1
            border.color: Theme.border

            Icon {
                anchors.centerIn: parent
                name: root.iconName
                size: 24
                color: Theme.text2
            }
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: root.title
            color: Theme.text1
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsSection
            font.weight: 600
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            visible: root.description.length > 0
            horizontalAlignment: Text.AlignHCenter
            text: root.description
            color: Theme.text2
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsBody
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.sp2

            SecondaryButton {
                visible: root.actionText.length > 0
                text: root.actionText
                onClicked: root.actionClicked()
            }
        }
    }
}
