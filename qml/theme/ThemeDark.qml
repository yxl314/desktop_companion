//! \file ThemeDark.qml
//! \brief 暗色设计 Token 表：继承 ThemeLight 的尺寸/字号/间距，仅覆盖颜色。
//!
//! 数值对齐 ref-esrguisoft/ccswitch-v4-ui-spec.md §1.2；品牌主色保持青绿
//! #0F766E，浅底/文字改用暗色可读的调色（accentText #7CC4BA）。
//! 由 Theme.qml 按 appVM.themeIndex 实例化并转发，页面不得直接引用本文件。

import QtQuick

ThemeLight {
    // ======================================================================
    // 背景 / 边框 / 文字
    // ======================================================================
    bgApp: "#1C1C1E"
    bgSidebar: "#161618"
    bgSubtle: "#26262A"
    bgSelected: "#313136"
    bgCard: "#2E2E33"
    bgPopover: "#2E2E33"

    border: "#3E3E45"
    borderStrong: "#4A4A52"

    text1: "#E4E4E7"
    text2: "#ABABB3"
    text3: "#9A9AA2"

    // ======================================================================
    // 品牌主操作（暗色下 hover 更深、文字/浅底提亮）
    // ======================================================================
    accent: "#0F766E"
    accentHover: "#0B5F58"
    accentFg: "#FFFFFF"
    accentText: "#7CC4BA"
    accentSoft: Qt.rgba(15 / 255, 118 / 255, 110 / 255, 0.20)
    ring: "#2A9D8F"
    focusGlow: Qt.rgba(42 / 255, 157 / 255, 143 / 255, 0.18)

    // ======================================================================
    // 反色 / 遮罩 / 开关 / 滚动条 / 阴影
    // ======================================================================
    inverseBg: "#F4F4F5"
    inverseFg: "#18181B"
    inverseHover: "#E4E4E7"
    overlay: Qt.rgba(0, 0, 0, 0.55)
    controlOff: "#6F6F77"
    scrollHandle: "#4A4A52"
    scrollHandleHover: "#9A9AA2"
    shadowColor: Qt.rgba(0, 0, 0, 0.45)
    shadowBlur: 24
    shadowOffsetY: 8

    // ======================================================================
    // 语义色（暗色提亮）
    // ======================================================================
    success: "#4ADE80"
    successText: "#4ADE80"
    successSoft: "#0F2B1A"
    warning: "#FBBF24"
    warningText: "#FCD34D"
    warningSoft: "#33260A"
    danger: "#F87171"
    dangerText: "#FCA5A5"
    dangerSoft: "#3A1414"

    // ======================================================================
    // 聊天 / 卡片派生 token
    // ======================================================================
    bubbleUserBg: Qt.rgba(15 / 255, 118 / 255, 110 / 255, 0.24)
    bubbleUserBorder: Qt.rgba(42 / 255, 157 / 255, 143 / 255, 0.55)
    bubbleAssistantBg: "#2E2E33"
    bubbleAssistantBorder: "#3E3E45"
    codeBg: "#26262A"
    cardActiveBg: Qt.rgba(15 / 255, 118 / 255, 110 / 255, 0.20)
    cardActiveBorder: "#2A9D8F"
}
