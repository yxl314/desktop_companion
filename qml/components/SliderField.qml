//! \file SliderField.qml
//! \brief 带标签与数值的滑块字段（温度 / 浮动幅度 / maxTokens）。
//!
//! \param label    字段标签
//! \param from     最小值
//! \param to       最大值
//! \param stepSize 步进
//! \param value    当前值（父级绑定 VM 属性）
//! \param decimals 数值显示小数位，默认 0
//! \param suffix   数值后缀，如 " tokens"
//!
//! \signal moved(real value) 拖动中/释放时通知（父级调用 VM setter）

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string label: ""
    property real from: 0
    property real to: 1
    property real stepSize: 0.05
    property real value: 0
    property int decimals: 0
    property string suffix: ""

    signal moved(real value)

    implicitWidth: 360
    implicitHeight: layout.implicitHeight

    function syncFromValue() {
        if (!slider.pressed)
            slider.value = root.value
    }

    onValueChanged: syncFromValue()
    Component.onCompleted: syncFromValue()

    ColumnLayout {
        id: layout
        width: root.width
        spacing: Theme.spSpacing6

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2

            Text {
                Layout.fillWidth: true
                text: root.label
                color: Theme.text1
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsCaption
                font.weight: 500
                elide: Text.ElideRight
            }
            Text {
                text: Number(root.value).toFixed(root.decimals) + root.suffix
                color: Theme.text2
                font.family: root.decimals > 0 ? Theme.fontFamily : Theme.monoFamily
                font.pixelSize: Theme.fsCaption
            }
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            from: root.from
            to: root.to
            stepSize: root.stepSize
            snapMode: Slider.SnapAlways
            onMoved: root.moved(value)

            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 4
                radius: 2
                color: Theme.bgSubtle

                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.accent
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 16
                height: 16
                radius: 8
                color: Theme.bgCard
                border.width: 2
                border.color: Theme.accent
                scale: slider.pressed ? 1.15 : 1.0
                Behavior on scale {
                    NumberAnimation {
                        duration: Theme.durationFast
                    }
                }
            }
        }
    }
}
