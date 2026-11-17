//! \file PersonasPage.qml
//! \brief 人设页：左侧人设列表 + 右侧编辑区（名称/正文/预览/保存/删除）。
//!
//! 数据绑定（§7.5）：
//!   personasVM.personas（id,name,current）、currentName、editName、editBody、
//!   editing、preview、editId、intimacyLevel
//! 命令：createNew / loadFor / save / importMd / select / remove /
//!       setEditName / setEditBody / togglePreview


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    function removeCurrent() {
        if (dialogs.confirm(qsTr("删除人设"), qsTr("确定删除当前人设吗？")))
            personasVM.remove(personasVM.editId)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("人设")
            description: qsTr("当前人设：%1 · 好感度 %2")
                         .arg(personasVM.currentName.length > 0
                              ? personasVM.currentName : qsTr("未选择"))
                         .arg(personasVM.intimacyLevel)

            SecondaryButton {
                text: qsTr("导入 .md")
                iconName: "book-open"
                onClicked: personasVM.importMd()
            }
            PrimaryButton {
                text: qsTr("新建人设")
                iconName: "plus"
                onClicked: personasVM.createNew()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ==============================================================
            // 左侧：人设列表
            // ==============================================================
            Rectangle {
                Layout.preferredWidth: 300
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
                        text: qsTr("人设列表")
                        color: Theme.text3
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBadge
                        font.weight: 500
                    }

                    ListView {
                        id: personaList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.sp1
                        model: personasVM.personas

                        delegate: Rectangle {
                            id: personaRow
                            width: personaList.width
                            height: 44
                            radius: Theme.radiusControl
                            color: modelData.current ? Theme.bgSelected
                                                     : (rowHover.hovered ? Theme.bgSubtle
                                                                         : "transparent")

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.sp2
                                anchors.rightMargin: Theme.sp2
                                spacing: Theme.sp2

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: Theme.text1
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fsBody
                                    elide: Text.ElideRight
                                }
                                Tag {
                                    visible: modelData.current === true
                                    text: qsTr("当前")
                                    tone: "accent"
                                }
                            }

                            HoverHandler {
                                id: rowHover
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: personasVM.loadFor(modelData.id)
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: personasVM.personas.length === 0
                        text: qsTr("暂无人设")
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
                    visible: personasVM.editId.length === 0 && !personasVM.editing
                    iconName: "sparkles"
                    title: qsTr("选择或新建一个人设")
                    description: qsTr("人设会作为提示词的一部分注入对话，影响角色的说话方式。")
                    actionText: qsTr("新建人设")
                    onActionClicked: personasVM.createNew()
                }

                Card {
                    Layout.fillWidth: true
                    visible: personasVM.editId.length > 0 || personasVM.editing
                    title: personasVM.editing && personasVM.editId.length === 0
                           ? qsTr("新建人设") : qsTr("编辑人设")
                    description: qsTr("保存后立即生效；「使用」可切换当前生效人设")

                    TextFieldCard {
                        id: nameCard
                        Layout.fillWidth: true
                        label: qsTr("名称")
                        placeholder: qsTr("如：小柚")
                        onEdited: function(value) {
                            personasVM.setEditName(value)
                        }
                    }

                    TextFieldCard {
                        id: bodyCard
                        Layout.fillWidth: true
                        visible: !personasVM.preview
                        label: qsTr("正文（注入提示词）")
                        placeholder: qsTr("描述性格、说话习惯、背景故事…")
                        multiline: true
                        fieldHeight: 280
                        onEdited: function(value) {
                            personasVM.setEditBody(value)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: personasVM.preview
                        implicitHeight: previewText.implicitHeight + Theme.sp6
                        radius: Theme.radiusControl
                        color: Theme.bgSubtle
                        border.width: 1
                        border.color: Theme.border

                        Text {
                            id: previewText
                            anchors.fill: parent
                            anchors.margins: Theme.sp3
                            text: personasVM.editBody
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
                            text: personasVM.preview ? qsTr("编辑") : qsTr("预览")
                            onClicked: personasVM.togglePreview()
                        }
                        SecondaryButton {
                            visible: personasVM.editId.length > 0
                            text: qsTr("使用")
                            onClicked: personasVM.select(personasVM.editId)
                        }
                        SecondaryButton {
                            visible: personasVM.editId.length > 0
                            text: qsTr("删除")
                            danger: true
                            onClicked: page.removeCurrent()
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        PrimaryButton {
                            text: qsTr("保存")
                            enabled: personasVM.editing
                            onClicked: personasVM.save()
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: personasVM.editing && personasVM.editId.length === 0
                        text: qsTr("提示：填写名称与正文后点击「保存」创建新人设。")
                        color: Theme.text3
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        nameCard.text = personasVM.editName
        bodyCard.text = personasVM.editBody
        // 打开页面时自动载入当前人设，直接进入编辑态。
        if (!personasVM.editing) {
            for (let i = 0; i < personasVM.personas.length; ++i) {
                const row = personasVM.personas[i]
                if (row.current === true) {
                    personasVM.loadFor(row.id)
                    break
                }
            }
        }
    }

    // VM → 编辑器字段（不在聚焦时打断输入）
    Connections {
        target: personasVM

        function onEditNameChanged() {
            nameCard.text = personasVM.editName
        }
        function onEditBodyChanged() {
            bodyCard.text = personasVM.editBody
        }
    }
}
