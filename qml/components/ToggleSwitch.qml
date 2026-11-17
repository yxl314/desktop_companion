//! \file ToggleSwitch.qml
//! \brief 开关：30×18 轨道、14×14 圆点、150ms 过渡（对齐设计规范 §3.8）。
//!
//! \param checked  开态（父级绑定 VM 属性，点击时由父级调用 VM setter）
//! \param interactive 是否可交互（避免覆盖 Item.enabled）
//!
//! \signal clicked() 点击切换（父级负责把新值写回 ViewModel）

import QtQuick
import "../theme"

Item {
    id: root

    property bool checked: false
    property bool interactive: true

    signal clicked()

    implicitWidth: 30
    implicitHeight: 18

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? Theme.accent : Theme.controlOff
        opacity: root.interactive ? 1.0 : Theme.disabledOpacity
        Behavior on color {
            ColorAnimation {
                duration: Theme.durationFast
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            width: 14
            height: 14
            radius: 7
            y: 2
            x: root.checked ? track.width - width - 2 : 2
            color: Theme.switchKnob
            Behavior on x {
                NumberAnimation {
                    duration: Theme.durationFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: -Theme.sp1
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
