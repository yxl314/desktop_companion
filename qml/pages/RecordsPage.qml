//! \file RecordsPage.qml
//! \brief 记录页：当前角色/人设的最近聊天记录、赞踩统计、导出与清空。
//!
//! 数据绑定（§7.9）：
//!   recordsVM.records（id,role,text,feedback,time）、feedbackSummary、busy
//! 命令：refresh() / exportChat() / clearRecords()


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    function clearRecords() {
        if (dialogs.confirm(qsTr("清空记录"), qsTr("确定清空当前角色/人设的聊天记录吗？")))
            recordsVM.clearRecords()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("记录")
            description: recordsVM.feedbackSummary.length > 0
                         ? recordsVM.feedbackSummary : qsTr("当前角色/人设的最近聊天记录")

            IconButton {
                iconName: "refresh-cw"
                tooltip: qsTr("刷新")
                enabled: !recordsVM.busy
                onClicked: recordsVM.refresh()
            }
            SecondaryButton {
                text: qsTr("导出记录")
                iconName: "external-link"
                onClicked: recordsVM.exportChat()
            }
            SecondaryButton {
                text: qsTr("清空记录")
                danger: true
                onClicked: page.clearRecords()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: recordList
                anchors.fill: parent
                anchors.leftMargin: Theme.pagePadding
                anchors.rightMargin: Theme.pagePadding
                anchors.topMargin: Theme.pagePaddingTop
                anchors.bottomMargin: Theme.pagePadding
                clip: true
                spacing: Theme.sp2
                model: recordsVM.records

                delegate: Rectangle {
                    width: recordList.width
                    implicitHeight: recordColumn.implicitHeight + Theme.sp3 * 2
                    radius: Theme.radiusControl
                    color: modelData.role === "user" ? Theme.bubbleUserBg : Theme.bgCard
                    border.width: 1
                    border.color: modelData.role === "user"
                                  ? Theme.bubbleUserBorder : Theme.border

                    ColumnLayout {
                        id: recordColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.sp3
                        spacing: Theme.sp1

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2

                            Tag {
                                text: modelData.role === "user" ? qsTr("我") : qsTr("桌宠")
                                tone: modelData.role === "user" ? "accent" : "neutral"
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.time === undefined ? "" : modelData.time
                                color: Theme.text3
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBadge
                            }
                            Text {
                                visible: modelData.feedback === 1
                                text: "👍"
                                font.pixelSize: Theme.fsCaption
                            }
                            Text {
                                visible: modelData.feedback === -1
                                text: "👎"
                                font.pixelSize: Theme.fsCaption
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.text
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsBody
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                width: Math.min(460, parent.width - Theme.pagePadding * 2)
                visible: recordsVM.records.length === 0
                iconName: "film"
                title: qsTr("暂无记录")
                description: qsTr("和角色聊天后，这里会展示最近的对话与赞踩反馈。")
            }
        }
    }
}
