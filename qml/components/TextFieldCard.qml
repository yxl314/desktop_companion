//! \file TextFieldCard.qml
//! \brief 带标签的输入框 / 多行文本域：32 高、6 圆角、focus 显示品牌色焦点环。
//!
//! \param label       字段标签（12/500），可为空
//! \param hint        字段说明（12，text2），可为空；错误说明放这里并改 error
//! \param placeholder 占位提示
//! \param text        当前文本（程序写入；用户编辑时通过 edited() 通知）
//! \param multiline   true=多行文本域（高度 fieldHeight），false=单行 32 高
//! \param fieldHeight 多行文本域高度，默认 140
//! \param password    密码模式（API Key）
//! \param error       true=红色描边（校验失败）
//! \param readOnly    只读
//!
//! \signal edited(string value) 用户输入变更（父级可转调 VM setter）

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string label: ""
    property string hint: ""
    property string placeholder: ""
    property string text: ""
    property bool multiline: false
    property int fieldHeight: 140
    property bool password: false
    property bool error: false
    property bool readOnly: false

    signal edited(string value)

    implicitWidth: 320
    implicitHeight: layout.implicitHeight

    //! 外部写入 text 时同步到内部控件（聚焦中不打断用户输入）
    function syncFromText() {
        if (!field.activeFocus)
            field.text = root.text
        if (!area.activeFocus)
            area.text = root.text
    }

    onTextChanged: syncFromText()
    Component.onCompleted: syncFromText()

    ColumnLayout {
        id: layout
        width: root.width
        spacing: Theme.spSpacing6

        Text {
            Layout.fillWidth: true
            visible: root.label.length > 0
            text: root.label
            color: Theme.text1
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
            font.weight: 500
        }

        Item {
            id: fieldWrap
            Layout.fillWidth: true
            implicitHeight: root.multiline ? root.fieldHeight : Theme.hControl

            //! 焦点光晕（3px，22% 品牌色）
            Rectangle {
                anchors.fill: fieldBox
                anchors.margins: -3
                radius: Theme.radiusControl + 3
                color: "transparent"
                border.width: 3
                border.color: Theme.focusGlow
                visible: field.activeFocus || area.activeFocus
            }

            Rectangle {
                id: fieldBox
                anchors.fill: parent
                radius: Theme.radiusControl
                color: Theme.bgCard
                border.width: 1
                border.color: {
                    if (root.error)
                        return Theme.danger
                    if (field.activeFocus || area.activeFocus)
                        return Theme.ring
                    return Theme.borderStrong
                }

                //! 自绘占位文字（不使用 Material 浮动 label，避免填充后占位残留/重叠）
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: Theme.sp2 + Theme.spHalf
                    anchors.rightMargin: Theme.sp2 + Theme.spHalf
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root.multiline && field.text.length === 0
                             && root.placeholder.length > 0
                    text: root.placeholder
                    color: Theme.text3
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: Theme.sp3
                    anchors.topMargin: Theme.sp2 + Theme.spHalf
                    anchors.right: parent.right
                    anchors.rightMargin: Theme.sp3
                    visible: root.multiline && area.text.length === 0
                             && root.placeholder.length > 0
                    text: root.placeholder
                    color: Theme.text3
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    wrapMode: Text.Wrap
                }

                TextField {
                    id: field
                    anchors.fill: parent
                    visible: !root.multiline
                    leftPadding: Theme.sp2 + Theme.spHalf
                    rightPadding: Theme.sp2 + Theme.spHalf
                    topPadding: 0
                    bottomPadding: 0
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text1
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.accentFg
                    echoMode: root.password ? TextInput.Password : TextInput.Normal
                    readOnly: root.readOnly
                    font.family: root.password ? Theme.monoFamily : Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    background: Rectangle {
                        color: "transparent"
                    }
                    onTextChanged: {
                        if (field.activeFocus) {
                            root.text = field.text
                            root.edited(field.text)
                        }
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus)
                            root.edited(field.text)
                    }
                }

                ScrollView {
                    id: areaScroll
                    anchors.fill: parent
                    anchors.margins: Theme.spSpacing6
                    visible: root.multiline
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    TextArea {
                        id: area
                        width: areaScroll.availableWidth
                        color: Theme.text1
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.accentFg
                        wrapMode: TextArea.Wrap
                        readOnly: root.readOnly
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBody
                        background: Rectangle {
                            color: "transparent"
                        }
                        onTextChanged: {
                            if (area.activeFocus) {
                                root.text = area.text
                                root.edited(area.text)
                            }
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.hint.length > 0
            text: root.hint
            color: root.error ? Theme.dangerText : Theme.text2
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
            wrapMode: Text.Wrap
        }
    }
}
