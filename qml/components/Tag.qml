//! \file Tag.qml
//! \brief 小标签 / 徽章：18 高、全圆角、11 号字（对齐设计规范 §3.5）。
//!
//! \param text 标签文字
//! \param tone 语义色：neutral / accent / success / warning / danger
//!
//! \note 一张卡片最多放两个徽章；徽章不使用实心彩色块。

import QtQuick
import "../theme"

Rectangle {
    id: root

    property string text: ""
    property string tone: "neutral"

    implicitWidth: label.implicitWidth + Theme.spSpacing6 * 2
    implicitHeight: 18
    radius: Theme.radiusFull
    color: {
        switch (root.tone) {
        case "accent": return Theme.accentSoft
        case "success": return Theme.successSoft
        case "warning": return Theme.warningSoft
        case "danger": return Theme.dangerSoft
        default: return "transparent"
        }
    }
    border.width: root.tone === "neutral" ? 1 : 0
    border.color: Theme.borderStrong

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: {
            switch (root.tone) {
            case "accent": return Theme.accentText
            case "success": return Theme.successText
            case "warning": return Theme.warningText
            case "danger": return Theme.dangerText
            default: return Theme.text2
            }
        }
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fsBadge
        font.weight: 500
    }
}
