//! \file AboutPage.qml
//! \brief 关于页：应用信息、功能概览、开源许可与更新状态。
//!
//! \signal backRequested() 返回设置页（关于页不在 0~8 页导航中，由主窗口维护显隐）
//!
//! 数据绑定：appVM.appVersion / appVM.updateStatus / appVM.updateUrl


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: page

    signal backRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("关于")
            description: qsTr("桌面桌宠 · 版本 %1").arg(appVM.appVersion)

            SecondaryButton {
                text: qsTr("返回设置")
                onClicked: page.backRequested()
            }
            SecondaryButton {
                text: qsTr("检查更新")
                iconName: "refresh-cw"
                onClicked: appVM.checkUpdate()
            }
        }

        PageScroll {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ==============================================================
            // 应用信息
            // ==============================================================
            Card {
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp4

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        radius: Theme.radiusDialog
                        color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("宠")
                            color: Theme.accentFg
                            font.family: Theme.fontFamily
                            font.pixelSize: 26
                            font.weight: 600
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.sp1

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("桌面桌宠")
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsPage
                            font.weight: 600
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("版本 %1 · MVVM 重构版").arg(appVM.appVersion)
                            color: Theme.text2
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsCaption
                        }
                        RowLayout {
                            spacing: Theme.sp1

                            Tag {
                                text: "Windows"
                                tone: "neutral"
                            }
                            Tag {
                                text: "Qt 6"
                                tone: "neutral"
                            }
                            Tag {
                                text: "MVVM"
                                tone: "accent"
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("陪伴桌面的桌宠应用：主窗口管理角色、人设、世界观、记忆与模型，桌宠窗口可独立运行，支持 1~3 只桌宠、点击穿透、贴边隐藏与悬浮球。")
                    color: Theme.text2
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    lineHeight: 1.5
                    wrapMode: Text.Wrap
                }
            }

            // ==============================================================
            // 功能概览
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("功能概览")
                description: qsTr("所有数据保存在本机，模型通过 OpenAI 兼容接口接入")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    Repeater {
                        model: [
                            qsTr("专注聊天：760px 居中消息区、流式回复、卡片折叠、👍👎 与回溯"),
                            qsTr("角色与人设：卡片网格管理角色，启动为桌宠；人设与世界观注入提示词"),
                            qsTr("记忆与好感度：长期记忆抽取、置顶、跨天好感度衰减"),
                            qsTr("多模型：预设 / 自定义 Base URL / 多套配置 / 连接测试"),
                            qsTr("桌宠：多桌宠、拖拽、贴边、点击穿透、命中区调试、悬浮球与气泡"),
                            qsTr("数据：聊天与日志导出、数据目录管理、新手引导重置")
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2

                            Rectangle {
                                Layout.preferredWidth: 6
                                Layout.preferredHeight: 6
                                radius: 3
                                color: Theme.accent
                                Layout.alignment: Qt.AlignTop
                                Layout.topMargin: 6
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData
                                color: Theme.text2
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBody
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            // ==============================================================
            // 开源许可
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("开源许可")
                description: qsTr("本项目使用的第三方组件")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    Repeater {
                        model: [
                            { "name": "Qt 6", "license": "LGPLv3" },
                            { "name": "SQLite", "license": "Public Domain" },
                            { "name": "lucide icons", "license": "ISC" },
                            { "name": "stacklog / singleproc", "license": "MIT" }
                        ]

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp2

                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: Theme.text1
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fsBody
                            }
                            Text {
                                text: modelData.license
                                color: Theme.text3
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.fsCaption
                            }
                        }
                    }
                }
            }

            // ==============================================================
            // 更新 / 数据
            // ==============================================================
            Card {
                Layout.fillWidth: true
                title: qsTr("更新与数据")

                Text {
                    Layout.fillWidth: true
                    text: appVM.updateStatus.length > 0
                          ? appVM.updateStatus : qsTr("暂无更新信息")
                    color: Theme.text2
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    wrapMode: Text.Wrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: appVM.updateUrl.length > 0
                    text: qsTr("更新地址：%1").arg(appVM.updateUrl)
                    color: Theme.text3
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.fsCaption
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp2

                    SecondaryButton {
                        text: qsTr("打开数据目录")
                        iconName: "folder-open"
                        onClicked: appVM.openDataDir()
                    }
                    SecondaryButton {
                        text: qsTr("打开日志目录")
                        onClicked: appVM.openLogsDir()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
