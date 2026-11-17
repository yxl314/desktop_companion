//! \file InputDialog.qml
//! \brief 文本输入对话框（新建/重命名/编辑记忆等）：14 圆角 + 遮罩 + 取消/确定。
//!
//! \param prompt      输入框标签
//! \param detail      标题下方说明（可选）
//! \param multiline   true=多行文本域
//!
//! \signal acceptedText(string text) 用户点击「确定」时回传文本
//!
//! 用法：`dialog.openFor(qsTr("新建角色"), qsTr("角色名字"), "小柚", false)`。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: control

    property string prompt: ""
    property string detail: ""
    property bool multiline: false

    signal acceptedText(string text)

    modal: true
    width: 440
    padding: Theme.sp5
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: Overlay.overlay

    //! 打开对话框并预填文本。
    function openFor(titleText, promptText, value, isMultiline) {
        control.title = titleText
        control.prompt = promptText
        control.detail = ""
        control.multiline = isMultiline === true
        inputCard.text = value === undefined || value === null ? "" : value
        control.open()
        Qt.callLater(function() {
            inputCard.forceActiveFocus()
        })
    }

    background: Rectangle {
        color: Theme.bgPopover
        radius: Theme.radiusDialog
        border.width: 1
        border.color: Theme.border
    }

    header: Text {
        leftPadding: Theme.sp5
        rightPadding: Theme.sp5
        topPadding: Theme.sp4
        text: control.title
        color: Theme.text1
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fsTitle
        font.weight: 600
    }

    contentItem: ColumnLayout {
        width: Math.max(0, control.availableWidth)
        spacing: Theme.sp3

        Text {
            Layout.fillWidth: true
            visible: control.detail.length > 0
            text: control.detail
            color: Theme.text2
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsBody
            wrapMode: Text.Wrap
        }

        TextFieldCard {
            id: inputCard
            Layout.fillWidth: true
            label: control.prompt
            multiline: control.multiline
            fieldHeight: 140
            placeholder: control.multiline ? qsTr("输入内容…") : qsTr("输入…")
        }
    }

    footer: RowLayout {
        spacing: Theme.sp2
        Item {
            Layout.fillWidth: true
        }
        SecondaryButton {
            text: qsTr("取消")
            onClicked: control.close()
        }
        PrimaryButton {
            text: qsTr("确定")
            enabled: inputCard.text.length > 0
            onClicked: {
                const value = inputCard.text
                control.close()
                control.acceptedText(value)
            }
        }
    }

    onOpened: inputCard.forceActiveFocus()
}
