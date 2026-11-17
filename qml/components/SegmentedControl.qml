//! \file SegmentedControl.qml
//! \brief 分段控件：36/28 高轨道、3px 内衬、选中块 200ms 滑动（规范 §3.2）。
//!
//! \param options      选项文字数组，如 ["浅色", "深色", "跟随系统"]
//! \param currentIndex 当前选中项
//! \param compact      true=28 高（sm 档）
//!
//! \signal activated(int index) 点击选项（父级调用 VM setter）


import QtQuick
import "../theme"

Item {
    id: root

    property var options: []
    property int currentIndex: 0
    property bool compact: false

    signal activated(int index)

    implicitWidth: 300
    implicitHeight: compact ? Theme.hSegmentSm : Theme.hSegment

    readonly property int itemCount: Math.max(1, options.length)
    readonly property real itemWidth: (track.width - Theme.segmentInset * 2) / itemCount

    Rectangle {
        id: track
        anchors.fill: parent
        radius: root.compact ? Theme.radiusIconBox : Theme.radiusPanel
        color: Theme.bgSubtle

        Rectangle {
            id: pill
            x: Theme.segmentInset + root.currentIndex * root.itemWidth
            y: Theme.segmentInset
            width: root.itemWidth
            height: track.height - Theme.segmentInset * 2
            radius: root.compact ? 5 : Theme.radiusNested
            color: Theme.bgCard
            border.width: 1
            border.color: Theme.border
            Behavior on x {
                NumberAnimation {
                    duration: Theme.durationSlow
                    easing.type: Easing.OutCubic
                }
            }
        }

        Row {
            anchors.fill: parent
            Repeater {
                model: root.options
                delegate: Item {
                    width: root.itemWidth
                    height: track.height

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: index === root.currentIndex ? Theme.text1 : Theme.text2
                        font.family: Theme.fontFamily
                        font.pixelSize: root.compact ? Theme.fsCaption : Theme.fsBody
                        font.weight: index === root.currentIndex ? 600 : 500
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activated(index)
                    }
                }
            }
        }
    }
}
