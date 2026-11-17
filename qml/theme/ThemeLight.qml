//! \file ThemeLight.qml
//! \brief 亮色设计 Token 表（颜色 / 圆角 / 控件高 / 间距 / 字号 / 字体）。
//!
//! 本文件是「亮色」视觉语言的唯一数据源，数值对齐
//! ref-esrguisoft/ccswitch-v4-ui-spec.md §1，仅品牌主色替换为青绿 #0F766E。
//! 组件不得硬编码色值，统一通过 Theme.qml 单例读取（Theme 按亮/暗自动转发）。
//!
//! 暗色继承本表并覆盖颜色；尺寸/字号/间距两套主题一致。

import QtQuick

QtObject {
    id: light

    // ======================================================================
    // 颜色 · 背景
    // ======================================================================
    property color bgApp: "#FFFFFF"          // 内容区底色
    property color bgSidebar: "#F5F5F7"      // 侧栏底色
    property color bgSubtle: "#EFEFF2"       // hover 底 / 分段轨道 / 图标框底
    property color bgSelected: "#E4E4EA"     // 侧栏当前项 / 选中行
    property color bgCard: "#FFFFFF"         // 卡片 / 输入框 / 弹层
    property color bgPopover: "#FFFFFF"      // 下拉 / 菜单弹层

    // ======================================================================
    // 颜色 · 边框 / 文字
    // ======================================================================
    property color border: "#E4E4E9"
    property color borderStrong: "#CFCFD6"
    property color text1: "#18181B"
    property color text2: "#52525B"
    property color text3: "#65656E"

    // ======================================================================
    // 颜色 · 品牌主操作（青绿，替换参考项目的橙色）
    // ======================================================================
    property color accent: "#0F766E"
    property color accentHover: "#0B5F58"
    property color accentFg: "#FFFFFF"
    property color accentText: "#0F766E"
    property color accentSoft: "#E7F6F4"
    property color ring: "#0F766E"
    property color focusGlow: Qt.rgba(15 / 255, 118 / 255, 110 / 255, 0.20)

    // ======================================================================
    // 颜色 · 反色 / 遮罩 / 开关关态 / 滚动条 / 阴影
    // ======================================================================
    property color inverseBg: "#18181B"
    property color inverseFg: "#FFFFFF"
    property color inverseHover: "#2E2E33"
    property color overlay: Qt.rgba(9 / 255, 9 / 255, 11 / 255, 0.36)
    property color controlOff: "#888890"
    property color switchKnob: "#FFFFFF"   // 开关圆点（亮/暗均白）
    property color scrollHandle: "#CFCFD6"
    property color scrollHandleHover: "#65656E"
    property color shadowColor: Qt.rgba(16 / 255, 24 / 255, 40 / 255, 0.10)
    property int shadowBlur: 18
    property int shadowOffsetY: 6

    // ======================================================================
    // 颜色 · 语义
    // ======================================================================
    property color success: "#16A34A"
    property color successText: "#007634"
    property color successSoft: "#E8F7EE"
    property color warning: "#D97706"
    property color warningText: "#A64A00"
    property color warningSoft: "#FEF3C7"
    property color danger: "#DC2626"
    property color dangerText: "#B91C1C"
    property color dangerSoft: "#FEE2E2"

    // ======================================================================
    // 颜色 · 聊天 / 数据展示的派生 token
    // ======================================================================
    property color bubbleUserBg: "#E7F6F4"     // 用户气泡：品牌浅底
    property color bubbleUserBorder: "#B9E4DF"
    property color bubbleAssistantBg: "#FFFFFF"
    property color bubbleAssistantBorder: "#E4E4E9"
    property color codeBg: "#EFEFF2"
    property color cardActiveBg: "#E7F6F4"     // 当前生效卡片：品牌浅底
    property color cardActiveBorder: "#0F766E"

    // ======================================================================
    // 尺寸 · 圆角 / 控件高（ccswitch §1.4 / §1.6）
    // ======================================================================
    property int radiusControl: 6
    property int radiusNested: 7
    property int radiusPanel: 10
    property int radiusDialog: 14
    property int radiusFull: 9999
    property int radiusIconBox: 8

    property int hControl: 32
    property int hControlSm: 28
    property int hNavRow: 28
    property int hCardMin: 58
    property int cardPadY: 10
    property int hPageHeader: 52
    property int hSegment: 36
    property int hSegmentSm: 28
    property int segmentInset: 3

    // ======================================================================
    // 尺寸 · 布局（ccswitch §2）
    // ======================================================================
    property int sidebarWidth: 200
    property int pagePadding: 24
    property int pagePaddingTop: 16
    property int pagePaddingBottom: 48
    property int statusBarHeight: 28
    property int bubbleSpacing: 12   // 聊天消息间距（专注模式）

    // ======================================================================
    // 字号阶梯（ccswitch §1.5）
    // ======================================================================
    property int fsBadge: 11
    property int fsCaption: 12
    property int fsBody: 13
    property int fsStrong: 14
    property int fsSection: 15
    property int fsTitle: 16
    property int fsPage: 18
    property int fsMetric: 24

    // ======================================================================
    // 间距刻度（4px 基准，ccswitch §1.7）
    // ======================================================================
    property int sp1: 4
    property int sp2: 8
    property int sp3: 12
    property int sp4: 16
    property int sp5: 20
    property int sp6: 24
    property int spHalf: 2
    property int spSpacing6: 6

    // ======================================================================
    // 图标尺寸（ccswitch §3.4）
    // ======================================================================
    property int iconSm: 14
    property int iconMd: 16
    property int iconNav: 18
    property int iconHeader: 22

    // ======================================================================
    // 动效与状态
    // ======================================================================
    property int durationFast: 150        // hover / 颜色过渡
    property int durationSlow: 200        // 分段滑块 / 主题切换
    property real disabledOpacity: 0.45

    // ======================================================================
    // 字体
    // ======================================================================
    property string fontFamily: Qt.platform.os === "windows"
                                         ? "Microsoft YaHei UI"
                                         : "Noto Sans CJK SC"
    property string monoFamily: Qt.platform.os === "windows"
                                         ? "Consolas"
                                         : "monospace"
}
