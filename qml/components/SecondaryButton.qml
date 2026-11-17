//! \file SecondaryButton.qml
//! \brief 次要按钮：描边 / 透明底（neutral）、危险操作（danger）、安静样式（quiet）。
//!
//! \param text     按钮文字
//! \param iconName 可选前置线性图标（16×16）
//! \param compact  true=28 高，默认 32
//! \param danger   危险操作：红色文字/描边（删除、清空等）
//! \param quiet    安静样式：透明底、无描边（页内次级操作）

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Button {
    id: control

    property bool compact: false
    property string iconName: ""
    property bool danger: false
    property bool quiet: false

    hoverEnabled: true
    implicitHeight: compact ? Theme.hControlSm : Theme.hControl
    leftPadding: compact ? Theme.sp3 : Theme.sp3 + 2
    rightPadding: compact ? Theme.sp3 : Theme.sp3 + 2
    topPadding: 0
    bottomPadding: 0
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fsBody
    font.weight: 500
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.hovered && control.enabled ? Theme.bgSubtle : "transparent"
        border.width: control.quiet ? 0 : 1
        border.color: control.danger ? Theme.danger : Theme.borderStrong
        opacity: control.enabled ? 1.0 : Theme.disabledOpacity
    }

    contentItem: RowLayout {
        spacing: Theme.spSpacing6

        Icon {
            visible: control.iconName.length > 0
            name: control.iconName
            size: Theme.iconMd
            color: control.danger ? Theme.dangerText
                                 : (control.hovered ? Theme.text1 : Theme.text2)
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: control.text
            color: control.danger ? Theme.dangerText : Theme.text1
            font.family: control.font.family
            font.pixelSize: control.font.pixelSize
            font.weight: control.font.weight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    scale: control.down && control.enabled ? 0.97 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: Theme.durationFast
            easing.type: Easing.OutCubic
        }
    }
}
