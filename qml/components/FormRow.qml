//! \file FormRow.qml
//! \brief 设置表单行：左侧 120px 标签列 + 右侧默认内容插槽。
//!
//! \param label 字段名
//! \param hint  字段说明（可选，12 号 text3）
//!
//! 用法：直接声明的子项进入右侧内容行（纵向居中）。

import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string label: ""
    property string hint: ""

    default property alias content: contentRow.data

    implicitWidth: 480
    implicitHeight: Math.max(row.implicitHeight, Theme.hControl)

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: Theme.sp3

        ColumnLayout {
            Layout.preferredWidth: 120
            Layout.alignment: Qt.AlignVCenter
            spacing: Theme.spHalf

            Text {
                Layout.fillWidth: true
                text: root.label
                color: Theme.text1
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsBody
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: root.hint.length > 0
                text: root.hint
                color: Theme.text3
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsCaption
                wrapMode: Text.Wrap
            }
        }

        RowLayout {
            id: contentRow
            Layout.fillWidth: true
            spacing: Theme.sp2
        }
    }
}
