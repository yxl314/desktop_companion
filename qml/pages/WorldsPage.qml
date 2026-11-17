//! \file WorldsPage.qml
//! \brief 世界观页：左侧列表（预览/角色数）+ 右侧编辑区（名称/正文/预览/保存）。
//!
//! 数据绑定（§7.6）：
//!   worldsVM.worlds（id,name,bodyPreview,characterCount）、editId/editName/editBody、
//!   editing、preview
//! 命令：createNew / loadFor / save / importFile / rename(id,name) / remove(id) /
//!       togglePreview / setEditName / setEditBody


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    property string pendingWorldId: ""

    function renameWorld(id, name) {
        page.pendingWorldId = id
        renameDialog.openFor(qsTr("重命名世界观"), qsTr("世界观新名字"), name, false)
    }

    function removeWorld(id) {
        if (dialogs.confirm(qsTr("删除世界观"), qsTr("确定删除该世界观吗？")))
            worldsVM.remove(id)
    }

    InputDialog {
        id: renameDialog
        onAcceptedText: function(text) {
            worldsVM.rename(page.pendingWorldId, text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("世界观")
            description: qsTr("为角色提供共享的背景设定，注入对话提示词")

            SecondaryButton {
                text: qsTr("导入 .md")
                iconName: "book-open"
                onClicked: worldsVM.importFile()
            }
            PrimaryButton {
                text: qsTr("新建世界观")
                iconName: "plus"
                onClicked: worldsVM.createNew()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ==============================================================
            // 左侧：世界观列表
            // ==============================================================
            Rectangle {
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                color: Theme.bgSidebar

                Rectangle {
                    width: 1
                    height: parent.height
                    anchors.right: parent.right
                    color: Theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp3
                    spacing: Theme.sp2

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("世界观列表")
                        color: Theme.text3
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBadge
                        font.weight: 500
                    }

                    ListView {
                        id: worldList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.sp1
                        model: worldsVM.worlds

                        delegate: Rectangle {
                            id: worldRow
                            width: worldList.width
                            height: 62
                            radius: Theme.radiusControl
                            color: (worldsVM.editId === modelData.id) ? Theme.bgSelected
                                                                      : (rowHover.hovered
                                                                         ? Theme.bgSubtle : "transparent")

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.sp2
                                anchors.rightMargin: Theme.sp1
                                anchors.topMargin: Theme.sp1
                                anchors.bottomMargin: Theme.sp1
                                spacing: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.sp1

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: Theme.text1
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fsBody
                                        elide: Text.ElideRight
                                    }
                                    Tag {
                                        text: qsTr("%1 角色").arg(modelData.characterCount === undefined
                                                                  ? 0 : modelData.characterCount)
                                        tone: "neutral"
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.bodyPreview === undefined ? ""
                                                                              : modelData.bodyPreview
                                    color: Theme.text3
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fsCaption
                                    elide: Text.ElideRight
                                }
                            }

                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: Theme.sp1
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 0

                                IconButton {
                                    fallbackText: "✎"
                                    tooltip: qsTr("重命名")
                                    onClicked: page.renameWorld(modelData.id, modelData.name)
                                }
                                IconButton {
                                    iconName: "trash-2"
                                    tooltip: qsTr("删除")
                                    danger: true
                                    onClicked: page.removeWorld(modelData.id)
                                }
                            }

                            HoverHandler {
                                id: rowHover
                            }
                            MouseArea {
                                anchors.fill: parent
                                anchors.rightMargin: 56
                                cursorShape: Qt.PointingHandCursor
                                onClicked: worldsVM.loadFor(modelData.id)
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: worldsVM.worlds.length === 0
                        text: qsTr("暂无世界观")
                        color: Theme.text3
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // ==============================================================
            // 右侧：编辑区
            // ==============================================================
            PageScroll {
                Layout.fillWidth: true
                Layout.fillHeight: true

                EmptyState {
                    Layout.fillWidth: true
                    visible: worldsVM.editId.length === 0 && !worldsVM.editing
                    iconName: "book-open"
                    title: qsTr("选择或新建一个世界观")
                    description: qsTr("世界观正文会作为提示词的世界观段落，与角色的人设一起注入。")
                    actionText: qsTr("新建世界观")
                    onActionClicked: worldsVM.createNew()
                }

                Card {
                    Layout.fillWidth: true
                    visible: worldsVM.editId.length > 0 || worldsVM.editing
                    title: worldsVM.editing && worldsVM.editId.length === 0
                           ? qsTr("新建世界观") : qsTr("编辑世界观")
                    description: qsTr("世界观为角色提供共享背景；保存后新对话立即生效")

                    TextFieldCard {
                        id: nameCard
                        Layout.fillWidth: true
                        label: qsTr("名称")
                        placeholder: qsTr("如：魔法学院")
                        onEdited: function(value) {
                            worldsVM.setEditName(value)
                        }
                    }

                    TextFieldCard {
                        id: bodyCard
                        Layout.fillWidth: true
                        visible: !worldsVM.preview
                        label: qsTr("正文（注入提示词）")
                        placeholder: qsTr("地理、历史、势力、规则…")
                        multiline: true
                        fieldHeight: 320
                        onEdited: function(value) {
                            worldsVM.setEditBody(value)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: worldsVM.preview
                        implicitHeight: previewText.implicitHeight + Theme.sp6
                        radius: Theme.radiusControl
                        color: Theme.bgSubtle
                        border.width: 1
                        border.color: Theme.border

                        Text {
                            id: previewText
                            anchors.fill: parent
                            anchors.margins: Theme.sp3
                            text: worldsVM.editBody
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsBody
                            textFormat: Text.MarkdownText
                            wrapMode: Text.Wrap
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp2

                        SecondaryButton {
                            text: worldsVM.preview ? qsTr("编辑") : qsTr("预览")
                            onClicked: worldsVM.togglePreview()
                        }
                        SecondaryButton {
                            text: qsTr("绑定到当前角色")
                            visible: worldsVM.editId.length > 0
                            onClicked: worldsVM.assignToCurrent(worldsVM.editId)
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        PrimaryButton {
                            text: qsTr("保存")
                            enabled: worldsVM.editing
                            onClicked: worldsVM.save()
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        nameCard.text = worldsVM.editName
        bodyCard.text = worldsVM.editBody
    }

    Connections {
        target: worldsVM

        function onEditNameChanged() {
            nameCard.text = worldsVM.editName
        }
        function onEditBodyChanged() {
            bodyCard.text = worldsVM.editBody
        }
    }
}
