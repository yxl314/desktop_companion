//! \file Theme.qml
//! \brief 设计 Token 单例：QML 侧所有颜色 / 圆角 / 间距 / 字号的唯一入口。
//!
//! 用法：
//!   import "../theme"
//!   Rectangle { color: Theme.bgCard; radius: Theme.radiusPanel }
//!
//! 约定：
//!   - 任何 QML 文件不得硬编码色值，必须读 Theme.*；
//!   - 亮色 token 定义在 ThemeLight.qml、暗色在 ThemeDark.qml，
//!     本单例按 appVM.themeIndex（0 浅 / 1 深 / 2 跟随系统）选择并转发；
//!   - 主题切换即时生效（属性 NOTIFY），不需要重启。
//!
//! \note 本文件在 qmldir 中注册为 singleton；目录导入（import "../theme"）后可用。

pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    id: root

    // ======================================================================
    // 外观解析：浅/深/跟随系统
    // ======================================================================
    //! 0=浅色 1=深色 2=跟随系统（AppViewModel.themeIndex）
    property int themeIndex: (typeof appVM !== "undefined" && appVM !== null)
                             ? appVM.themeIndex : 0

    //! 系统调色板（仅用于「跟随系统」时判断亮/暗）
    property SystemPalette systemPalette: SystemPalette {
        colorGroup: SystemPalette.Active
    }

    //! 系统外观是否为暗色
    property bool systemDark: systemPalette.window.hslLightness < 0.5

    //! 当前实际使用暗色 token
    property bool dark: themeIndex === 1 || (themeIndex === 2 && systemDark)

    //! 亮/暗 token 实例与当前生效集合
    property ThemeLight lightTokens: ThemeLight {}
    property ThemeLight darkTokens: ThemeDark {}
    property ThemeLight tokens: dark ? darkTokens : lightTokens

    // ======================================================================
    // 颜色 token（转发当前生效 token 表）
    // ======================================================================
    property color bgApp: tokens.bgApp
    property color bgSidebar: tokens.bgSidebar
    property color bgSubtle: tokens.bgSubtle
    property color bgSelected: tokens.bgSelected
    property color bgCard: tokens.bgCard
    property color bgPopover: tokens.bgPopover

    property color border: tokens.border
    property color borderStrong: tokens.borderStrong
    property color text1: tokens.text1
    property color text2: tokens.text2
    property color text3: tokens.text3

    property color accent: tokens.accent
    property color accentHover: tokens.accentHover
    property color accentFg: tokens.accentFg
    property color accentText: tokens.accentText
    property color accentSoft: tokens.accentSoft
    property color ring: tokens.ring
    property color focusGlow: tokens.focusGlow

    property color inverseBg: tokens.inverseBg
    property color inverseFg: tokens.inverseFg
    property color inverseHover: tokens.inverseHover
    property color overlay: tokens.overlay
    property color controlOff: tokens.controlOff
    property color switchKnob: tokens.switchKnob
    property color scrollHandle: tokens.scrollHandle
    property color scrollHandleHover: tokens.scrollHandleHover
    property color shadowColor: tokens.shadowColor
    property int shadowBlur: tokens.shadowBlur
    property int shadowOffsetY: tokens.shadowOffsetY

    property color success: tokens.success
    property color successText: tokens.successText
    property color successSoft: tokens.successSoft
    property color warning: tokens.warning
    property color warningText: tokens.warningText
    property color warningSoft: tokens.warningSoft
    property color danger: tokens.danger
    property color dangerText: tokens.dangerText
    property color dangerSoft: tokens.dangerSoft

    property color bubbleUserBg: tokens.bubbleUserBg
    property color bubbleUserBorder: tokens.bubbleUserBorder
    property color bubbleAssistantBg: tokens.bubbleAssistantBg
    property color bubbleAssistantBorder: tokens.bubbleAssistantBorder
    property color codeBg: tokens.codeBg
    property color cardActiveBg: tokens.cardActiveBg
    property color cardActiveBorder: tokens.cardActiveBorder

    // ======================================================================
    // 尺寸 token
    // ======================================================================
    property int radiusControl: tokens.radiusControl
    property int radiusNested: tokens.radiusNested
    property int radiusPanel: tokens.radiusPanel
    property int radiusDialog: tokens.radiusDialog
    property int radiusFull: tokens.radiusFull
    property int radiusIconBox: tokens.radiusIconBox

    property int hControl: tokens.hControl
    property int hControlSm: tokens.hControlSm
    property int hNavRow: tokens.hNavRow
    property int hCardMin: tokens.hCardMin
    property int cardPadY: tokens.cardPadY
    property int hPageHeader: tokens.hPageHeader
    property int hSegment: tokens.hSegment
    property int hSegmentSm: tokens.hSegmentSm
    property int segmentInset: tokens.segmentInset

    property int sidebarWidth: tokens.sidebarWidth
    property int pagePadding: tokens.pagePadding
    property int pagePaddingTop: tokens.pagePaddingTop
    property int pagePaddingBottom: tokens.pagePaddingBottom
    property int statusBarHeight: tokens.statusBarHeight
    property int bubbleSpacing: tokens.bubbleSpacing

    property int fsBadge: tokens.fsBadge
    property int fsCaption: tokens.fsCaption
    property int fsBody: tokens.fsBody
    property int fsStrong: tokens.fsStrong
    property int fsSection: tokens.fsSection
    property int fsTitle: tokens.fsTitle
    property int fsPage: tokens.fsPage
    property int fsMetric: tokens.fsMetric

    property int sp1: tokens.sp1
    property int sp2: tokens.sp2
    property int sp3: tokens.sp3
    property int sp4: tokens.sp4
    property int sp5: tokens.sp5
    property int sp6: tokens.sp6
    property int spHalf: tokens.spHalf
    property int spSpacing6: tokens.spSpacing6

    property int iconSm: tokens.iconSm
    property int iconMd: tokens.iconMd
    property int iconNav: tokens.iconNav
    property int iconHeader: tokens.iconHeader

    property int durationFast: tokens.durationFast
    property int durationSlow: tokens.durationSlow
    property real disabledOpacity: tokens.disabledOpacity

    // ======================================================================
    // 字体 / 资源
    // ======================================================================
    property string fontFamily: tokens.fontFamily
    property string monoFamily: tokens.monoFamily

    //! 线性图标根路径（qrc 前缀 "/" + resources/icons，见 Lead 集成说明）
    property string iconRoot: "qrc:/icons/"
}
