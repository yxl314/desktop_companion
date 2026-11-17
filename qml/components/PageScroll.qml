//! \file PageScroll.qml
//! \brief 页面滚动容器：24px 页面留白 + 16px 顶部 / 48px 底部留白（规范 §2.2）。
//!
//! 用法：直接声明的子项进入页面内容列（自动具备宽度与 12px 纵向间距）：
//!   PageScroll { Layout.fillWidth: true; Layout.fillHeight: true
//!       Card { Layout.fillWidth: true }
//!   }
//!
//! \note 根节点用 Item 包裹 ScrollView：ScrollView 的默认属性为 FINAL，
//!       不能在子类化时覆盖 default property，因此这里用组合而非继承。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property int padding: Theme.pagePadding
    property int paddingTop: Theme.pagePaddingTop
    property int paddingBottom: Theme.pagePaddingBottom
    property int spacing: Theme.sp3

    //! 页面内容插槽
    default property alias content: contentColumn.data

    implicitWidth: 600
    implicitHeight: 400

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: contentColumn.implicitHeight + root.paddingTop + root.paddingBottom

        ScrollBar.vertical: ScrollBar {
            id: verticalBar
            policy: ScrollBar.AsNeeded
            width: 8
            contentItem: Rectangle {
                implicitWidth: 4
                implicitHeight: 24
                radius: 2
                color: verticalBar.pressed || verticalBar.hovered
                       ? Theme.scrollHandleHover : Theme.scrollHandle
            }
            background: Item {}
        }

        ColumnLayout {
            id: contentColumn
            x: root.padding
            y: root.paddingTop
            width: Math.max(0, scroll.availableWidth - root.padding * 2)
            spacing: root.spacing
        }
    }
}
