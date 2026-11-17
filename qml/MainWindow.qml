//! \file MainWindow.qml
//! \brief 主窗口（软件本体）：1280×800，左侧 200px 导航 + 右侧页面栈 + 顶栏（页内）。
//!
//! 布局（对齐 ccswitch-v4-ui-spec §2）：
//!   ├ 侧栏 200：品牌（应用名 + 版本）+ 导航（聊天/角色/人设/世界观/记忆/模型/
//!   │  记录/数据/设置）+ 底部主题/语言/关于 + 桌宠运行状态与开关
//!   └ 内容区：页面栈（每页自带 52px 页头：标题/描述/主操作）+ 底部状态栏
//!
//! 窗口职责（§7.13）：
//!   - 唯一常驻主窗口；关闭 = 隐藏到托盘（appVM.hideMainWindow()），不退出进程；
//!   - 由 C++ Application / WindowRegistry 负责显示/隐藏与二次启动激活；
//!   - 聊天统一在本窗口内（不再有独立桌面 Chat 窗口）。
//!
//! \note objectName = "mainWindow"，供 WindowRegistry 按名查找。
//! \note 关于页不在 0~8 页导航内，用本地 aboutVisible 维护（不占用 appVM.pageIndex）。


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "components"
import "pages"

Window {
    id: root

    objectName: "mainWindow"
    width: 1280
    height: 800
    minimumWidth: 980
    minimumHeight: 640
    visible: true
    title: qsTr("桌面桌宠") + (appVM.appVersion.length > 0
                              ? " · v" + appVM.appVersion : "")
    color: Theme.bgApp

    //! 关于页显隐（本地 UI 状态，关于页不属于 appVM.pageIndex 0~8）
    property bool aboutVisible: false

    //! 侧栏导航项
    readonly property var navItems: [
        { "icon": "activity", "label": qsTr("聊天") },
        { "icon": "images", "label": qsTr("角色") },
        { "icon": "sparkles", "label": qsTr("人设") },
        { "icon": "monitor", "label": qsTr("世界观") },
        { "icon": "book-open", "label": qsTr("记忆") },
        { "icon": "cpu", "label": qsTr("模型") },
        { "icon": "film", "label": qsTr("记录") },
        { "icon": "folder-open", "label": qsTr("数据") },
        { "icon": "settings", "label": qsTr("设置") }
    ]

    //! 关闭 = 隐藏到托盘（§7.13），不退出进程
    onClosing: function(close) {
        close.accepted = false
        appVM.hideMainWindow()
    }

    Component.onCompleted: {
        if (!appVM.onboardingDone)
            onboardingDialog.open()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ==================================================================
        // 侧栏
        // ==================================================================
        Rectangle {
            id: sidebar
            Layout.preferredWidth: Theme.sidebarWidth
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
                anchors.margins: Theme.sp2
                spacing: Theme.spHalf

                // 品牌行：logo + 应用名 + 版本
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    spacing: Theme.sp2

                    Rectangle {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: Theme.radiusIconBox
                        color: Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("宠")
                            color: Theme.accentFg
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsStrong
                            font.weight: 600
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("桌面桌宠")
                            color: Theme.text1
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsStrong
                            font.weight: 600
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "v" + appVM.appVersion
                            color: Theme.text3
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fsBadge
                            elide: Text.ElideRight
                        }
                    }
                }

                // 导航
                Repeater {
                    model: root.navItems

                    delegate: SidebarItem {
                        Layout.fillWidth: true
                        iconName: modelData.icon
                        label: modelData.label
                        selected: !root.aboutVisible && appVM.pageIndex === index
                        onClicked: {
                            root.aboutVisible = false
                            appVM.setPageIndex(index)
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: Theme.sp1
                    Layout.bottomMargin: Theme.sp1
                    color: Theme.border
                }

                // 主题 / 语言 / 关于
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spHalf

                    IconButton {
                        iconName: Theme.dark ? "sun" : "moon"
                        tooltip: Theme.dark ? qsTr("切换到浅色主题")
                                            : qsTr("切换到深色主题")
                        onClicked: appVM.setThemeIndex(Theme.dark ? 0 : 1)
                    }
                    IconButton {
                        fallbackText: appVM.languageIndex === 0 ? qsTr("中")
                                      : (appVM.languageIndex === 1 ? "EN" : qsTr("系"))
                        tooltip: qsTr("切换语言（需重启生效）")
                        onClicked: {
                            appVM.setLanguageIndex((appVM.languageIndex + 1) % 3)
                            toast.show(qsTr("语言已切换，重启应用后生效"))
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    IconButton {
                        iconName: "info"
                        tooltip: qsTr("关于")
                        onClicked: root.aboutVisible = true
                    }
                }

                // 桌宠运行状态 + 启动/停止开关
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.hControl
                    spacing: Theme.sp1

                    Rectangle {
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 3
                        color: appVM.petVisible ? Theme.success : Theme.text3
                    }
                    Text {
                        Layout.fillWidth: true
                        text: appVM.petStatusText.length > 0
                              ? appVM.petStatusText
                              : (appVM.petVisible ? qsTr("桌宠运行中") : qsTr("桌宠已停止"))
                        color: Theme.text2
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fsBadge
                        elide: Text.ElideRight
                    }
                    ToggleSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: appVM.petVisible
                        onClicked: appVM.setPetVisible(!appVM.petVisible)
                    }
                }
            }
        }

        // ==================================================================
        // 内容区：页面栈 + 状态栏
        // ==================================================================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            StackLayout {
                id: pageStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.aboutVisible
                              ? 9
                              : Math.max(0, Math.min(8, appVM.pageIndex))

                ChatPage {}
                CharactersPage {}
                PersonasPage {}
                WorldsPage {}
                MemoriesPage {}
                ModelsPage {}
                RecordsPage {}
                DataPage {}
                SettingsPage {
                    onAboutRequested: root.aboutVisible = true
                }
                AboutPage {
                    onBackRequested: root.aboutVisible = false
                }
            }

            StatusBar {
                Layout.fillWidth: true
                message: appVM.appStatus
                rightText: appVM.petStatusText
                dotColor: appVM.petVisible ? Theme.success : Theme.text3
                version: appVM.appVersion.length > 0 ? "v" + appVM.appVersion : ""
            }
        }
    }

    // ======================================================================
    // 轻提示
    // ======================================================================
    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.statusBarHeight + Theme.sp5
        z: 1000
    }

    // ======================================================================
    // 新手引导（首次启动 / 设置中重置后显示）
    // ======================================================================
    Dialog {
        id: onboardingDialog
        modal: true
        width: 520
        padding: Theme.sp5
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: Overlay.overlay
        title: qsTr("欢迎使用桌面桌宠")

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
            text: qsTr("欢迎使用桌面桌宠")
            color: Theme.text1
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fsTitle
            font.weight: 600
        }

        contentItem: ColumnLayout {
            width: Math.max(0, onboardingDialog.availableWidth)
            spacing: Theme.sp2

            Repeater {
                model: [
                    qsTr("聊天：主窗口「聊天」页专注对话，Enter 发送、Shift+Enter 换行。"),
                    qsTr("角色：在「角色」页新建或导入角色，点「启动为桌宠」让 TA 出现在桌面。"),
                    qsTr("桌宠：左键点击按命中区说台词，拖动移动，右键菜单，双击打开主窗口聊天。"),
                    qsTr("模型：在「模型」页选择预设或填写 OpenAI 兼容接口，测试连接后保存。"),
                    qsTr("快捷键：Ctrl+Alt+P 显隐桌宠 · Ctrl+Alt+C 打开聊天 · Ctrl+Alt+M 勿扰。")
                ]

                delegate: Text {
                    Layout.fillWidth: true
                    text: "• " + modelData
                    color: Theme.text2
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fsBody
                    lineHeight: 1.45
                    wrapMode: Text.Wrap
                }
            }
        }

        footer: RowLayout {
            spacing: Theme.sp2
            Item {
                Layout.fillWidth: true
            }
            SecondaryButton {
                text: qsTr("稍后")
                onClicked: onboardingDialog.close()
            }
            PrimaryButton {
                text: qsTr("开始使用")
                onClicked: {
                    appVM.setOnboardingDone(true)
                    onboardingDialog.close()
                }
            }
        }
    }

    // ======================================================================
    // ViewModel → UI
    // ======================================================================
    Connections {
        target: appVM

        function onToast(message) {
            toast.show(message)
        }
        function onStatusMessage(message) {
            if (message.length > 0)
                toast.show(message)
        }
        function onUpdateAvailable(version) {
            toast.show(qsTr("发现新版本 %1，可在「设置 - 更新」中查看").arg(version))
        }
        function onOnboardingDoneChanged() {
            if (!appVM.onboardingDone)
                onboardingDialog.open()
        }
    }
}
