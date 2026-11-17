//! \file MemoriesPage.qml
//! \brief 记忆页：添加 / 编辑 / 置顶 / 删除记忆，异步抽取长期记忆。
//!
//! 数据绑定（§7.7）：
//!   memoriesVM.memories（id,content,pinned,createdAt）、busy、count
//! 命令：add(content) / update(id,content) / remove(id) / togglePin(id) /
//!       refresh() / extractNow()
//!
//! \note extractNow() 当前由 VM 发 extractRequested 信号，后续由 Application 接线。


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    property string pendingMemoryId: ""

    function addMemory() {
        const text = addCard.text.trim()
        if (text.length === 0 || memoriesVM.busy)
            return
        memoriesVM.add(text)
        addCard.text = ""
    }

    function editMemory(id, content) {
        page.pendingMemoryId = id
        editDialog.openFor(qsTr("编辑记忆"), qsTr("记忆内容"), content, true)
    }

    function removeMemory(id) {
        if (dialogs.confirm(qsTr("删除记忆"), qsTr("确定删除这条记忆吗？")))
            memoriesVM.remove(id)
    }

    InputDialog {
        id: editDialog
        onAcceptedText: function(text) {
            memoriesVM.update(page.pendingMemoryId, text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("记忆")
            description: qsTr("%1 条记忆 · 注入当前角色与人设的提示词")
                         .arg(memoriesVM.count)

            IconButton {
                iconName: "refresh-cw"
                tooltip: qsTr("刷新")
                enabled: !memoriesVM.busy
                onClicked: memoriesVM.refresh()
            }
            SecondaryButton {
                text: qsTr("立即抽取")
                iconName: "zap"
                enabled: !memoriesVM.busy
                onClicked: memoriesVM.extractNow()
            }
            PrimaryButton {
                text: qsTr("添加记忆")
                iconName: "plus"
                onClicked: addCard.forceActiveFocus()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ==============================================================
            // 添加记忆
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("添加记忆")
                description: qsTr("手动记录一条希望角色长期记住的信息")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    TextFieldCard {
                        id: addCard
                        Layout.fillWidth: true
                        placeholder: qsTr("例如：主人最喜欢雨天和热可可")
                    }

                    PrimaryButton {
                        id: addButton
                        Layout.alignment: Qt.AlignBottom
                        text: qsTr("添加")
                        enabled: addCard.text.trim().length > 0 && !memoriesVM.busy
                        onClicked: page.addMemory()
                    }
                }
            }

            // ==============================================================
            // 记忆列表
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("已保存的记忆")
                description: qsTr("置顶的记忆会优先注入提示词")

                EmptyState {
                    Layout.fillWidth: true
                    visible: memoriesVM.memories.length === 0
                    iconName: "book-open"
                    title: qsTr("还没有记忆")
                    description: qsTr("聊天后角色会逐渐记住你们之间的事，也可以在这里手动添加。")
                    actionText: qsTr("添加记忆")
                    onActionClicked: addCard.forceActiveFocus()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    Repeater {
                        model: memoriesVM.memories

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: memoryColumn.implicitHeight + Theme.sp3 * 2
                            radius: Theme.radiusControl
                            color: modelData.pinned === true ? Theme.warningSoft : Theme.bgSubtle
                            border.width: 1
                            border.color: modelData.pinned === true
                                          ? Theme.warning : Theme.border

                            ColumnLayout {
                                id: memoryColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: Theme.sp3
                                spacing: Theme.sp1

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.sp1

                                    Tag {
                                        visible: modelData.pinned === true
                                        text: qsTr("置顶")
                                        tone: "warning"
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.createdAt === undefined
                                              ? "" : modelData.createdAt
                                        color: Theme.text3
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fsBadge
                                    }
                                    SecondaryButton {
                                        text: modelData.pinned === true
                                              ? qsTr("取消置顶") : qsTr("置顶")
                                        compact: true
                                        onClicked: memoriesVM.togglePin(modelData.id)
                                    }
                                    SecondaryButton {
                                        text: qsTr("编辑")
                                        compact: true
                                        onClicked: page.editMemory(modelData.id,
                                                                   modelData.content)
                                    }
                                    SecondaryButton {
                                        text: qsTr("删除")
                                        compact: true
                                        danger: true
                                        onClicked: page.removeMemory(modelData.id)
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.content
                                    color: Theme.text1
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fsBody
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
