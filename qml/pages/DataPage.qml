//! \file DataPage.qml
//! \brief 数据页：数据/日志目录、占用空间、最近导出、导出与维护操作。
//!
//! 数据绑定（§7.10）：
//!   dataVM.dataPath / logPath / dataSize / lastExportPath / busy
//! 命令：exportChats() / exportLogs() / openDataDir() / openLogsDir() /
//!       resetOnboarding() / checkUpdate()
//!
//! \note 导出无 DialogService 时由 ViewModel 默认写到 <AppData>/exports/。


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("数据")
            description: qsTr("聊天记录、日志与本地数据的导出和位置管理")

            SecondaryButton {
                text: qsTr("导出聊天")
                iconName: "external-link"
                enabled: !dataVM.busy
                onClicked: dataVM.exportChats()
            }
            SecondaryButton {
                text: qsTr("导出日志")
                iconName: "film"
                enabled: !dataVM.busy
                onClicked: dataVM.exportLogs()
            }
            PrimaryButton {
                text: qsTr("打开数据目录")
                iconName: "folder-open"
                onClicked: dataVM.openDataDir()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ==============================================================
            // 存储位置
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("存储位置")
                description: qsTr("所有配置与聊天记录均保存在本机，不上传云端")

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("数据目录")

                    Text {
                        Layout.fillWidth: true
                        text: dataVM.dataPath
                        color: Theme.text2
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fsCaption
                        wrapMode: Text.Wrap
                    }
                    SecondaryButton {
                        text: qsTr("打开")
                        compact: true
                        onClicked: dataVM.openDataDir()
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("日志目录")

                    Text {
                        Layout.fillWidth: true
                        text: dataVM.logPath
                        color: Theme.text2
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fsCaption
                        wrapMode: Text.Wrap
                    }
                    SecondaryButton {
                        text: qsTr("打开")
                        compact: true
                        onClicked: dataVM.openLogsDir()
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("占用空间")

                    Text {
                        Layout.fillWidth: true
                        text: dataVM.dataSize
                        color: Theme.text1
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBody
                    }
                }

                FormRow {
                    Layout.fillWidth: true
                    label: qsTr("最近导出")

                    Text {
                        Layout.fillWidth: true
                        text: dataVM.lastExportPath.length > 0
                              ? dataVM.lastExportPath : qsTr("尚未导出")
                        color: Theme.text2
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fsCaption
                        wrapMode: Text.Wrap
                    }
                }
            }

            // ==============================================================
            // 维护
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("维护")
                description: qsTr("导出格式为 JSON / Markdown，可用于备份或迁移")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    SecondaryButton {
                        text: qsTr("导出聊天记录")
                        enabled: !dataVM.busy
                        onClicked: dataVM.exportChats()
                    }
                    SecondaryButton {
                        text: qsTr("导出日志")
                        enabled: !dataVM.busy
                        onClicked: dataVM.exportLogs()
                    }
                    SecondaryButton {
                        text: qsTr("检查更新")
                        iconName: "refresh-cw"
                        enabled: !dataVM.busy
                        onClicked: dataVM.checkUpdate()
                    }
                    SecondaryButton {
                        text: qsTr("重置新手引导")
                        enabled: !dataVM.busy
                        onClicked: dataVM.resetOnboarding()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("提示：导出文件默认保存在数据目录下的 exports/ 中。")
                    color: Theme.text3
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsCaption
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: dataVM.busy
                    spacing: Theme.sp2

                    Rectangle {
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 3
                        color: Theme.accent
                    }
                    Text {
                        text: qsTr("处理中…")
                        color: Theme.text2
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsCaption
                    }
                }
            }
        }
    }
}
