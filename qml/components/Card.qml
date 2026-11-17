//! \file Card.qml
//! \brief 通用卡片容器：10px 圆角 + 1px 描边 + 可选标题/描述 + 默认内容插槽。
//!
//! \param title        卡片标题（14/500），留空不占位
//! \param description  卡片描述（12/18，text2），留空不占位
//! \param padded       是否使用 16px 内边距，默认 true
//! \param contentSpacing 内容间距，默认 Theme.sp3（12）
//! \param interactive  hover 时描边加深（可点击卡片用）
//!
//! 用法：
//!   Card { title: qsTr("外观")
//!       FormRow { ... }  // 直接声明的子项进入默认内容列
//!   }

import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string title: ""
    property string description: ""
    property bool padded: true
    property int contentSpacing: Theme.sp3
    property bool interactive: false

    //! 默认内容插槽：所有直接声明的子项都在内容列内（可带 Layout 附加属性）
    default property alias content: contentColumn.data

    implicitWidth: 320
    implicitHeight: contentColumn.implicitHeight + (padded ? Theme.sp4 * 2 : 0)
    color: Theme.bgCard
    radius: Theme.radiusPanel
    border.width: 1
    border.color: (root.interactive && hoverHandler.hovered) ? Theme.borderStrong : Theme.border
    clip: false

    HoverHandler {
        id: hoverHandler
        enabled: root.interactive
    }

    ColumnLayout {
        id: contentColumn
        x: root.padded ? Theme.sp4 : 0
        y: root.padded ? Theme.sp4 : 0
        width: Math.max(0, root.width - (root.padded ? Theme.sp4 * 2 : 0))
        spacing: root.contentSpacing

        //! 标题 / 描述（内部，外部内容排在其后）
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spHalf
            visible: root.title.length > 0 || root.description.length > 0

            Text {
                Layout.fillWidth: true
                visible: root.title.length > 0
                text: root.title
                color: Theme.text1
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsStrong
                font.weight: 500
                wrapMode: Text.Wrap
            }
            Text {
                Layout.fillWidth: true
                visible: root.description.length > 0
                text: root.description
                color: Theme.text2
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsCaption
                wrapMode: Text.Wrap
            }
        }
    }
}
