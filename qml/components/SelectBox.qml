//! \file SelectBox.qml
//! \brief 下拉选择框：32 高 / 6 圆角触发器 + 10 圆角弹层（规范 §3.6）。
//!
//! \param options      选项文字数组
//! \param currentIndex 当前项（父级绑定 VM 属性）
//! \param placeholder  无选项时的占位文字
//!
//! \signal activated(int index) 选择选项（父级调用 VM setter，不直接改 currentIndex）


import QtQuick
import QtQuick.Controls
import "../theme"

Item {
    id: root

    property var options: []
    property int currentIndex: 0
    property string placeholder: ""

    signal activated(int index)

    implicitWidth: 220
    implicitHeight: Theme.hControl

    readonly property string currentText: options.length > 0
                                           && currentIndex >= 0
                                           && currentIndex < options.length
                                         ? String(options[currentIndex]) : placeholder

    Rectangle {
        id: trigger
        anchors.fill: parent
        radius: Theme.radiusControl
        color: hoverHandler.hovered || popup.opened ? Theme.bgSubtle : Theme.bgCard
        border.width: 1
        border.color: Theme.borderStrong
        Behavior on color {
            ColorAnimation {
                duration: Theme.durationFast
            }
        }

        Text {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp2 + Theme.spHalf
            anchors.rightMargin: 28
            text: root.currentText
            color: root.currentText.length > 0 ? Theme.text1 : Theme.text3
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsBody
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Icon {
            anchors.right: parent.right
            anchors.rightMargin: Theme.sp2
            anchors.verticalCenter: parent.verticalCenter
            name: "chevron-down"
            size: Theme.iconSm
            color: Theme.text3
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: popup.opened ? popup.close() : popup.open()
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    Popup {
        id: popup
        x: 0
        y: trigger.height + Theme.sp1
        width: trigger.width
        padding: Theme.sp1
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: Theme.radiusPanel
            color: Theme.bgPopover
            border.width: 1
            border.color: Theme.border
        }

        contentItem: Column {
            spacing: 0
            Repeater {
                model: root.options
                delegate: Rectangle {
                    width: popup.width - popup.leftPadding - popup.rightPadding
                    height: 30
                    radius: Theme.radiusControl
                    color: itemHover.hovered ? Theme.bgSubtle : "transparent"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp2
                        anchors.rightMargin: Theme.sp2
                        text: modelData
                        color: index === root.currentIndex ? Theme.accentText : Theme.text1
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBody
                        font.weight: index === root.currentIndex ? 500 : 400
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        id: itemHover
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            popup.close()
                            root.activated(index)
                        }
                    }
                }
            }
        }
    }
}
