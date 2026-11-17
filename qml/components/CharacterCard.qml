//! \file CharacterCard.qml
//! \brief 角色卡片：形象缩略图 / 名字 / 世界观 / 启用开关 / 使用中 / 桌宠状态。
//!
//! \param characterId 角色 id
//! \param name        角色名
//! \param avatar      形象路径
//! \param worldName   绑定世界观名（空=未绑定）
//! \param characterEnabled 是否启用
//! \param current     是否为当前使用角色
//! \param isPet       是否为当前桌宠
//!
//! \signal useRequested / petRequested / toggleEnabled(bool) / renameRequested /
//!         deleteRequested / exportRequested

import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string characterId: ""
    property string name: ""
    property string avatar: ""
    property string worldName: ""
    property bool characterEnabled: true
    property bool current: false
    property bool isPet: false

    signal useRequested()
    signal petRequested()
    signal toggleEnabled(bool value)
    signal renameRequested()
    signal deleteRequested()
    signal exportRequested()

    implicitWidth: 300
    implicitHeight: 272
    radius: Theme.radiusPanel
    color: root.current ? Theme.cardActiveBg : Theme.bgCard
    border.width: 1
    border.color: root.current ? Theme.cardActiveBorder : Theme.border

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp4
        spacing: Theme.sp3

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp3

            Avatar {
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
                source: root.avatar
                name: root.name
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.sp1

                Text {
                    Layout.fillWidth: true
                    text: root.name
                    color: Theme.text1
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsStrong
                    font.weight: 500
                    elide: Text.ElideRight
                }

                RowLayout {
                    spacing: Theme.sp1

                    Tag {
                        visible: root.current
                        text: qsTr("使用中")
                        tone: "accent"
                    }
                    Tag {
                        visible: root.isPet
                        text: qsTr("桌宠")
                        tone: "success"
                    }
                    Tag {
                        visible: !root.characterEnabled
                        text: qsTr("已禁用")
                        tone: "neutral"
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.worldName.length > 0 ? qsTr("世界观：") + root.worldName
                                            : qsTr("未绑定世界观")
            color: Theme.text2
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsCaption
            elide: Text.ElideRight
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2

            SecondaryButton {
                text: root.current ? qsTr("使用中") : qsTr("使用")
                compact: true
                enabled: !root.current
                onClicked: root.useRequested()
            }
            SecondaryButton {
                text: root.isPet ? qsTr("桌宠中") : qsTr("启动为桌宠")
                compact: true
                onClicked: root.petRequested()
            }
            Item {
                Layout.fillWidth: true
            }
            ToggleSwitch {
                checked: root.characterEnabled
                Layout.alignment: Qt.AlignVCenter
                onClicked: root.toggleEnabled(!root.characterEnabled)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2

            Text {
                Layout.fillWidth: true
                text: root.characterEnabled ? qsTr("已启用") : qsTr("已禁用")
                color: Theme.text3
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fsCaption
            }
            IconButton {
                fallbackText: "✎"
                tooltip: qsTr("重命名")
                onClicked: root.renameRequested()
            }
            IconButton {
                iconName: "external-link"
                tooltip: qsTr("导出角色包")
                onClicked: root.exportRequested()
            }
            IconButton {
                iconName: "trash-2"
                tooltip: qsTr("删除角色")
                danger: true
                onClicked: root.deleteRequested()
            }
        }
    }
}
