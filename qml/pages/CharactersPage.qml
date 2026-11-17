//! \file CharactersPage.qml
//! \brief 角色页：卡片网格（形象/名字/世界观/启用/当前/桌宠），新建/导入/导出。
//!
//! 数据绑定（§7.4）：
//!   charactersVM.characters（id,name,avatar,worldId,worldName,enabled,current,isPet）、
//!   emptyHint、busy
//! 命令：create(name) / importAvatar() / importPack() / exportPack(id) /
//!       rename(id,name) / remove(id) / select(id) / setEnabled(id,bool) /
//!       launchAsPet(id) / refresh()


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    property string pendingCharacterId: ""

    function createCharacter() {
        createDialog.openFor(qsTr("新建角色"), qsTr("角色名字"), qsTr("小柚"), false)
    }

    function renameCharacter(id, name) {
        page.pendingCharacterId = id
        renameDialog.openFor(qsTr("重命名角色"), qsTr("角色新名字"), name, false)
    }

    function removeCharacter(id) {
        if (dialogs.confirm(qsTr("删除角色"), qsTr("确定删除该角色及其聊天记录吗？")))
            charactersVM.remove(id)
    }

    InputDialog {
        id: createDialog
        onAcceptedText: function(text) {
            charactersVM.create(text)
        }
    }

    InputDialog {
        id: renameDialog
        onAcceptedText: function(text) {
            charactersVM.rename(page.pendingCharacterId, text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("角色")
            description: qsTr("管理桌宠角色：形象、世界观与启用状态")

            IconButton {
                iconName: "refresh-cw"
                tooltip: qsTr("刷新")
                enabled: !charactersVM.busy
                onClicked: charactersVM.refresh()
            }
            SecondaryButton {
                text: qsTr("导入形象")
                iconName: "image"
                enabled: !charactersVM.busy
                onClicked: charactersVM.importAvatar()
            }
            SecondaryButton {
                text: qsTr("导入角色包")
                iconName: "images"
                enabled: !charactersVM.busy
                onClicked: charactersVM.importPack()
            }
            PrimaryButton {
                text: qsTr("新建角色")
                iconName: "plus"
                enabled: !charactersVM.busy
                onClicked: page.createCharacter()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            EmptyState {
                Layout.fillWidth: true
                visible: charactersVM.characters.length === 0
                iconName: "images"
                title: qsTr("还没有角色")
                description: charactersVM.emptyHint.length > 0
                             ? charactersVM.emptyHint
                             : qsTr("新建一个角色，或导入形象 / 角色包开始陪伴。")
                actionText: qsTr("新建角色")
                onActionClicked: page.createCharacter()
            }

            Flow {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                spacing: Theme.sp2

                Repeater {
                    model: charactersVM.characters

                    delegate: CharacterCard {
                        width: Math.floor((parent.width - (parent.width < 780 ? Theme.sp2
                                                                             : Theme.sp2 * 2))
                                          / (parent.width < 780 ? 2 : 3))
                        height: 272
                        characterId: modelData.id
                        name: modelData.name
                        avatar: modelData.avatar
                        worldName: modelData.worldName === undefined ? ""
                                                                    : modelData.worldName
                        characterEnabled: modelData.enabled === true
                        current: modelData.current === true
                        isPet: modelData.isPet === true

                        onUseRequested: charactersVM.select(modelData.id)
                        onPetRequested: charactersVM.launchAsPet(modelData.id)
                        onToggleEnabled: function(value) {
                            charactersVM.setEnabled(modelData.id, value)
                        }
                        onRenameRequested: page.renameCharacter(modelData.id, modelData.name)
                        onDeleteRequested: page.removeCharacter(modelData.id)
                        onExportRequested: charactersVM.exportPack(modelData.id)
                    }
                }
            }
        }
    }
}
