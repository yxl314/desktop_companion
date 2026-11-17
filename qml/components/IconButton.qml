//! \file IconButton.qml
//! \brief 图标按钮：28×28（compact 档）或 32×32，16×16 线性图标，hover 浅底。
//!
//! \param iconName 线性图标名
//! \param tooltip  悬浮提示（可为空）
//! \param size     按钮边长，默认 Theme.hControlSm（28）
//! \param danger   hover/图标使用危险色
//! \param fallbackText 无图标时显示的文字符号（如 "✎"），用于图标集缺失的语义

import QtQuick
import QtQuick.Controls
import "../theme"

Button {
    id: control

    property string iconName: ""
    property string tooltip: ""
    property bool danger: false
    property string fallbackText: ""

    implicitWidth: Theme.hControlSm
    implicitHeight: Theme.hControlSm
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: Theme.radiusControl
        color: control.hovered && control.enabled ? Theme.bgSubtle : "transparent"
        opacity: control.enabled ? 1.0 : Theme.disabledOpacity
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            visible: control.iconName.length > 0
            name: control.iconName
            size: Theme.iconMd
            color: control.danger ? Theme.dangerText
                                 : (control.hovered ? Theme.text1 : Theme.text2)
        }
        Text {
            anchors.centerIn: parent
            visible: control.iconName.length === 0 && control.fallbackText.length > 0
            text: control.fallbackText
            color: control.danger ? Theme.dangerText
                                 : (control.hovered ? Theme.text1 : Theme.text2)
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsBody
        }
    }

    ToolTip.visible: control.hovered && control.tooltip.length > 0
    ToolTip.text: control.tooltip
    ToolTip.delay: 450
}
