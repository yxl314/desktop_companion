//! \file PrimaryButton.qml
//! \brief 主操作按钮（品牌青绿实心，32/28 高，6 圆角，600 字重）。
//!
//! \param text     按钮文字
//! \param iconName 可选前置线性图标（16×16）
//! \param compact  true=28 高（compact 档），默认 32
//!
//! \note 一个视图只应有一个实心主按钮，其余用 SecondaryButton / IconButton。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Button {
    id: control

    property bool compact: false
    property string iconName: ""

    hoverEnabled: true
    implicitHeight: compact ? Theme.hControlSm : Theme.hControl
    leftPadding: compact ? Theme.sp3 : Theme.sp3 + 2
    rightPadding: compact ? Theme.sp3 : Theme.sp3 + 2
    topPadding: 0
    bottomPadding: 0
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fsBody
    font.weight: 600
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: Theme.radiusControl
        color: (control.down || control.hovered) && control.enabled
               ? Theme.accentHover : Theme.accent
        opacity: control.enabled ? 1.0 : Theme.disabledOpacity
    }

    contentItem: RowLayout {
        spacing: Theme.spSpacing6

        Icon {
            visible: control.iconName.length > 0
            name: control.iconName
            size: Theme.iconMd
            color: Theme.accentFg
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: control.text
            color: Theme.accentFg
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
